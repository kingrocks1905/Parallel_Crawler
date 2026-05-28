#pragma once

#include <functional>
#include <string>

using UrlFilter = std::function<bool(const std::string&)>;

// Here we extract the host from a URL ("https://example.com/path" → "example.com").
inline std::string url_host(const std::string& url) {

    auto sep = url.find("://");

    if (sep == std::string::npos) return "";
    auto start = sep + 3;
    auto end   = url.find('/', start);
    return url.substr(start, end == std::string::npos ? end : end - start);
}

// We only follow URLs on the same domain as the seed.
inline UrlFilter domain_filter(const std::string& base_domain) {

    return [base_domain](const std::string& url) {
        return url_host(url) == base_domain;
    };
}

// Here we will only follow  the URLs that start with a given prefix (e.g. "https://example.com/docs/").
inline UrlFilter prefix_filter(const std::string& prefix) {

    return [prefix](const std::string& url) {

        return url.rfind(prefix, 0) == 0;
    };
}

// we combine two filters, both must pass.
inline UrlFilter and_filter(UrlFilter a, UrlFilter b) {
    return [a, b](const std::string& url) {
        return a(url) && b(url);
    };
}
