#include "output.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>

void write_output(const StripedHashSet& visited,
                  const std::string&    filename,
                  std::chrono::duration<double> elapsed) {
    auto pages = visited.get_all();

    // Sort by depth then in_link_count descending (most linked first per level)
    std::sort(pages.begin(), pages.end(), [](const PageInfo& a, const PageInfo& b) {
        if (a.depth != b.depth) return a.depth < b.depth;
        return a.in_link_count > b.in_link_count;
    });

    // Write CSV
    std::ofstream out(filename);
    out << "url,depth,in_links,parent_url\n";
    for (const PageInfo& p : pages) {
        out << '"' << p.url        << '"' << ','
            <<        p.depth      << ','
            <<        p.in_link_count << ','
            << '"' << p.parent_url << '"' << '\n';
    }

    double secs = elapsed.count();

    std::cout << "\n── Crawl summary ──────────────────────────\n"
              << "  Pages crawled : " << pages.size() << "\n"
              << "  Elapsed       : " << std::fixed << std::setprecision(2) << secs << " s\n"
              << "  Throughput    : " << std::setprecision(1) << pages.size() / secs << " pages/s\n"
              << "  Output file   : " << filename << "\n"
              << "───────────────────────────────────────────\n";
}
