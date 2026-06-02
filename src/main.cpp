#include "crawler.hpp"
#include "http_fetcher.hpp"
#include "output.hpp"
#include "url_filter.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>

// This would effectively fetch a 1 MB file to estimate available bandwidth .

static double probe_bandwidth() {

  HttpFetcher f;

  auto t0 = std::chrono::steady_clock::now();

  std::string body =
      f.fetch("https://speed.cloudflare.com/__down?bytes=1048576");

  double secs =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - t0)
          .count();

  if (body.empty() || secs <= 0) {

    return 5e6; 
  }

  return body.size() / secs;
}


// Usage as follows:  ./crawler <seed_url> [num_threads] [max_depth]

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0]
              << " <seed_url> [num_threads] [max_depth]\n";

    return 1;
  }

  CrawlConfig cfg;

  cfg.seed_url = argv[1];
  cfg.num_threads = (argc > 2) ? std::stoi(argv[2]) : 1;
  cfg.max_depth = (argc > 3) ? std::stoi(argv[3]) : 3;

  cfg.num_threads = std::max(1, cfg.num_threads);
  cfg.max_depth = std::max(0, cfg.max_depth);

  // We'll filter the exterior domain links
  cfg.filter = domain_filter(url_host(cfg.seed_url));

  // Bandwidth-based thread heuristic

  // T_opt = bandwidth * avg_latency / avg_page_size

  // avg_latency = 0.25s, avg Wikipedia page =  200 KB

  double bw = probe_bandwidth();

  int recommended = std::max(1, (int)(bw * 0.25 / 200000.0));

  std::cout << "Bandwidth probe : " << bw / 1e6 << " MB/s\n"
            << "Recommended threads: " << recommended << "\n";

  std::cout << "Threads : " << cfg.num_threads << "\n"
            << "MaxDepth: " << cfg.max_depth << "\n\n";

  StripedHashSet visited;
  CrawlStats stats;

  auto t0 = std::chrono::steady_clock::now();
  crawl(cfg, visited, stats);
  auto elapsed = std::chrono::steady_clock::now() - t0;

  write_output(visited, "results.csv", elapsed, stats);

  // Print shortest path to the most-linked page discovered
  auto pages = visited.get_all();
  
  if (!pages.empty()) {
    auto top = std::max_element(pages.begin(), pages.end(),
                                [](const PageInfo &a, const PageInfo &b) {
                                  return a.in_link_count < b.in_link_count;
                                });

    auto path = shortest_path(visited, top->url);
    std::cout << "\nShortest path to most-linked page:\n";
    for (size_t i = 0; i < path.size(); i++)
      std::cout << "  [" << i << "] " << path[i] << "\n";
  }

  return 0;
}