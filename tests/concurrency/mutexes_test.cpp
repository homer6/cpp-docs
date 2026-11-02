// Checks claims made by docs/examples/concurrency/mutexes.md.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <latch>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Lock with RAII": the guard unlocks on scope exit, even when the scope throws.
TEST(Mutexes, LockGuardUnlocksOnThrow) {
    std::mutex m;
    try {
        std::lock_guard<std::mutex> lk{m};
        throw std::runtime_error("x");
    } catch (const std::runtime_error&) {
    }
    EXPECT_TRUE(m.try_lock());
    m.unlock();
}

// "Lock with RAII": increments from many threads under the lock aren't lost.
TEST(Mutexes, CriticalSection) {
    std::mutex m;
    int shared = 0;
    std::latch go{1};
    std::vector<std::thread> ts;
    for (int i = 0; i < 4; ++i)
        ts.emplace_back([&] {
            go.wait();
            for (int j = 0; j < 10000; ++j) {
                std::lock_guard lk{m};
                ++shared;
            }
        });
    go.count_down();
    for (auto& t : ts) t.join();
    EXPECT_EQ(shared, 40000);
}

// "The lock types": scoped_lock locks both mutexes; opposite orders don't deadlock.
TEST(Mutexes, ScopedLockMultiple) {
    std::mutex m1, m2;
    std::latch go{1};
    auto a = [&] { go.wait(); for (int i = 0; i < 10000; ++i) std::scoped_lock lk{m1, m2}; };
    auto b = [&] { go.wait(); for (int i = 0; i < 10000; ++i) std::scoped_lock lk{m2, m1}; };
    std::thread t1{a}, t2{b};
    go.count_down();
    t1.join();
    t2.join();
    {
        std::scoped_lock lk{m1, m2};
        std::thread probe{[&] { EXPECT_FALSE(m1.try_lock()); EXPECT_FALSE(m2.try_lock()); }};
        probe.join();
    }
}

// "Mutex flavors": recursive_mutex re-locks on the same thread; timed_mutex try_lock_for.
TEST(Mutexes, RecursiveAndTimed) {
    std::recursive_mutex rm;
    std::lock_guard a{rm};
    std::lock_guard b{rm};  // same thread: fine
    std::timed_mutex tm;
    std::unique_lock held{tm};
    bool got = true;
    std::thread{[&] { got = tm.try_lock_for(std::chrono::milliseconds{1}); }}.join();
    EXPECT_FALSE(got);
}

// "Mutex flavors": shared_mutex allows many readers or one writer.
TEST(Mutexes, SharedMutexReadersAndWriter) {
    std::shared_mutex sm;
    std::shared_lock r1{sm};
    bool second_reader = false, writer = true;
    std::thread{[&] {
        second_reader = sm.try_lock_shared();
        if (second_reader) sm.unlock_shared();
        writer = sm.try_lock();
    }}.join();
    EXPECT_TRUE(second_reader);
    EXPECT_FALSE(writer);
}

// "Condition variables": the consumer waits with a predicate and gets the value.
TEST(Mutexes, ConditionVariableQueue) {
    std::mutex m;
    std::condition_variable cv;
    std::queue<int> q;
    int got = 0;
    std::thread consumer{[&] {
        std::unique_lock lk{m};
        cv.wait(lk, [&] { return !q.empty(); });
        got = q.front();
        q.pop();
    }};
    {
        std::lock_guard lk{m};
        q.push(7);
    }
    cv.notify_one();
    consumer.join();
    EXPECT_EQ(got, 7);
}

