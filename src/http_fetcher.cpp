#include "http_fetcher.hpp"

namespace {

// libcurl global setup. The local static variable makes this run only once.
class CurlGlobalSetup {
public:
    CurlGlobalSetup() {
        code = curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~CurlGlobalSetup() {
        if (code == CURLE_OK) {
            curl_global_cleanup();
        }
    }

    CURLcode code;
};

bool prepare_curl() {
    static CurlGlobalSetup setup;
    return setup.code == CURLE_OK;
}

}

HttpFetcher::HttpFetcher()
    : curl_(nullptr),
      last_status_code_(0),
      last_error_("") {
    error_buffer_[0] = '\0';

    if (!prepare_curl()) {
        last_error_ = "curl_global_init failed";
        return;
    }

    curl_ = curl_easy_init();
    if (curl_ == nullptr) {
        last_error_ = "curl_easy_init failed";
        return;
    }

    // Options that stay the same for all requests made by this object.
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, "CSE305-ParallelCrawler/1.0");
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &HttpFetcher::write_callback);
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_, CURLOPT_ACCEPT_ENCODING, "");
}

HttpFetcher::~HttpFetcher() {
    if (curl_ != nullptr) {
        curl_easy_cleanup(curl_);
    }
}

std::string HttpFetcher::fetch(const std::string& url) {
    std::string body;
    last_status_code_ = 0;
    last_error_.clear();
    error_buffer_[0] = '\0';

    if (curl_ == nullptr) {
        last_error_ = "HttpFetcher is not ready";
        return body;
    }

    if (!starts_with_http(url)) {
        last_error_ = "URL must start with http:// or https://";
        return body;
    }

    // Options that change for each request.
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl_, CURLOPT_ERRORBUFFER, error_buffer_);

    CURLcode result = curl_easy_perform(curl_);
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &last_status_code_);

    if (result != CURLE_OK) {
        if (error_buffer_[0] != '\0') {
            last_error_ = error_buffer_;
        } else {
            last_error_ = curl_easy_strerror(result);
        }
        return "";
    }

    if (last_status_code_ >= 400) {
        last_error_ = "HTTP error " + std::to_string(last_status_code_);
        return "";
    }

    return body;
}

long HttpFetcher::last_status_code() const {
    return last_status_code_;
}

std::string HttpFetcher::last_error() const {
    return last_error_;
}

bool HttpFetcher::is_ready() const {
    return curl_ != nullptr;
}

bool HttpFetcher::starts_with_http(const std::string& url) {
    return url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0;
}

size_t HttpFetcher::write_callback(char* data, size_t size, size_t nmemb, void* user_data) {
    size_t real_size = size * nmemb;
    std::string* body = static_cast<std::string*>(user_data);
    body->append(data, real_size);
    return real_size;
}

