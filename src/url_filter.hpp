#pragma once

// URL filtering functions to control crawl scope
//
// Filters are std::function<bool(const std::string&)>
// Return true if the URL should be followed, false to skip.
//
// Planned filters:
//   domain_filter(base_domain)    — only same-domain URLs
//   prefix_filter(prefix)         — only URLs starting with prefix
//
