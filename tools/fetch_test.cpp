// Fetches a single page and prints extracted links after domain filtering.
// Usage: ./fetch_test [url]   (defaults to Concurrent_computing on Wikipedia)

#include "http_fetcher.hpp"
#include "link_extractor.hpp"
#include "url_filter.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string url = (argc > 1)
        ? argv[1]
        : "https://en.wikipedia.org/wiki/Concurrent_computing";

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

    auto links = extract_links(html, url);
    std::cout << "Total links found: " << links.size() << "\n\n";

    // keep only links on the same domain
    auto keep = domain_filter(url_host(url));

    std::vector<std::string> filtered;
    for (const auto& link : links)
        if (keep(link)) filtered.push_back(link);

    std::cout << "After domain filter: " << filtered.size() << " links\n\n";

    int show = std::min<int>(20, filtered.size());
    for (int i = 0; i < show; ++i)
        std::cout << "  " << filtered[i] << "\n";
    if ((int)filtered.size() > show)
        std::cout << "  ... and " << filtered.size() - show << " more\n";

    return 0;
}
