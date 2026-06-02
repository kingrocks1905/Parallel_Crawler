# CSE305 Project: Parallel Web Crawler

A multithreaded web crawler in C++ that does BFS traversal of websites and builds a page index with depth ranking, in-link counts, and shortest paths.

## what you need

you'll need a C++17 compiler (clang++ or g++), CMake 3.14+, and libcurl with SSL support.

on macOS libcurl is usually there by default. on Ubuntu/Debian you can get it with:
```bash
sudo apt install libcurl4-openssl-dev
```

## building

```bash
cmake -S . -B build
cmake --build build
```

this builds a few things in `build/`:
`crawler` : the main crawler program
`benchmarks` : runs the thread-scaling benchmark
`fetch_test` : a small tool to test fetching a single URL

## usage

### running the crawler

```bash
./build/crawler <seed_url> [num_threads] [max_depth]
```

arguments:
`seed_url` is the starting URL (required)
`num_threads` is how many worker threads to use (default 1)
`max_depth` is the max BFS depth from the seed (default 3)

examples:
```bash
# 8 threads, depth 2, crawling wikipedia
./build/crawler "https://en.wikipedia.org/wiki/Concurrent_computing" 8 2

# single-threaded, default depth
./build/crawler "https://en.wikipedia.org/wiki/Web_crawler"
```

what happens when you run it:
1. probes your bandwidth and prints a recommended thread count
2. runs the BFS crawl
3. saves everything to `results.csv`
4. prints a quick summary: pages found, throughput, time breakdown
5. prints the shortest path to the most-linked page it found

### benchmarks

sweeps through 1, 2, 4, 8, 16, 32 threads on a depth-1 crawl and outputs CSV data:

```bash
./build/benchmarks > benchmark_results.csv
```

### fetch test

downloads a single URL and prints the body. good for checking if libcurl works on your machine:

```bash
./build/fetch_test "https://en.wikipedia.org/wiki/Main_Page"
```

## project structure

```
src/
├── main.cpp                  # entry point, bandwidth probe, shortest-path demo
├── crawler.hpp / .cpp        # BFS engine, thread pool, termination detection
├── safe_queue.hpp            # thread-safe FIFO queue
├── striped_hash_set.hpp      # concurrent hash set with lock striping
├── page_info.hpp             # per-URL metadata
├── http_fetcher.hpp / .cpp   # libcurl wrapper (one handle per thread)
├── link_extractor.hpp / .cpp # regex-based href extraction + URL resolution
├── url_filter.hpp            # domain/prefix filters
└── output.hpp / .cpp         # CSV writer + stats + shortest-path stuff

benchmarks/
└── bench_threads.cpp         # thread-count sweep

tools/
└── fetch_test.cpp            # single-URL fetch utility
```

## output

the crawler writes `results.csv` with one row per discovered page:

```
url,depth,in_links,parent_url
"https://en.wikipedia.org/wiki/Concurrent_computing",0,0,""
"https://en.wikipedia.org/wiki/Thread_(computing)",1,3,"https://en.wikipedia.org/wiki/Concurrent_computing"
...
```

pages are sorted by depth ascending, then by in-link count descending.

## authors

Kingshuk Gupta
Ziji Wang
