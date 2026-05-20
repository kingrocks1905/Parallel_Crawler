#pragma once

#include "page_info.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <vector>

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


// Concurrent hash set used for the crawler's visited URLs.
class StripedHashSet {
public:
    // Input: number of buckets and number of locks.
    // Output: an empty concurrent hash set.
    StripedHashSet(std::size_t bucket_count = 64, std::size_t stripe_count = 16)
        : buckets_(fixed_bucket_count(bucket_count, stripe_count)),
          locks_(fixed_stripe_count(stripe_count)),
          count_(0),
          bucket_count_(fixed_bucket_count(bucket_count, stripe_count)) {
    }

    // Input: url and its PageInfo.
    // Output: true if inserted, false if the url was already present.
    bool insert_if_absent(const std::string& url, const PageInfo& info) {
        std::size_t h = hash_url(url);
        std::size_t stripe = h % locks_.size();
        bool inserted = false;

        {
            std::lock_guard<std::mutex> guard(locks_[stripe]);
            std::list<PageInfo>& bucket = buckets_[h % buckets_.size()];

            for (const PageInfo& page : bucket) {
                if (page.url == url) {
                    return false;
                }
            }

            PageInfo copy = info;
            copy.url = url;
            bucket.push_back(copy);
            count_++;
            inserted = true;
        }

        if (inserted) {
            resize_if_needed();
        }

        return true;
    }

    // Input: url.
    // Output: true if the url is already in the set.
    bool contains(const std::string& url) const {
        std::size_t h = hash_url(url);
        std::size_t stripe = h % locks_.size();

        std::lock_guard<std::mutex> guard(locks_[stripe]);
        const std::list<PageInfo>& bucket = buckets_[h % buckets_.size()];

        for (const PageInfo& page : bucket) {
            if (page.url == url) {
                return true;
            }
        }
        return false;
    }

    // Input: url and output variable.
    // Output: true if found, and fills result with the stored PageInfo.
    bool get(const std::string& url, PageInfo& result) const {
        std::size_t h = hash_url(url);
        std::size_t stripe = h % locks_.size();

        std::lock_guard<std::mutex> guard(locks_[stripe]);
        const std::list<PageInfo>& bucket = buckets_[h % buckets_.size()];

        for (const PageInfo& page : bucket) {
            if (page.url == url) {
                result = page;
                return true;
            }
        }
        return false;
    }

    // Input: url.
    // Output: true if the counter was incremented.
    bool increment_inlinks(const std::string& url) {
        std::size_t h = hash_url(url);
        std::size_t stripe = h % locks_.size();

        std::lock_guard<std::mutex> guard(locks_[stripe]);
        std::list<PageInfo>& bucket = buckets_[h % buckets_.size()];

        for (PageInfo& page : bucket) {
            if (page.url == url) {
                page.in_link_count++;
                return true;
            }
        }
        return false;
    }

    // Output: a snapshot of all stored pages.
    std::vector<PageInfo> get_all() const {
        lock_all_stripes();

        std::vector<PageInfo> result;
        for (const std::list<PageInfo>& bucket : buckets_) {
            for (const PageInfo& page : bucket) {
                result.push_back(page);
            }
        }

        unlock_all_stripes();
        return result;
    }

    std::size_t size() const {
        return count_.load();
    }

    bool empty() const {
        return size() == 0;
    }

private:
    std::vector<std::list<PageInfo>> buckets_;
    mutable std::vector<std::mutex> locks_;
    mutable std::mutex resize_mutex_;

    std::atomic<std::size_t> count_;
    std::atomic<std::size_t> bucket_count_;

    static const std::size_t MAX_LOAD = 4;

    static std::size_t fixed_stripe_count(std::size_t stripe_count) {
        if (stripe_count == 0) {
            return 1;
        }
        return stripe_count;
    }

    static std::size_t fixed_bucket_count(std::size_t bucket_count,
                                          std::size_t stripe_count) {
        stripe_count = fixed_stripe_count(stripe_count);

        if (bucket_count < stripe_count) {
            bucket_count = stripe_count;
        }

        std::size_t remainder = bucket_count % stripe_count;
        if (remainder != 0) {
            bucket_count += stripe_count - remainder;
        }

        return bucket_count;
    }

    static std::size_t hash_url(const std::string& url) {
        return std::hash<std::string>{}(url);
    }

    // Resize is rare. It locks every stripe before replacing the bucket table.
    void resize_if_needed() {
        if (count_.load() <= bucket_count_.load() * MAX_LOAD) {
            return;
        }

        std::lock_guard<std::mutex> resize_guard(resize_mutex_);

        if (count_.load() <= bucket_count_.load() * MAX_LOAD) {
            return;
        }

        lock_all_stripes();

        std::size_t new_count = buckets_.size() * 2;
        std::vector<std::list<PageInfo>> new_buckets(new_count);

        for (const std::list<PageInfo>& bucket : buckets_) {
            for (const PageInfo& page : bucket) {
                std::size_t h = hash_url(page.url);
                new_buckets[h % new_count].push_back(page);
            }
        }

        buckets_ = new_buckets;
        bucket_count_ = buckets_.size();

        unlock_all_stripes();
    }

    void lock_all_stripes() const {
        for (std::size_t i = 0; i < locks_.size(); i++) {
            locks_[i].lock();
        }
    }

    void unlock_all_stripes() const {
        for (std::size_t i = locks_.size(); i > 0; i--) {
            locks_[i - 1].unlock();
        }
    }
};