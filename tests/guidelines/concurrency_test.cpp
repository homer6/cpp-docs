// Checks claims made by docs/guidelines/core-guidelines/examples/concurrency.md.
//
// Every test is deterministic: threads are joined (or their results read
// through futures) before anything is checked, and nothing waits on a timer.
#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <csignal>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

#ifdef __cpp_lib_jthread
#include <stop_token>
#endif

namespace {

// Runs fn on n threads at once and joins them all.
template <class F>
void run_on_threads(unsigned n, F fn) {
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < n; ++i) threads.emplace_back(fn);
    for (auto& t : threads) t.join();
}

// A mutex that another thread can lock is free. try_lock may fail spuriously,
// so retry: a mutex that is really held never succeeds.
bool free_from_another_thread(std::mutex& m) {
    bool locked = false;
    std::thread([&] {
        for (int i = 0; i < 1000 && !locked; ++i) {
            if (m.try_lock()) {
                locked = true;
                m.unlock();
            }
        }
    }).join();
    return locked;
}

bool held_from_another_thread(std::mutex& m) {
    bool got = true;
    std::thread([&] {
        got = m.try_lock();
        if (got) m.unlock();
    }).join();
    return !got;
}

// CP.1
int calls_on_this_thread() {
    thread_local int calls = 0;
    return ++calls;
}

// CP.3
std::vector<int> evens_and_odds(const std::vector<int>& in) {
    auto pick = [&in](int rem) {
        std::vector<int> part;
        for (int x : in)
            if (x % 2 == rem) part.push_back(x);
        return part;
    };
    auto evens = std::async(std::launch::async, pick, 0);
    auto odds = std::async(std::launch::async, pick, 1);
    std::vector<int> out = evens.get();
    std::vector<int> rest = odds.get();
    out.insert(out.end(), rest.begin(), rest.end());
    return out;
}

// CP.21
struct Account {
    std::mutex m;
    long balance = 0;
};
void transfer(Account& from, Account& to, long amount) {
    std::scoped_lock lock(from.m, to.m);
    from.balance -= amount;
    to.balance += amount;
}

// CP.22
class Publisher {
public:
    void subscribe(std::function<void(int)> f) {
        std::scoped_lock l(m_);
        subs_.push_back(std::move(f));
    }
    std::size_t size() {
        std::scoped_lock l(m_);
        return subs_.size();
    }
    void publish(int v) {
        std::vector<std::function<void(int)>> snapshot;
        {
            std::scoped_lock l(m_);
            snapshot = subs_;
        }
        for (auto& f : snapshot) f(v);
    }

private:
    std::mutex m_;
    std::vector<std::function<void(int)>> subs_;
};

// CP.31
std::string shout(std::string s) {
    for (char& c : s) c = static_cast<char>(c & ~0x20);
    return s;
}

// CP.50
template <class T>
class Synchronized {
public:
    template <class F>
    decltype(auto) with(F&& f) {
        std::scoped_lock lock(m_);
        return std::forward<F>(f)(value_);
    }

private:
    std::mutex m_;
    T value_{};
};

// A minimal lazy coroutine that produces a string, for the CP.51-CP.53 tests.
struct Task {
    struct promise_type {
        std::string value;
        Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(std::string v) { value = std::move(v); }
        void unhandled_exception() { std::terminate(); }
    };
    explicit Task(std::coroutine_handle<promise_type> h) : h_(h) {}
    Task(Task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    Task& operator=(Task&&) = delete;
    ~Task() {
        if (h_) h_.destroy();
    }
    void resume() { h_.resume(); }
    bool done() const { return h_.done(); }
    const std::string& value() const { return h_.promise().value; }

private:
    std::coroutine_handle<promise_type> h_;
};

Task make_task(std::string name) {
    auto body = [](std::string n) -> Task {  // no captures: n lives in the frame
        co_await std::suspend_always{};
        co_return "hello " + n;
    };
    return body(std::move(name));
}

std::mutex coro_mutex;
int coro_total = 0;
Task add_then_wait(int x) {
    {
        std::scoped_lock lock(coro_mutex);
        coro_total += x;
    }
    co_await std::suspend_always{};
    co_return "added";
}

Task greet(std::string name) {
    co_await std::suspend_always{};
    co_return "hi " + name;
}

// CP.100
class Stack {
public:
    void push(int v) {
        std::scoped_lock l(m_);
        items_.push_back(v);
    }
    std::optional<int> pop() {
        std::scoped_lock l(m_);
        if (items_.empty()) return std::nullopt;
        int v = items_.back();
        items_.pop_back();
        return v;
    }

private:
    std::mutex m_;
    std::vector<int> items_;
};

// CP.110
std::atomic<int> registry_constructions{0};
struct Registry {
    Registry() { ++registry_constructions; }
};
Registry& registry() {
    static Registry instance;
    return instance;
}

// CP.201
volatile std::sig_atomic_t interrupted = 0;
extern "C" void on_interrupt(int) { interrupted = 1; }

}  // namespace

