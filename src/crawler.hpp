#pragma once

#include "page_info.hpp"
#include "striped_hash_set.hpp"
#include "url_filter.hpp"

#include <atomic>
#include <string>

// Parameters for one crawl run.
struct CrawlConfig {
    std::string seed_url;
    int         num_threads = 1;
    int         max_depth   = 3;
    UrlFilter   filter;        // empty filter means follow every URL
};

// Statistics collected during one crawl run.
struct CrawlStats {
    std::atomic<int> tasks_processed{0};
    std::atomic<int> pages_fetched{0};
    std::atomic<int> fetch_errors{0};
    std::atomic<int> links_extracted{0};
    std::atomic<int> links_accepted{0};
    std::atomic<int> new_pages{0};
    std::atomic<int> duplicate_links{0};
};

// Runs the crawler and fills 'visited' with discovered pages.
void crawl(const CrawlConfig& cfg, StripedHashSet& visited, CrawlStats& stats);