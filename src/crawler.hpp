#pragma once

#include "page_info.hpp"
#include "striped_hash_set.hpp"
#include "url_filter.hpp"

#include <string>

// Parameters for a crawl run.
struct CrawlConfig {
    std::string seed_url;
    int         num_threads = 1;  // 1 = single-threaded
    int         max_depth   = 3;
    UrlFilter   filter;           // which URLs to follow (nullptr = follow all)
};

// Runs a BFS crawl and populates `visited` with all discovered pages.
// Single-threaded for now; num_threads > 1 will be wired up later.
void crawl(const CrawlConfig& cfg, StripedHashSet& visited);