// CP.1: a thread_local cache is per thread: a new thread starts with its own fresh copy.
TEST(CoreGuidelinesCP, CP1_ThreadLocalStateIsPerThread) {
    EXPECT_EQ(calls_on_this_thread(), 1);
    EXPECT_EQ(calls_on_this_thread(), 2);
    int other = 0;
    std::thread([&] { other = calls_on_this_thread(); }).join();
    EXPECT_EQ(other, 1);
    EXPECT_EQ(calls_on_this_thread(), 3);
}

// CP.2: concurrent fetch_adds on an atomic counter lose no update, and each sees a different previous value.
TEST(CoreGuidelinesCP, CP2_AtomicCounterLosesNoUpdate) {
    std::atomic<long> requests_served{0};
    std::mutex m;
    std::set<long> seen;
    run_on_threads(4, [&] {
        std::vector<long> mine;
        for (int i = 0; i < 1000; ++i) mine.push_back(requests_served.fetch_add(1, std::memory_order_relaxed));
        std::scoped_lock l(m);
        seen.insert(mine.begin(), mine.end());
    });
    EXPECT_EQ(requests_served.load(), 4000);
    EXPECT_EQ(seen.size(), 4000u);
}

// CP.3: tasks that share only const input and return their own results need no lock.
TEST(CoreGuidelinesCP, CP3_ConstInputOwnResults) {
    const std::vector<int> in{1, 2, 3, 4, 5, 6, 7};
    EXPECT_EQ(evens_and_odds(in), (std::vector<int>{2, 4, 6, 1, 3, 5, 7}));
}

// CP.4: a task returns its value, or its exception, through a future.
TEST(CoreGuidelinesCP, CP4_TaskReturnsValueOrException) {
    auto ok = std::async(std::launch::async, [] { return 6 * 7; });
    EXPECT_EQ(ok.get(), 42);
    auto bad = std::async(std::launch::async, []() -> int { throw std::runtime_error("parse error"); });
    EXPECT_THROW(static_cast<void>(bad.get()), std::runtime_error);
}

// CP.8: atomics, not volatile, synchronize: a counter adds up exactly and a flag publishes data.
TEST(CoreGuidelinesCP, CP8_AtomicsSynchronize) {
    std::atomic<int> count{0};
    run_on_threads(4, [&] {
        for (int i = 0; i < 10000; ++i) count.fetch_add(1);
    });
    EXPECT_EQ(count.load(), 40000);

    std::atomic<bool> ready{false};
    int payload = 0;
    int seen = 0;
    std::thread consumer([&] {
        while (!ready.load()) std::this_thread::yield();
        seen = payload;
    });
    std::thread producer([&] {
        payload = 42;
        ready.store(true);
    });
    producer.join();
    consumer.join();
    EXPECT_EQ(seen, 42);
}

// CP.20: a lock object unlocks on every way out of its scope, including a throw.
TEST(CoreGuidelinesCP, CP20_LockReleasedOnThrowAndReturn) {
    std::mutex m;
    auto add = [&](int x) {
        std::scoped_lock lock(m);
        if (x < 0) return false;
        if (x == 0) throw std::invalid_argument("zero");
        return true;
    };
    EXPECT_FALSE(add(-1));
    EXPECT_TRUE(free_from_another_thread(m));
    EXPECT_THROW(add(0), std::invalid_argument);
    EXPECT_TRUE(free_from_another_thread(m));
    EXPECT_TRUE(add(1));
}

