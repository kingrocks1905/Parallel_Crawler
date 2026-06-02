cmake_minimum_required(VERSION 3.14)
project(ParallelCrawler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Main executable
add_executable(crawler
    src/main.cpp
    src/http_fetcher.cpp
    src/link_extractor.cpp
    src/crawler.cpp
    src/output.cpp
)

target_include_directories(crawler PRIVATE src)
target_link_libraries(crawler PRIVATE CURL::libcurl Threads::Threads)


# Benchmark executable
add_executable(benchmarks
    benchmarks/bench_threads.cpp
    src/crawler.cpp
    src/http_fetcher.cpp
    src/link_extractor.cpp
    src/output.cpp
)

target_include_directories(benchmarks PRIVATE src)
target_link_libraries(benchmarks PRIVATE CURL::libcurl Threads::Threads)

# Smoke test: fetch one page and print extracted links
add_executable(fetch_test
    tools/fetch_test.cpp
    src/http_fetcher.cpp
    src/link_extractor.cpp
)
target_include_directories(fetch_test PRIVATE src)
target_link_libraries(fetch_test PRIVATE CURL::libcurl Threads::Threads)
