#pragma once

// Crawler engine
//
// Spawns N worker threads. Each worker:
//   1. Pops a URL from the BFS queue
//   2. Fetches the page (libcurl)
//   3. Extracts & filters links
//   4. Inserts new links into visited set + queue
//
// Termination: queue empty AND active_workers == 0
//
// Key operations:
//   crawl(seed_url, num_threads, max_depth, filter) → results
//