// CP.21: scoped_lock takes two mutexes without deadlock even when threads list them in opposite orders.
TEST(CoreGuidelinesCP, CP21_ScopedLockOppositeOrdersNoDeadlock) {
    Account a, b;
    a.balance = 1000;
    b.balance = 1000;
    std::thread t1([&] {
        for (int i = 0; i < 10000; ++i) transfer(a, b, 1);
    });
    std::thread t2([&] {
        for (int i = 0; i < 10000; ++i) transfer(b, a, 1);
    });
    t1.join();
    t2.join();
    EXPECT_EQ(a.balance, 1000);
    EXPECT_EQ(b.balance, 1000);
}

// CP.22: callbacks called after the lock is released may call back into the object.
TEST(CoreGuidelinesCP, CP22_CallbackRunsWithoutTheLock) {
    Publisher pub;
    std::size_t seen_size = 0;
    int seen_value = 0;
    pub.subscribe([&](int v) {
        seen_value = v;
        seen_size = pub.size();  // locks the publisher's mutex: fine, it isn't held
    });
    pub.publish(7);
    EXPECT_EQ(seen_value, 7);
    EXPECT_EQ(seen_size, 1u);
}

// CP.23: a jthread joins before the locals declared ahead of it are destroyed, so it may use them.
TEST(CoreGuidelinesCP, CP23_JoiningThreadMayUseEnclosingLocals) {
    CPPDOCS_REQUIRE(__cpp_lib_jthread);
#ifdef __cpp_lib_jthread
    std::vector<int> out;
    {
        std::jthread t([&out] { out.push_back(1); });
    }
    EXPECT_EQ(out, std::vector<int>{1});
#endif
}

// CP.25: std::jthread joins in its destructor: the work is done when the scope ends, with no join() call.
TEST(CoreGuidelinesCP, CP25_JthreadJoinsInDestructor) {
    CPPDOCS_REQUIRE(__cpp_lib_jthread);
#ifdef __cpp_lib_jthread
    int result = 0;
    {
        std::jthread t([&result] { result = 42; });
        EXPECT_TRUE(t.joinable());
    }
    EXPECT_EQ(result, 42);
#endif
}

// CP.26: an owned jthread with a stop_token stops and is joined when its owner is destroyed.
TEST(CoreGuidelinesCP, CP26_OwnedThreadStopsOnDestruction) {
    CPPDOCS_REQUIRE(__cpp_lib_jthread);
#ifdef __cpp_lib_jthread
    std::atomic<int> beats{0};
    bool stopped = false;
    {
        std::jthread heartbeat([&](std::stop_token st) {
            while (!st.stop_requested()) {
                beats.fetch_add(1);
                std::this_thread::yield();
            }
            stopped = true;
        });
        while (beats.load() == 0) std::this_thread::yield();
    }  // request_stop() and join()
    EXPECT_TRUE(stopped);
    EXPECT_GT(beats.load(), 0);
#endif
}

// CP.31: async copies its arguments when it is called, so later changes to the caller's object don't reach the task.
TEST(CoreGuidelinesCP, CP31_ArgumentsAreCopied) {
    std::string word = "quiet";
    auto f = std::async(std::launch::async, shout, word);
    word = "changed";
    EXPECT_EQ(f.get(), "QUIET");
    EXPECT_EQ(word, "changed");
}

// CP.32: an object shared through shared_ptr copies is destroyed once, after the last thread lets go.
TEST(CoreGuidelinesCP, CP32_SharedPtrDeletesOnceAfterLastOwner) {
    std::atomic<int> destroyed{0};
    struct Config {
        explicit Config(std::atomic<int>* d) : destroyed(d) {}
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;
        ~Config() { destroyed->fetch_add(1); }
        std::atomic<int>* destroyed;
        int level = 3;
    };
    std::atomic<int> sum{0};
    {
        auto cfg = std::make_shared<const Config>(&destroyed);
        std::thread a([cfg, &sum] { sum += cfg->level; });
        std::thread b([cfg, &sum] { sum += cfg->level; });
        cfg.reset();
        a.join();
        b.join();
    }
    EXPECT_EQ(sum.load(), 6);
    EXPECT_EQ(destroyed.load(), 1);
}

