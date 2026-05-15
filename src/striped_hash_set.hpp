#pragma once

// Concurrent hash set with lock striping (Herlihy Ch.13)
// Uses: std::vector<std::mutex> for stripe locks
//
// Key operations:
//   insert_if_absent(url, info)   — atomic check-and-insert, returns true if new
//   contains(url)                 — check membership
//   increment_inlinks(url)        — atomically bump in-link counter
//   get_all()                     — snapshot of all entries for output
//
// Design:
//   - N buckets, each a std::list<PageInfo>
//   - L locks (L < N), lock[i] protects buckets {i, i+L, i+2L, ...}
//   - Resize when load factor exceeds threshold (acquire ALL locks)
//
