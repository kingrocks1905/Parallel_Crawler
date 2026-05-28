// Unit tests for SafeBFSQueue
// Tests: basic FIFO, concurrent push/pop, timeout, shutdown, FIFO ordering

#include "safe_queue.hpp"

#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <set>
#include <chrono>
#include <sstream>


#define TEST(name) \
    static void name(); \
    struct name##_reg { name##_reg() { tests().push_back({#name, name}); } } name##_inst; \
    static void name()

struct TestEntry { const char* name; void(*fn)(); };
static std::vector<TestEntry>& tests() {
    static std::vector<TestEntry> v;
    return v;
}


// 1. Push one, pop one 
TEST(test_push_pop_single) {
    SafeBFSQueue q;
    q.push({"https://example.com", 0, ""});

    CrawlTask task;
    bool ok = q.try_pop(task, 100);
    assert(ok);
    assert(task.url == "https://example.com");
    assert(task.depth == 0);
    assert(task.parent_url == "");
}

// 2. FIFO ordering is preserved.
TEST(test_fifo_ordering) {
    SafeBFSQueue q;
    for (int i = 0; i < 100; ++i) {
        q.push({"url_" + std::to_string(i), i, ""});
    }

    for (int i = 0; i < 100; ++i) {
        CrawlTask task;
        assert(q.try_pop(task, 100));
        assert(task.url == "url_" + std::to_string(i));
        assert(task.depth == i);
    }
    assert(q.empty());
}

// 3. try_pop returns false on empty queue after timeout.
TEST(test_timeout_on_empty) {
    SafeBFSQueue q;
    CrawlTask task;

    auto start = std::chrono::steady_clock::now();
    bool ok = q.try_pop(task, 50);  // 50 ms timeout
    auto elapsed = std::chrono::steady_clock::now() - start;

    assert(!ok);
    // Should have waited roughly 50 ms (allow generous margin)
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    assert(ms >= 40);  // at least ~40 ms
}

// 4. try_pop wakes up immediately when a push arrives.
TEST(test_pop_wakes_on_push) {
    SafeBFSQueue q;
    CrawlTask result;

    std::thread consumer([&] {
        // Will block up to 2 s, but should wake in ~50 ms
        q.try_pop(result, 2000);
    });

    // Give the consumer thread time to block
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto before = std::chrono::steady_clock::now();
    q.push({"woke", 1, ""});
    consumer.join();
    auto after = std::chrono::steady_clock::now();

    assert(result.url == "woke");
    // Join should complete well before the 2 s timeout
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
    assert(ms < 500);
}

// 5. shutdown() unblocks all waiting threads.
TEST(test_shutdown_unblocks) {
    SafeBFSQueue q;
    constexpr int N = 4;
    std::atomic<int> unblocked{0};

    std::vector<std::thread> workers;
    for (int i = 0; i < N; ++i) {
        workers.emplace_back([&] {
            CrawlTask task;
            q.try_pop(task, 5000);  // long timeout
            unblocked.fetch_add(1);
        });
    }

    // Let all workers block
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(unblocked.load() == 0);

    q.shutdown();

    for (auto& t : workers) t.join();
    assert(unblocked.load() == N);
}

// 6. After shutdown, remaining items are still poppable.
TEST(test_drain_after_shutdown) {
    SafeBFSQueue q;
    q.push({"a", 0, ""});
    q.push({"b", 1, ""});
    q.shutdown();

    CrawlTask task;
    assert(q.try_pop(task, 0));
    assert(task.url == "a");
    assert(q.try_pop(task, 0));
    assert(task.url == "b");
    // Now truly empty
    assert(!q.try_pop(task, 0));
}

// 7. Concurrent multi-producer / multi-consumer — no data loss.
TEST(test_concurrent_mpmc) {
    SafeBFSQueue q;
    constexpr int NUM_PRODUCERS = 4;
    constexpr int NUM_CONSUMERS = 4;
    constexpr int ITEMS_PER_PRODUCER = 1000;
    constexpr int TOTAL = NUM_PRODUCERS * ITEMS_PER_PRODUCER;

    // Producers: each pushes ITEMS_PER_PRODUCER items with a unique prefix.
    std::vector<std::thread> producers;
    for (int p = 0; p < NUM_PRODUCERS; ++p) {
        producers.emplace_back([&q, p] {
            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                std::ostringstream oss;
                oss << p << "_" << i;
                q.push({oss.str(), i, ""});
            }
        });
    }

    // Consumers: pop until they collectively gather TOTAL items.
    std::atomic<int> consumed{0};
    std::vector<std::vector<std::string>> per_consumer(NUM_CONSUMERS);
    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; ++c) {
        consumers.emplace_back([&q, &consumed, &per_consumer, c] {
            while (true) {
                CrawlTask task;
                if (q.try_pop(task, 50)) {
                    per_consumer[c].push_back(task.url);
                    consumed.fetch_add(1);
                } else if (consumed.load() >= NUM_PRODUCERS * ITEMS_PER_PRODUCER) {
                    break;
                }
            }
        });
    }

    for (auto& t : producers)  t.join();
    for (auto& t : consumers)  t.join();

    std::set<std::string> all;
    for (auto& v : per_consumer) {
        for (auto& s : v) {
            assert(all.insert(s).second);  
        }
    }
    assert(static_cast<int>(all.size()) == TOTAL);
}

TEST(test_size_and_empty) {
    SafeBFSQueue q;
    assert(q.empty());
    assert(q.size() == 0);

    q.push({"x", 0, ""});
    assert(!q.empty());
    assert(q.size() == 1);

    q.push({"y", 1, ""});
    assert(q.size() == 2);

    CrawlTask t;
    q.try_pop(t, 0);
    assert(q.size() == 1);
    q.try_pop(t, 0);
    assert(q.size() == 0);
    assert(q.empty());
}


int main() {
    int passed = 0;
    int failed = 0;
    for (auto& [name, fn] : tests()) {
        try {
            fn();
            std::cout << "  ✓ " << name << "\n";
            ++passed;
        } catch (const std::exception& e) {
            std::cout << "  ✗ " << name << " — " << e.what() << "\n";
            ++failed;
        } catch (...) {
            std::cout << "  ✗ " << name << " — assertion failed\n";
            ++failed;
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed.\n";
    return failed ? 1 : 0;
}
