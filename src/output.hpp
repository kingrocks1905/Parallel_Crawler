#pragma once

#include "striped_hash_set.hpp"

#include <chrono>
#include <string>

// Writes all crawled pages to a CSV file (url, depth, in_links, parent_url)
// and prints a summary to stdout.
void write_output(const StripedHashSet& visited,
                  const std::string&    filename,
                  std::chrono::duration<double> elapsed);
