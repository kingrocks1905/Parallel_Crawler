// Benchmark: throughput vs thread count
// Runs the same depth=1 crawl at 1,2,4,8,16,32 threads and prints a CSV table.
// Usage: ./build/benchmarks [seed_url]

#include "crawler.hpp"
#include "url_filter.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    const std::string seed = (argc > 1)
        ? argv[1]
        : "https://en.wikipedia.org/wiki/Concurrent_computing";

    const int depth = 1;  

    const std::vector<int> thread_counts = {1, 2, 4, 8, 16, 32};

    std::cout << "threads,elapsed_s,pages_fetched,throughput_pps,fetch_errors,"

              << "fetch_pct,parse_pct,sync_pct\n";

    for (int t : thread_counts) {

        CrawlConfig cfg;

        cfg.seed_url    = seed;
        cfg.num_threads = t;
        cfg.max_depth   = depth;
        cfg.filter      = domain_filter(url_host(seed));

        StripedHashSet visited;
        CrawlStats     stats;

        auto t0 = std::chrono::steady_clock::now();

        crawl(cfg, visited, stats);

        double secs = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count();

        double throughput = secs > 0 ? stats.pages_fetched.load() / secs : 0;

        long long fn = stats.fetch_ns.load();

        long long pn = stats.parse_ns.load();
        long long sn = stats.sync_ns.load();
        
        long long tn = fn + pn + sn;
        double fetch_pct = tn > 0 ? 100.0 * fn / tn : 0;
        double parse_pct = tn > 0 ? 100.0 * pn / tn : 0;
        double sync_pct  = tn > 0 ? 100.0 * sn / tn : 0;

        std::cout << t << ","
                  << secs << ","
                  << stats.pages_fetched.load() << ","
                  << throughput << ","
                  << stats.fetch_errors.load() << ","
                  << fetch_pct << ","
                  << parse_pct << ","
                  << sync_pct << "\n";

        std::cerr << "[done] threads=" << t
                  << "  throughput=" << throughput << " pages/s"
                  << "  errors="     << stats.fetch_errors.load() << "\n";
    }

    return 0;
}
