// Checks claims made by docs/examples/concurrency/atomics.md.
#include <atomic>
#include <cstddef>
#include <latch>
#include <memory>
#include <new>
#include <stdatomic.h>
#include <thread>
#include <type_traits>
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

// "wait and notify (C++20)": wait blocks while the value equals old; a store
// followed by notify_all wakes every waiter. Correct whichever runs first.
TEST(Atomics, WaitNotifyAll) {
    std::atomic<int> state{0};
    std::atomic<int> woke{0};
    std::vector<std::thread> ts;
    for (int i = 0; i < 4; ++i)
        ts.emplace_back([&] {
            state.wait(0);
            EXPECT_NE(state.load(), 0);
            ++woke;
        });
    state.store(1);
    state.notify_all();
    for (auto& t : ts) t.join();
    EXPECT_EQ(woke, 4);
}

// "wait and notify (C++20)": notify_one wakes a single waiter; wait returns at
// once when the value already differs. atomic_flag and atomic_ref have them too.
TEST(Atomics, WaitNotifyOne) {
    std::atomic<int> state{0};
    int seen = 0;
    std::thread t{[&] { state.wait(0); seen = state.load(); }};
    state.store(2);
    state.notify_one();
    t.join();
    EXPECT_EQ(seen, 2);
    state.wait(0);  // 2 != 0: returns immediately

    std::atomic_flag f;
    f.wait(true);  // clear != true: returns immediately
    int plain = 1;
    std::atomic_ref<int> r{plain};
    r.wait(0);
    r.notify_all();
}

// "fetch_max and fetch_min (C++26)": return the old value, keep the max/min.
TEST(Atomics, FetchMaxMin) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_atomic_min_max, 202403L);
#if __cpp_lib_atomic_min_max >= 202403L
    std::atomic<int> peak{3};
    EXPECT_EQ(peak.fetch_max(7), 3);
    EXPECT_EQ(peak.fetch_max(5), 7);
    EXPECT_EQ(peak, 7);
    EXPECT_EQ(peak.fetch_min(1), 7);
    EXPECT_EQ(peak, 1);

    int x = 10;
    std::atomic_ref<int> r{x};
    r.fetch_min(4);
    EXPECT_EQ(x, 4);

    int arr[4]{};
    std::atomic<int*> p{arr + 2};
    p.fetch_max(arr + 3);
    EXPECT_EQ(p.load(), arr + 3);

    std::atomic<unsigned> u{5};
    EXPECT_EQ(std::atomic_fetch_max(&u, 9u), 5u);
    EXPECT_EQ(u, 9u);

    std::atomic<int> best{0};  // racing threads: the largest sample wins
    std::latch go{1};
    std::vector<std::thread> ts;
    for (int i = 1; i <= 8; ++i)
        ts.emplace_back([&, i] { go.wait(); for (int j = 0; j < 1000; ++j) best.fetch_max(i * 1000 + j); });
    go.count_down();
    for (auto& t : ts) t.join();
    EXPECT_EQ(best, 8999);
#endif
}

// "fetch_max and fetch_min (C++26)": the pre-C++26 CAS loop gives the same result.
TEST(Atomics, FetchMaxCasLoop) {
    auto fetch_max = [](std::atomic<int>& a, int v) {
        int cur = a.load();
        while (v > cur && !a.compare_exchange_weak(cur, v)) {}
        return cur;
    };
    std::atomic<int> a{3};
    EXPECT_EQ(fetch_max(a, 7), 3);
    EXPECT_EQ(fetch_max(a, 5), 7);
    EXPECT_EQ(a, 7);
}

// "False sharing and interference sizes (C++17)": the padded counters sit a
// destructive size apart; the standard requires both sizes >= alignof(max_align_t).
TEST(Atomics, InterferenceSizes) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_hardware_interference_size, 201703L);
#ifdef __cpp_lib_hardware_interference_size
    constexpr auto destructive = std::hardware_destructive_interference_size;
    constexpr auto constructive = std::hardware_constructive_interference_size;
    static_assert(destructive >= alignof(std::max_align_t));
    static_assert(constructive >= alignof(std::max_align_t));
    struct Counters {
        alignas(destructive) std::atomic<long> produced{0};
        alignas(destructive) std::atomic<long> consumed{0};
    };
    static_assert(sizeof(Counters) >= 2 * destructive);
    static_assert(offsetof(Counters, consumed) - offsetof(Counters, produced) >= destructive);
    struct Hot { int key; int value; };
    static_assert(sizeof(Hot) <= constructive);
#if defined(_MSVC_STL_VERSION)
    // MS STL (MSVC and clang-cl) fixes both at 64 on x86, x64 and ARM64; checked first
    // because clang-cl on ARM64 also defines __aarch64__.
    EXPECT_EQ(destructive, 64u);
    EXPECT_EQ(constructive, 64u);
#elif defined(__x86_64__) && defined(__GNUC__) && !defined(__clang__)
    EXPECT_EQ(destructive, 64u);  // the page's x86-64 GCC values
    EXPECT_EQ(constructive, 64u);
#elif defined(__aarch64__) || defined(__arm64__)
    EXPECT_EQ(destructive, 256u);  // the page's AArch64 values
    EXPECT_EQ(constructive, 64u);
#endif
#endif
}

// "C compatibility: <stdatomic.h> (C++23)": the C names are the C++ types.
TEST(Atomics, StdatomicH) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_stdatomic_h, 202011L);
#ifdef __cpp_lib_stdatomic_h
    static_assert(std::is_same_v<atomic_int, std::atomic<int>>);
    static_assert(std::is_same_v<_Atomic(long), std::atomic<long>>);
    atomic_int c = 0;
    atomic_fetch_add_explicit(&c, 1, memory_order_relaxed);
    EXPECT_EQ(atomic_load(&c), 1);
#endif
}
