#pragma once

#include "crawler.hpp"
#include "striped_hash_set.hpp"

#include <chrono>
#include <string>

// Writes crawled pages to a CSV file and prints a short summary.
void write_output(const StripedHashSet& visited,
                  const std::string& filename,
                  std::chrono::duration<double> elapsed,
                  const CrawlStats& stats);