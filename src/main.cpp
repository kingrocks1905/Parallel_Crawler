#include "crawler.hpp"
#include "output.hpp"
#include "url_filter.hpp"

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
    cfg.seed_url    = argv[1];
    
    cfg.num_threads = (argc > 2) ? std::stoi(argv[2]) : 1;
    cfg.max_depth   = (argc > 3) ? std::stoi(argv[3]) : 3;

    // We'll filter the exterior domain links
    cfg.filter = domain_filter(url_host(cfg.seed_url));

    std::cout << "Threads : " << cfg.num_threads << "\n"
              << "MaxDepth: " << cfg.max_depth   << "\n\n";

    auto t0 = std::chrono::steady_clock::now();
    StripedHashSet visited;
    crawl(cfg, visited);
    auto elapsed = std::chrono::steady_clock::now() - t0;

    write_output(visited, "results.csv", elapsed);
    return 0;
}