// CP.41: a fixed set of workers sharing an atomic index handles every item exactly once.
TEST(CoreGuidelinesCP, CP41_FixedWorkersHandleEveryItemOnce) {
    std::vector<std::atomic<int>> hits(500);
    std::atomic<std::size_t> next{0};
    run_on_threads(4, [&] {
        for (std::size_t i = 0; (i = next.fetch_add(1)) < hits.size();) hits[i].fetch_add(1);
    });
    for (const auto& h : hits) EXPECT_EQ(h.load(), 1);
}

// CP.42: a wait with a predicate doesn't miss a notification sent before it started.
TEST(CoreGuidelinesCP, CP42_PredicateWaitSeesEarlierNotify) {
    std::mutex m;
    std::condition_variable cv;
    bool ready = false;
    {
        std::scoped_lock l(m);
        ready = true;
    }
    cv.notify_one();  // nobody is waiting yet: without a predicate this wake-up is lost
    std::unique_lock lock(m);
    cv.wait(lock, [&] { return ready; });  // returns at once
    EXPECT_TRUE(ready);

    bool go = false;
    std::thread waiter([&] {
        std::unique_lock l(m);
        cv.wait(l, [&] { return go; });
    });
    go = true;  // still under `lock`
    lock.unlock();
    cv.notify_one();
    waiter.join();
    EXPECT_TRUE(go);
}

// CP.43: work done before the lock's block runs with the mutex free.
TEST(CoreGuidelinesCP, CP43_PrepWorkRunsOutsideTheLock) {
    std::mutex m;
    std::vector<std::string> lines;
    bool free_during_format = false;
    auto format_entry = [&](int id) {
        free_during_format = free_from_another_thread(m);
        return "entry " + std::to_string(id);
    };
    std::string line = format_entry(1);
    {
        std::scoped_lock lock(m);
        EXPECT_TRUE(held_from_another_thread(m));
        lines.push_back(std::move(line));
    }
    EXPECT_TRUE(free_during_format);
    EXPECT_EQ(lines, std::vector<std::string>{"entry 1"});
}

// CP.44: an unnamed lock_guard unlocks at the end of its statement; a named one holds the mutex until scope end.
TEST(CoreGuidelinesCP, CP44_UnnamedGuardUnlocksImmediately) {
    std::mutex m;
    {
        // The cast silences the [[nodiscard]] warning libstdc++ and libc++ give for this mistake.
        static_cast<void>(std::lock_guard<std::mutex>{m});
        EXPECT_TRUE(free_from_another_thread(m));
    }
    {
        std::lock_guard<std::mutex> lock{m};
        EXPECT_TRUE(held_from_another_thread(m));
    }
    EXPECT_TRUE(free_from_another_thread(m));
}

// CP.50: data reachable only through its own mutex stays consistent under concurrent updates.
TEST(CoreGuidelinesCP, CP50_SynchronizedValueGuardsItsData) {
    Synchronized<std::vector<int>> data;
    run_on_threads(4, [&] {
        for (int i = 0; i < 1000; ++i) data.with([](auto& v) { v.push_back(1); });
    });
    EXPECT_EQ(data.with([](auto& v) { return v.size(); }), 4000u);
}

// CP.51: a coroutine lambda that takes its data as parameters keeps it in the frame after the closure is gone.
TEST(CoreGuidelinesCP, CP51_CoroutineLambdaParametersOutliveClosure) {
    Task t = make_task("Ada");  // the lambda object died inside make_task
    t.resume();
    ASSERT_FALSE(t.done());
    t.resume();
    ASSERT_TRUE(t.done());
    EXPECT_EQ(t.value(), "hello Ada");
}

// CP.52: a coroutine that releases its lock before co_await leaves the mutex free while suspended.
TEST(CoreGuidelinesCP, CP52_LockReleasedBeforeSuspending) {
    coro_total = 0;
    Task t = add_then_wait(5);
    t.resume();  // runs to the co_await
    ASSERT_FALSE(t.done());
    EXPECT_TRUE(free_from_another_thread(coro_mutex));
    t.resume();
    ASSERT_TRUE(t.done());
    EXPECT_EQ(coro_total, 5);
    EXPECT_EQ(t.value(), "added");
}