// "Gotchas": condition_variable needs unique_lock; condition_variable_any takes
// other lockables and a stop_token.
template <class CV, class L>
concept CanWait = requires(CV cv, L& lk) { cv.wait(lk); };
TEST(Mutexes, ConditionVariableLockTypes) {
    static_assert(CanWait<std::condition_variable, std::unique_lock<std::mutex>>);
    static_assert(!CanWait<std::condition_variable, std::lock_guard<std::mutex>>);
    static_assert(CanWait<std::condition_variable_any, std::shared_lock<std::shared_mutex>>);
    // stop_token and its condition_variable_any overloads come with jthread (P0660).
    CPPDOCS_REQUIRE(__cpp_lib_jthread);
#ifdef __cpp_lib_jthread
    std::condition_variable_any cv;
    std::mutex m;
    std::stop_source ss;
    ss.request_stop();
    std::unique_lock lk{m};
    EXPECT_FALSE(cv.wait(lk, ss.get_token(), [] { return false; }));  // returns on stop
#endif
}

// "Run something exactly once": racing threads run the callable once, and every
// caller returns only after it has finished.
TEST(Mutexes, CallOnceRunsOnce) {
    std::once_flag flag;
    std::atomic<int> calls{0};
    int value = 0;
    std::latch go{1};
    std::vector<int> seen(8);
    std::vector<std::thread> ts;
    for (int i = 0; i < 8; ++i)
        ts.emplace_back([&, i] {
            go.wait();
            std::call_once(flag, [&] { ++calls; value = 42; });
            seen[i] = value;  // the setup is complete and visible here
        });
    go.count_down();
    for (auto& t : ts) t.join();
    EXPECT_EQ(calls, 1);
    for (int v : seen) EXPECT_EQ(v, 42);
}

// "Run something exactly once": a function-local static is initialized once,
// thread-safely.
namespace {
std::atomic<int> config_ctors{0};
struct Config { int port = 8080; Config() { ++config_ctors; } };
Config& config() { static Config c; return c; }
}  // namespace
TEST(Mutexes, LocalStaticInitializedOnce) {
    std::latch go{1};
    std::vector<std::thread> ts;
    for (int i = 0; i < 8; ++i)
        ts.emplace_back([&] { go.wait(); EXPECT_EQ(config().port, 8080); });
    go.count_down();
    for (auto& t : ts) t.join();
    EXPECT_EQ(config_ctors, 1);
}

// "Run something exactly once": a once_flag member gives once per object.
namespace {
class Connection {
    std::once_flag opened_;
    void open() { ++opens; }
public:
    int opens = 0;
    void send() { std::call_once(opened_, &Connection::open, this); }
};
}  // namespace
TEST(Mutexes, CallOncePerObject) {
    Connection a, b;
    a.send(); a.send(); b.send();
    EXPECT_EQ(a.opens, 1);
    EXPECT_EQ(b.opens, 1);
    static_assert(!std::is_copy_constructible_v<std::once_flag>);
    static_assert(!std::is_move_constructible_v<std::once_flag>);
    static_assert(!std::is_copy_constructible_v<Connection>);
}

// "Run something exactly once" + "Gotchas": a throwing callable leaves the flag
// unset and the next call retries; libstdc++ off Linux breaks this (GCC PR 66146).
TEST(Mutexes, CallOnceRetriesAfterThrow) {
#if defined(__GLIBCXX__) && !defined(__linux__)
    GTEST_SKIP() << "libstdc++ off Linux: call_once with a throwing callable is broken (GCC PR 66146)";
#else
    std::once_flag flag;
    int attempts = 0;
    EXPECT_THROW(std::call_once(flag, [&] { ++attempts; throw std::runtime_error("x"); }),
                 std::runtime_error);
    std::call_once(flag, [&] { ++attempts; });
    std::call_once(flag, [&] { ++attempts; });
    EXPECT_EQ(attempts, 2);
#endif
}

// "Run something exactly once": a static's initializer that throws is retried.
TEST(Mutexes, LocalStaticRetriesAfterThrow) {
    static int tries = 0;
    auto get = [] -> int& {
        static int v = [] { if (++tries < 2) throw std::runtime_error("x"); return 7; }();
        return v;
    };
    EXPECT_THROW(get(), std::runtime_error);
    EXPECT_EQ(get(), 7);
    EXPECT_EQ(tries, 2);
}
