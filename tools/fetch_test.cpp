// Smoke test: fetch one Wikipedia page, extract its links, apply a domain filter.
// Run: ./build/fetch_test [url]

#include "http_fetcher.hpp"
#include "link_extractor.hpp"
#include "url_filter.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string url = (argc > 1)
        ? argv[1]
        : "https://en.wikipedia.org/wiki/Concurrent_computing";

    // 1. Fetch the page -------------------------------------------------------
    // HttpFetcher wraps a single libcurl handle.  Each worker thread in the
    // real crawler will own one of these (they're not thread-safe to share).
    HttpFetcher fetcher;
    if (!fetcher.is_ready()) {
        std::cerr << "curl init failed\n";
        return 1;
    }

    std::cout << "Fetching: " << url << "\n";
    std::string html = fetcher.fetch(url);

    if (html.empty()) {
        std::cerr << "Fetch failed: " << fetcher.last_error()
                  << " (HTTP " << fetcher.last_status_code() << ")\n";
        return 1;
    }
    std::cout << "Got " << html.size() << " bytes  (HTTP "
              << fetcher.last_status_code() << ")\n\n";

    // 2. Extract links --------------------------------------------------------
    // extract_links scans the HTML for <a href="..."> attributes and resolves
    // every href to an absolute URL using `url` as the base.
    auto links = extract_links(html, url);
    std::cout << "Total links found: " << links.size() << "\n\n";

    // 3. Apply a domain filter ------------------------------------------------
    // domain_filter returns a predicate that keeps only URLs whose host matches.
    // For en.wikipedia.org this drops external links (other sites, sister wikis, etc.)
    auto keep = domain_filter("en.wikipedia.org");

    std::vector<std::string> filtered;
    for (const auto& link : links)
        if (keep(link)) filtered.push_back(link);

    std::cout << "After domain filter (en.wikipedia.org): "
              << filtered.size() << " links\n\n";

    // Print first 20 so we can sanity-check the output
    int show = std::min<int>(20, filtered.size());
    for (int i = 0; i < show; ++i)
        std::cout << "  " << filtered[i] << "\n";
    if ((int)filtered.size() > show)
        std::cout << "  ... and " << filtered.size() - show << " more\n";

    return 0;
}
