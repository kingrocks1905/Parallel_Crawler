// Unit tests for StripedHashSet
// TODO: Implement
#include "striped_hash_set.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

static PageInfo make_page(const std::string& url, int depth, int links, const std::string& parent) {
    PageInfo info;
    info.url = url;
    info.depth = depth;
    info.in_link_count = links;
    info.parent_url = parent;
    return info;
}

static void test_empty_set() {
    StripedHashSet set;

    assert(set.empty());
    assert(set.size() == 0);
    assert(!set.contains("https://example.com"));

    PageInfo result;
    assert(!set.get("https://example.com", result));
    assert(!set.increment_inlinks("https://example.com"));
}

static void test_insert_contains_get() {
    StripedHashSet set;

    PageInfo info = make_page("https://example.com", 0, 1, "");
    assert(set.insert_if_absent(info.url, info));

    assert(!set.empty());
    assert(set.size() == 1);
    assert(set.contains("https://example.com"));
    assert(!set.contains("https://example.com/missing"));

    PageInfo result;
    assert(set.get("https://example.com", result));
    assert(result.url == "https://example.com");
    assert(result.depth == 0);
    assert(result.in_link_count == 1);
    assert(result.parent_url == "");
}

static void test_duplicate_insert_does_not_replace() {
    StripedHashSet set;

    PageInfo first = make_page("https://example.com/page", 1, 1, "seed");
    PageInfo second = make_page("https://example.com/page", 9, 99, "wrong-parent");

    assert(set.insert_if_absent(first.url, first));
    assert(!set.insert_if_absent(second.url, second));
    assert(set.size() == 1);

    PageInfo result;
    assert(set.get(first.url, result));
    assert(result.depth == 1);
    assert(result.in_link_count == 1);
    assert(result.parent_url == "seed");
}

static void test_increment_inlinks() {
    StripedHashSet set;

    std::string url = "https://example.com/a";
    PageInfo info = make_page(url, 1, 1, "https://example.com");

    assert(set.insert_if_absent(url, info));
    assert(set.increment_inlinks(url));
    assert(set.increment_inlinks(url));
    assert(!set.increment_inlinks("https://example.com/not-found"));

    PageInfo result;
    assert(set.get(url, result));
    assert(result.in_link_count == 3);
}

static void test_get_all() {
    StripedHashSet set;

    assert(set.insert_if_absent("https://example.com/1", make_page("https://example.com/1", 1, 1, "root")));
    assert(set.insert_if_absent("https://example.com/2", make_page("https://example.com/2", 1, 1, "root")));
    assert(set.insert_if_absent("https://example.com/3", make_page("https://example.com/3", 2, 1, "https://example.com/1")));

    std::vector<PageInfo> pages = set.get_all();
    assert(pages.size() == 3);

    bool found_one = false;
    bool found_two = false;
    bool found_three = false;

    for (const PageInfo& page : pages) {
        if (page.url == "https://example.com/1") {
            found_one = true;
        }
        if (page.url == "https://example.com/2") {
            found_two = true;
        }
        if (page.url == "https://example.com/3") {
            found_three = true;
        }
    }

    assert(found_one);
    assert(found_two);
    assert(found_three);
}

static void test_resize_keeps_all_urls() {
    StripedHashSet set(2, 2);
    const int total = 200;

    for (int i = 0; i < total; i++) {
        std::string url = "https://example.com/page" + std::to_string(i);
        assert(set.insert_if_absent(url, make_page(url, i % 4, 1, "root")));
    }

    assert(set.size() == static_cast<std::size_t>(total));

    for (int i = 0; i < total; i++) {
        std::string url = "https://example.com/page" + std::to_string(i);
        assert(set.contains(url));
    }
}

static void test_concurrent_unique_insertions() {
    StripedHashSet set(8, 4);
    const int thread_count = 8;
    const int per_thread = 500;

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; t++) {
        threads.emplace_back([&set, t]() {
            for (int i = 0; i < per_thread; i++) {
                std::string url = "https://site.com/t" + std::to_string(t) + "/page" + std::to_string(i);
                PageInfo info = make_page(url, 1, 1, "seed");
                assert(set.insert_if_absent(url, info));
            }
        });
    }

    for (std::thread& th : threads) {
        th.join();
    }

    assert(set.size() == static_cast<std::size_t>(thread_count * per_thread));

    for (int t = 0; t < thread_count; t++) {
        for (int i = 0; i < per_thread; i++) {
            std::string url = "https://site.com/t" + std::to_string(t) + "/page" + std::to_string(i);
            assert(set.contains(url));
        }
    }
}

static void test_concurrent_duplicate_insertions() {
    StripedHashSet set(8, 4);
    const int thread_count = 8;
    const int unique_urls = 300;

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; t++) {
        threads.emplace_back([&set]() {
            for (int i = 0; i < unique_urls; i++) {
                std::string url = "https://shared.com/page" + std::to_string(i);
                PageInfo info = make_page(url, 1, 1, "seed");

                if (!set.insert_if_absent(url, info)) {
                    set.increment_inlinks(url);
                }
            }
        });
    }

    for (std::thread& th : threads) {
        th.join();
    }

    assert(set.size() == static_cast<std::size_t>(unique_urls));

    for (int i = 0; i < unique_urls; i++) {
        std::string url = "https://shared.com/page" + std::to_string(i);
        PageInfo result;
        assert(set.get(url, result));
        assert(result.in_link_count == thread_count);
    }
}

int main() {
    test_empty_set();
    test_insert_contains_get();
    test_duplicate_insert_does_not_replace();
    test_increment_inlinks();
    test_get_all();
    test_resize_keeps_all_urls();
    test_concurrent_unique_insertions();
    test_concurrent_duplicate_insertions();

    std::cout << "All StripedHashSet tests passed." << std::endl;
    return 0;
}