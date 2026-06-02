#pragma once

#include <curl/curl.h>

#include <cstddef>
#include <string>
// wraps a libcurl handle for downloading pages
// each worker thread should have its own instance since curl handles aren't thread-safe


class HttpFetcher {
public:
    HttpFetcher();
    ~HttpFetcher();

    HttpFetcher(const HttpFetcher& other) = delete;
    HttpFetcher& operator=(const HttpFetcher& other) = delete;

    // returns the page body, or empty string on failure
    std::string fetch(const std::string& url);

    long last_status_code() const;
    std::string last_error() const;
    bool is_ready() const;

private:
    CURL* curl_;
    long last_status_code_;
    std::string last_error_;
    char error_buffer_[CURL_ERROR_SIZE];

    static bool starts_with_http(const std::string& url);
    static size_t write_callback(char* data, size_t size, size_t nmemb, void* user_data);
};