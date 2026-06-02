#include "output.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

void write_output(const StripedHashSet& visited,
                  const std::string& filename,
                  std::chrono::duration<double> elapsed,
                  const CrawlStats& stats) {
    std::vector<PageInfo> pages = visited.get_all();

    // sort by depth first, then by in-link count
    std::sort(pages.begin(), pages.end(), [](const PageInfo& a, const PageInfo& b) {

        if (a.depth != b.depth) {

            return a.depth < b.depth;
        }
        return a.in_link_count > b.in_link_count;
    });

    std::ofstream out(filename);
    out << "url,depth,in_links,parent_url\n";

    for (const PageInfo& p : pages) {
        out << '"' << p.url << '"' << ','
            << p.depth << ','
            << p.in_link_count << ','
            << '"' << p.parent_url << '"' << '\n';
    }

    double secs = elapsed.count();
    double throughput = 0.0;
    if (secs > 0.0) {
        throughput = static_cast<double>(stats.pages_fetched.load()) / secs;
    }

    std::cout << "\n Crawl summary \n"
    
              << "  Pages discovered : " << pages.size() << "\n"

              << "  Pages fetched    : " << stats.pages_fetched.load() << "\n"
              << "  Fetch errors     : " << stats.fetch_errors.load() << "\n"
              << "  Tasks processed  : " << stats.tasks_processed.load() << "\n"
              << "  Links extracted  : " << stats.links_extracted.load() << "\n"
              << "  Links accepted   : " << stats.links_accepted.load() << "\n"
              << "  New pages        : " << stats.new_pages.load() << "\n"
              << "  Duplicate links  : " << stats.duplicate_links.load() << "\n"
              << "  Elapsed          : " << std::fixed << std::setprecision(2)
              << secs << " s\n"
              << "  Throughput       : " << std::setprecision(2)
              << throughput << " fetched pages/s\n"
              << "  Output file      : " << filename << "\n";

    // time spent in each phase (summed across threads)
    long long fetch = stats.fetch_ns.load();
    long long parse = stats.parse_ns.load();
    long long sync  = stats.sync_ns.load();
    long long total = fetch + parse + sync;
    if (total > 0) {
        auto pct = [&](long long v) { return 100.0 * v / total; };
        std::cout << "\n Time breakdown (across threads)\n"
                  << std::fixed << std::setprecision(2)
                  << "  Fetch : " << fetch / 1e9 << " s  (" << pct(fetch) << "%)\n"
                  << "  Parse : " << parse / 1e9 << " s  (" << pct(parse) << "%)\n"
                  << "  Sync  : " << sync  / 1e9 << " s  (" << pct(sync)  << "%)\n";
    }

    std::cout << "\n";
}

std::vector<std::string> shortest_path(const StripedHashSet& visited,
                                        const std::string& url) {
    std::vector<std::string> path;
    std::string current = url;

    // follow parent_url back to the seed
    while (!current.empty()) {
        path.push_back(current);
        PageInfo info;
        if (!visited.get(current, info)) break;
        current = info.parent_url;
    }

    std::reverse(path.begin(), path.end());
    return path;
}