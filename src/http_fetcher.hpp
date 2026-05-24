#pragma once

#include <curl/curl.h>

#include <cstddef>
#include <string>
// HTTP page fetcher using libcurl
//
// Each worker thread creates its own instance (thread-local curl handle).
// Handles: HTTPS, redirects, timeouts, User-Agent.
//
// Key operations:
//   fetch(url) → std::string (HTML body)
//


class HttpFetcher {
public:
    HttpFetcher();
    ~HttpFetcher();

    HttpFetcher(const HttpFetcher& other) = delete;
    HttpFetcher& operator=(const HttpFetcher& other) = delete;

    // Input: a URL starting with http:// or https://
    // Output: the HTML/page body, or an empty string if the request failed.
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