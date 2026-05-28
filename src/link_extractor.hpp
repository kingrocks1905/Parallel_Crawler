#pragma once

#include <string>
#include <vector>

// Given raw HTML and the URL it came from, this function returns all absolute URLs

std::vector<std::string> extract_links(const std::string& html,
                                       const std::string& base_url);
