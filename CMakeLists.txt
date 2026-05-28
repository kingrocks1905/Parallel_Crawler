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

# Test executables — one per file (each defines its own main())
add_executable(test_safe_queue   tests/test_safe_queue.cpp)
add_executable(test_striped_hash tests/test_striped_hash.cpp)
add_executable(test_link_extractor tests/test_link_extractor.cpp)

foreach(t test_safe_queue test_striped_hash test_link_extractor)
    target_include_directories(${t} PRIVATE src)
    target_link_libraries(${t} PRIVATE CURL::libcurl Threads::Threads)
endforeach()

# Benchmark executable
add_executable(benchmarks
    benchmarks/bench_threads.cpp
)

target_include_directories(benchmarks PRIVATE src)
target_link_libraries(benchmarks PRIVATE CURL::libcurl Threads::Threads)
