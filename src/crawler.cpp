#include "crawler.hpp"

#include "http_fetcher.hpp"
#include "link_extractor.hpp"
#include "safe_queue.hpp"

#include <iostream>

// The core BFS step: pop one task, fetch, extract, enqueue new links.
// We have first written it as a free function so that the multithreaded version can run it on N threads.
static void process_task(CrawlTask task,
                         StripedHashSet& visited,
                         SafeBFSQueue&   queue,
                         HttpFetcher&    fetcher,
                         const CrawlConfig& cfg) {
    if (task.depth > cfg.max_depth)
        return;

    std::string html = fetcher.fetch(task.url);

    if (html.empty())
        return; // fetch failed or non-HTML

    for (const std::string& link : extract_links(html, task.url)) {
        if (cfg.filter && !cfg.filter(link))
            continue;

        int child_depth = task.depth + 1;
        if (child_depth > cfg.max_depth)
            continue; // no point recording pages we won't crawl

        PageInfo info;
        info.url           = link;
        info.depth         = child_depth;
        info.in_link_count = 1;
        info.parent_url    = task.url;

        if (visited.insert_if_absent(link, info)) {
            // if we see it for the first time we push it for crawling
            queue.push({link, child_depth, task.url});
        } else {
            // Already visited, so we'll just bump its in-link counter
            visited.increment_inlinks(link);
        }
    }
}

void crawl(const CrawlConfig& cfg, StripedHashSet& visited) {
    SafeBFSQueue   queue;
    HttpFetcher    fetcher;

    if (!fetcher.is_ready()) {
        std::cerr << "curl init failed\n";
        return;
    }

    // we insert the start URL at depth 0 with no parent
    PageInfo seed_info{cfg.seed_url, 0, 0, ""};

    visited.insert_if_absent(cfg.seed_url, seed_info);

    queue.push({cfg.seed_url, 0, ""});

    std::cout << "Crawling: " << cfg.seed_url << "\n";

    CrawlTask task;

    while (queue.try_pop(task, 0)) {

        std::cout << "  [depth : " << task.depth << "] " << task.url  << "\n";

        process_task(task, visited, queue, fetcher, cfg);
    }

    std::cout << "\nDone. Pages visited: " << visited.size() << "\n";
}
