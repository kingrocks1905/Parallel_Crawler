#pragma once

#include "crawler.hpp"
#include "striped_hash_set.hpp"

#include <chrono>
#include <string>

// we write the crawled pages to a CSV file.
void write_output(const StripedHashSet& visited,
                  const std::string& filename,
                  std::chrono::duration<double> elapsed,
                  const CrawlStats& stats);


// Returns the shortest path from the seed to 'url' by walking parent_url links.
// The returned vector starts at the seed and ends at 'url'.

std::vector<std::string> shortest_path(const StripedHashSet& visited,
                                        const std::string& url);