// CP.53: a by-value coroutine parameter is a copy in the frame, valid after the caller's temporary is gone.
TEST(CoreGuidelinesCP, CP53_ByValueParameterSurvivesSuspension) {
    Task t = greet(std::string("Grace"));  // the temporary dies at the end of this line
    t.resume();
    t.resume();
    ASSERT_TRUE(t.done());
    EXPECT_EQ(t.value(), "hi Grace");
}

// CP.60: future::get returns the task's value or rethrows its exception; promise and future carry both.
TEST(CoreGuidelinesCP, CP60_FutureCarriesValueAndException) {
    std::promise<long> p;
    std::future<long> f = p.get_future();
    std::thread([&p] { p.set_value(99); }).join();
    EXPECT_EQ(f.get(), 99);

    auto failing = std::async(std::launch::async, []() -> long { throw std::out_of_range("no file"); });
    EXPECT_THROW(static_cast<void>(failing.get()), std::out_of_range);
}

// CP.61: std::async with launch::async runs the task on another thread and returns its future.
TEST(CoreGuidelinesCP, CP61_AsyncRunsOnAnotherThread) {
    auto f = std::async(std::launch::async, [] { return std::this_thread::get_id(); });
    EXPECT_NE(f.get(), std::this_thread::get_id());
}

// CP.100: a mutex-protected stack loses nothing under concurrent pushes.
TEST(CoreGuidelinesCP, CP100_MutexStackIsCorrect) {
    Stack s;
    run_on_threads(4, [&] {
        for (int i = 1; i <= 1000; ++i) s.push(i);
    });
    long sum = 0;
    int count = 0;
    while (auto v = s.pop()) {
        sum += *v;
        ++count;
    }
    EXPECT_EQ(count, 4000);
    EXPECT_EQ(sum, 4L * 500500);
}

// CP.101: a release store and an acquire load make the data written before the store visible to the reader.
TEST(CoreGuidelinesCP, CP101_ReleaseAcquirePublishesData) {
    for (int round = 0; round < 50; ++round) {
        int data = 0;
        std::atomic<bool> ready{false};
        int seen = 0;
        std::thread reader([&] {
            while (!ready.load(std::memory_order_acquire)) std::this_thread::yield();
            seen = data;
        });
        std::thread writer([&] {
            data = 42 + round;
            ready.store(true, std::memory_order_release);
        });
        writer.join();
        reader.join();
        EXPECT_EQ(seen, 42 + round);
    }
}

// CP.110: a function-local static is constructed exactly once even when many threads reach it at once; so is call_once.
TEST(CoreGuidelinesCP, CP110_StaticLocalInitializedOnce) {
    std::set<Registry*> addresses;
    std::mutex m;
    run_on_threads(8, [&] {
        Registry* r = &registry();
        std::scoped_lock l(m);
        addresses.insert(r);
    });
    EXPECT_EQ(registry_constructions.load(), 1);
    EXPECT_EQ(addresses.size(), 1u);

    std::once_flag flag;
    std::atomic<int> runs{0};
    run_on_threads(8, [&] { std::call_once(flag, [&] { runs.fetch_add(1); }); });
    EXPECT_EQ(runs.load(), 1);
}

// CP.111: double-checked locking with an atomic flag and a mutex runs the action once.
TEST(CoreGuidelinesCP, CP111_AtomicDoubleCheckRunsOnce) {
    std::mutex flush_mutex;
    std::atomic<bool> needs_flush{true};
    int flushes = 0;
    run_on_threads(8, [&] {
        if (needs_flush.load(std::memory_order_acquire)) {
            std::scoped_lock l(flush_mutex);
            if (needs_flush.load(std::memory_order_relaxed)) {
                ++flushes;
                needs_flush.store(false, std::memory_order_release);
            }
        }
    });
    EXPECT_EQ(flushes, 1);
    EXPECT_FALSE(needs_flush.load());
}

// CP.201: a signal handler that only sets a volatile sig_atomic_t flag; normal code acts on it after raise() returns.
TEST(CoreGuidelinesCP, CP201_HandlerOnlySetsAFlag) {
    interrupted = 0;
    auto previous = std::signal(SIGINT, on_interrupt);
    ASSERT_NE(previous, SIG_ERR);
    ASSERT_EQ(std::raise(SIGINT), 0);  // the handler runs before raise returns
    std::signal(SIGINT, previous);
    const int seen = interrupted;
    EXPECT_EQ(seen, 1);
}
