#include "link_extractor.hpp"

#include <regex>
#include <set>
#include <string>
#include <vector>

// cracks a base URL into scheme, origin, and directory for resolving relative hrefs
// e.g. "https://example.com/news/today.html" -> scheme="https", origin="https://example.com", dir=".../news/"
struct ParsedBase {
    std::string scheme;
    std::string origin;
    std::string dir;
};

static ParsedBase parse_base(const std::string& base_url) {
    ParsedBase p;

    auto sep = base_url.find("://");
    if (sep == std::string::npos)
        return p; // malformed URL

    p.scheme = base_url.substr(0, sep);

    // origin ends at the first '/' after the scheme
    auto slash = base_url.find('/', sep + 3);
    if (slash == std::string::npos) {
        p.origin = base_url;
        p.dir    = base_url + '/';
    } else {
        p.origin = base_url.substr(0, slash);
        p.dir    = base_url.substr(0, base_url.rfind('/') + 1);
    }

    return p;
}

// turns a raw href into an absolute URL, handles the 5 common href forms
static std::string resolve_url(const std::string& href, const ParsedBase& base) {
    
    if (href.empty() || base.origin.empty())
        return "";

    if (href[0] == '#')                     // same-page anchor
        return "";
    if (href.rfind("http://", 0) == 0 || href.rfind("https://", 0) == 0)
        return href;
    if (href.rfind("//", 0) == 0)            // protocol-relative
        return base.scheme + ':' + href;
    if (href[0] == '/')                      // absolute path
        return base.origin + href;

    return base.dir + href;                  // relative
}

std::vector<std::string> extract_links(const std::string& html,
                                       const std::string& base_url) {
    // matches href="..." and href='...'
    static const std::regex href_re(
        R"re(href\s*=\s*(?:"([^"]*)"|'([^']*)')\s*)re",
        std::regex::icase
    );

    ParsedBase base = parse_base(base_url);
    std::set<std::string>    seen;
    std::vector<std::string> result;

    for (auto it = std::sregex_iterator(html.begin(), html.end(), href_re);
         it != std::sregex_iterator(); ++it) {

        // group 1 is double-quoted, group 2 single-quoted
        std::string href = (*it)[1].matched ? (*it)[1].str() : (*it)[2].str();
        std::string url  = resolve_url(href, base);
        if (url.empty()) continue;

        // strip fragment so page#section == page
        auto frag = url.find('#');
        if (frag != std::string::npos)
            url.erase(frag);

        if (seen.insert(url).second)
            result.push_back(url);
    }

    return result;
}
