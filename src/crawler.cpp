#include "crawler.hpp"

#include "http_fetcher.hpp"
#include "link_extractor.hpp"
#include "safe_queue.hpp"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// One processed task may create more tasks. This helper does the actual work.
static void process_task(const CrawlTask& task,
                         StripedHashSet& visited,
                         SafeBFSQueue& queue,
                         HttpFetcher& fetcher,
                         const CrawlConfig& cfg,
                         CrawlStats& stats,
                         std::atomic<int>& pending_tasks) {
    if (task.depth > cfg.max_depth) {
        return;
    }

    stats.tasks_processed++;

    std::string html = fetcher.fetch(task.url);
    if (html.empty()) {
        stats.fetch_errors++;
        return;
    }

    stats.pages_fetched++;

    // Pages at max depth are fetched but their outgoing links are not expanded.
    if (task.depth >= cfg.max_depth) {
        return;
    }

    std::vector<std::string> links = extract_links(html, task.url);
    stats.links_extracted += static_cast<int>(links.size());

    for (const std::string& link : links) {
        if (cfg.filter && !cfg.filter(link)) {
            continue;
        }

        stats.links_accepted++;

        int child_depth = task.depth + 1;
        if (child_depth > cfg.max_depth) {
            continue;
        }

        PageInfo info;
        info.url = link;
        info.depth = child_depth;
        info.in_link_count = 1;
        info.parent_url = task.url;

        if (visited.insert_if_absent(link, info)) {
            stats.new_pages++;
            pending_tasks++;
            queue.push({link, child_depth, task.url});
        } else {
            stats.duplicate_links++;
            visited.increment_inlinks(link);
        }
    }
}

void crawl(const CrawlConfig& cfg, StripedHashSet& visited, CrawlStats& stats) {
    SafeBFSQueue queue;
    std::atomic<int> pending_tasks{1};
    std::atomic<bool> finished{false};
    std::mutex print_mutex;

    int thread_count = std::max(1, cfg.num_threads);

    // The seed URL is depth 0 and has no parent.
    PageInfo seed_info;
    seed_info.url = cfg.seed_url;
    seed_info.depth = 0;
    seed_info.in_link_count = 0;
    seed_info.parent_url = "";

    visited.insert_if_absent(cfg.seed_url, seed_info);
    queue.push({cfg.seed_url, 0, ""});

    std::cout << "Crawling: " << cfg.seed_url << "\n";
    std::cout << "Worker threads: " << thread_count << "\n";

    auto worker = [&](int worker_id) {
        HttpFetcher fetcher;
        if (!fetcher.is_ready()) {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cerr << "Worker " << worker_id << ": curl init failed\n";
            finished = true;
            queue.shutdown();
            return;
        }

        while (!finished.load()) {
            CrawlTask task;

            if (!queue.try_pop(task, 100)) {
                continue;
            }

            process_task(task, visited, queue, fetcher, cfg, stats, pending_tasks);

            int left = --pending_tasks;
            if (left == 0) {
                finished = true;
                queue.shutdown();
            }
        }
    };

    std::vector<std::thread> workers;
    for (int i = 0; i < thread_count; i++) {
        workers.emplace_back(worker, i);
    }

    for (std::thread& t : workers) {
        t.join();
    }

    std::cout << "\nDone. Pages discovered: " << visited.size() << "\n";
}