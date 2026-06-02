#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <utility>

struct CrawlTask {
    std::string url;
    int         depth;
    std::string parent_url;
};

// thread-safe FIFO queue for the crawl frontier
// workers block on try_pop until something shows up or timeout expires
class SafeBFSQueue {
public:
    SafeBFSQueue() : shutdown_flag_(false) {}

    SafeBFSQueue(const SafeBFSQueue &)            = delete;
    SafeBFSQueue &operator=(const SafeBFSQueue &) = delete;
    SafeBFSQueue(SafeBFSQueue &&)                 = delete;
    SafeBFSQueue &operator=(SafeBFSQueue &&)      = delete;

    void push(CrawlTask task) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push(std::move(task));
        }
        cv_.notify_one();
    }

    // tries to grab a task within timeout_ms, returns false if nothing came through
    bool try_pop(CrawlTask &task, int timeout_ms) {
        std::unique_lock<std::mutex> lock(mtx_);

        bool got_item = cv_.wait_for(lock,
            std::chrono::milliseconds(timeout_ms),
            [this] { return !queue_.empty() || shutdown_flag_; });

        if (!got_item || queue_.empty())
            return false;

        task = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

    // unblocks all threads so they can check the termination flag and exit
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            shutdown_flag_ = true;
        }
        cv_.notify_all();
    }

    bool is_shutdown() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return shutdown_flag_;
    }

private:
    std::queue<CrawlTask>   queue_;
    mutable std::mutex      mtx_;
    std::condition_variable cv_;
    bool shutdown_flag_;
};
