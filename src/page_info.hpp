#pragma once

#include <string>

// Metadata stored for each discovered URL
struct PageInfo {
    std::string url;
    int depth;              // BFS depth from seed (= ranking)
    int in_link_count;      // how many pages link TO this page
    std::string parent_url; // who discovered this page first (for shortest path)
};
