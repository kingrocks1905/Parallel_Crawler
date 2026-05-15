#pragma once

// HTML link extractor and URL normalizer
//
// Given raw HTML and a base URL, extracts all <a href="..."> links
// and normalizes them (resolve relative paths, remove fragments, etc.)
//
// Key operations:
//   extract_links(html, base_url) → std::vector<std::string>
//
