# CSE305 Project: Parallel Web Crawler

A multithreaded web crawler in C++ that performs BFS traversal of websites, building a page index with ranking and shortest-path data.

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
./crawler <seed_url> [num_threads] [max_depth]
```

## Project Structure

```
src/
├── main.cpp              # CLI entry point
├── crawler.hpp / .cpp    # Crawler engine (thread pool + orchestration)
├── safe_queue.hpp        # Thread-safe BFS queue (mutex + CV)
├── striped_hash_set.hpp  # Concurrent hash set (lock striping)
├── http_fetcher.hpp/.cpp # libcurl wrapper
├── link_extractor.hpp/.cpp # HTML parsing + URL normalization
├── url_filter.hpp        # Crawl scope filtering
├── page_info.hpp         # Per-URL metadata struct
└── output.hpp / .cpp     # Index output + stats
tests/                    # Unit tests
benchmarks/               # Thread scaling benchmarks
```


