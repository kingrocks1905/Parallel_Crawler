#pragma once

// HTTP page fetcher using libcurl
//
// Each worker thread creates its own instance (thread-local curl handle).
// Handles: HTTPS, redirects, timeouts, User-Agent.
//
// Key operations:
//   fetch(url) → std::string (HTML body)
//

