#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <utility>

struct CrawlTask {
    std::string url;
    int         depth;        // BFS depth from seed
    std::string parent_url;
};

// Thread-safe unbounded FIFO queue for BFS workers.
// try_pop blocks with a timeout so workers can periodically check for
// termination (empty queue + no active workers  then the caller calls shutdown()).
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
        cv_.notify_one(); // wake one blocked worker
    }

    // Returns true and fills `task` if an item is available within timeout_ms.
    // Returns false on timeout or after shutdown() with an empty queue.
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

    // Wake all blocked try_pop calls so threads can exit cleanly.
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
    bool                    shutdown_flag_;
};
