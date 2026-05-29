#include "crawler.hpp"
#include "output.hpp"
#include "url_filter.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>

// Usage: ./crawler <seed_url> [num_threads] [max_depth]
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <seed_url> [num_threads] [max_depth]\n";
        return 1;
    }

    CrawlConfig cfg;
    cfg.seed_url = argv[1];
    cfg.num_threads = (argc > 2) ? std::stoi(argv[2]) : 1;
    cfg.max_depth = (argc > 3) ? std::stoi(argv[3]) : 3;

    cfg.num_threads = std::max(1, cfg.num_threads);
    cfg.max_depth = std::max(0, cfg.max_depth);

    // By default, stay on the same host as the seed URL.
    cfg.filter = domain_filter(url_host(cfg.seed_url));

    std::cout << "Threads : " << cfg.num_threads << "\n"
              << "MaxDepth: " << cfg.max_depth << "\n\n";

    StripedHashSet visited;
    CrawlStats stats;

    auto t0 = std::chrono::steady_clock::now();
    crawl(cfg, visited, stats);
    auto elapsed = std::chrono::steady_clock::now() - t0;

    write_output(visited, "results.csv", elapsed, stats);
    return 0;
}