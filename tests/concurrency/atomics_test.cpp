// Checks claims made by docs/examples/concurrency/atomics.md.
#include <atomic>
#include <latch>
#include <memory>
#include <thread>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Mental model": fetch_add returns the old value; ++, load, store.
TEST(Atomics, BasicOps) {
    std::atomic<int> counter{0};
    EXPECT_EQ(counter.fetch_add(1), 0);
    ++counter;
    EXPECT_EQ(counter.load(), 2);
    counter.store(5);
    EXPECT_EQ(counter, 5);
}

// "Mental model": increments from many threads aren't lost.
TEST(Atomics, NoLostUpdates) {
    std::atomic<int> counter{0};
    std::latch go{1};
    std::vector<std::thread> ts;
    for (int i = 0; i < 4; ++i)
        ts.emplace_back([&] {
            go.wait();
            for (int j = 0; j < 10000; ++j) counter.fetch_add(1);
        });
    go.count_down();
    for (auto& t : ts) t.join();
    EXPECT_EQ(counter, 40000);
}

// "Read-modify-write & CAS": the arithmetic results and CAS semantics.
TEST(Atomics, RmwAndCas) {
    std::atomic<int> x{0};
    x.fetch_add(3);
    x.fetch_sub(1);
    x.fetch_or(0b10);
    EXPECT_EQ(x, 2);
    int expected = 0;
    EXPECT_FALSE(x.compare_exchange_strong(expected, 1));  // x != 0
    EXPECT_EQ(expected, 2);                                  // loaded into expected
    EXPECT_TRUE(x.compare_exchange_strong(expected, 7));
    EXPECT_EQ(x, 7);
    while (!x.compare_exchange_weak(expected, expected + 1)) {}
    EXPECT_EQ(x, 8);
}

// "Memory order": a release store publishes the plain write to an acquire load.
TEST(Atomics, ReleaseAcquirePublishes) {
    for (int round = 0; round < 100; ++round) {
        int data = 0;
        std::atomic<bool> ready{false};
        std::thread producer{[&] {
            data = 42;
            ready.store(true, std::memory_order_release);
        }};
        while (!ready.load(std::memory_order_acquire)) {}
        EXPECT_EQ(data, 42);
        producer.join();
    }
}

// "atomic_ref and atomic<shared_ptr> (C++20)": atomic_ref works everywhere;
// atomic<shared_ptr> is missing from libc++ 22.
TEST(Atomics, AtomicRef) {
    int plain = 0;
    std::atomic_ref<int> ar{plain};
    ar.fetch_add(1);
    EXPECT_EQ(plain, 1);
}

TEST(Atomics, AtomicSharedPtr) {
    CPPDOCS_REQUIRE(__cpp_lib_atomic_shared_ptr);
#ifdef __cpp_lib_atomic_shared_ptr
    struct Node { int value; };
    std::atomic<std::shared_ptr<Node>> head;
    EXPECT_EQ(head.load(), nullptr);
    head.store(std::make_shared<Node>(Node{3}));
    EXPECT_EQ(head.load()->value, 3);
#endif
}

// "Gotchas": not always lock-free — a large struct isn't; int is.
TEST(Atomics, LockFreeness) {
    struct Big { char bytes[64]; };
    static_assert(std::atomic<int>::is_always_lock_free);
    static_assert(!std::atomic<Big>::is_always_lock_free);
    std::atomic<Big> big{};
    EXPECT_FALSE(big.is_lock_free());
}
