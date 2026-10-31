// Checks claims made by docs/examples/concurrency/futures.md.
#include <chrono>
#include <atomic>
#include <future>
#include <latch>
#include <stdexcept>
#include <thread>
#include <utility>

#include <gtest/gtest.h>

// "Mental model": an exception thrown by the producer is re-thrown at get().
TEST(Futures, AsyncRethrowsAtGet) {
    auto f = std::async(std::launch::async, []() -> int { throw std::runtime_error("boom"); });
    EXPECT_THROW((void)f.get(), std::runtime_error);
}

// "promise / future" and "packaged_task": the values from the examples.
TEST(Futures, PromiseAndPackagedTaskDeliverValues) {
    std::promise<int> p;
    std::future<int> f = p.get_future();
    std::thread t{[&p] { p.set_value(42); }};
    EXPECT_EQ(f.get(), 42);
    t.join();

    std::packaged_task<int(int)> task{[](int x) { return x * x; }};
    std::future<int> g = task.get_future();
    std::thread{std::move(task), 7}.join();
    EXPECT_EQ(g.get(), 49);
}

// "Gotchas": get() can be called only once; afterwards the future is no longer valid.
TEST(Futures, GetOnlyOnce) {
    std::promise<int> p;
    auto f = p.get_future();
    p.set_value(1);
    EXPECT_TRUE(f.valid());
    EXPECT_EQ(f.get(), 1);
    EXPECT_FALSE(f.valid());
}

// "Gotchas": with launch::deferred the task runs lazily, on the thread that calls get().
TEST(Futures, DeferredRunsOnGetCaller) {
    using namespace std::chrono_literals;
    bool ran = false;
    auto f = std::async(std::launch::deferred, [&ran] {
        ran = true;
        return std::this_thread::get_id();
    });
    EXPECT_EQ(f.wait_for(0s), std::future_status::deferred);
    EXPECT_FALSE(ran);
    EXPECT_EQ(f.get(), std::this_thread::get_id());
    EXPECT_TRUE(ran);
}

// "Gotchas": a promise destroyed without a value makes get() throw
// std::future_error with code broken_promise.
TEST(Futures, BrokenPromise) {
    std::future<int> f;
    {
        std::promise<int> p;
        f = p.get_future();
    }
    try {
        (void)f.get();
        FAIL() << "expected std::future_error";
    } catch (const std::future_error& e) {
        EXPECT_EQ(e.code(), std::future_errc::broken_promise);
    }
}

// "std::async": launch::async runs the task on another thread.
TEST(Futures, LaunchAsyncUsesAnotherThread) {
    auto f = std::async(std::launch::async, [] { return std::this_thread::get_id(); });
    EXPECT_NE(f.get(), std::this_thread::get_id());
}

// "Checking without blocking": wait_for(0s) reports ready once the value is set.
TEST(Futures, WaitForZeroPolls) {
    using namespace std::chrono_literals;
    std::promise<int> p;
    auto f = p.get_future();
    EXPECT_EQ(f.wait_for(0s), std::future_status::timeout);
    p.set_value(1);
    EXPECT_EQ(f.wait_for(0s), std::future_status::ready);
    f.wait();
    EXPECT_EQ(f.get(), 1);
}

// "Gotchas": shared_future allows several waiters.
TEST(Futures, SharedFutureManyWaiters) {
    std::promise<int> p;
    std::shared_future<int> sf = p.get_future().share();
    std::latch both_waiting{2};
    int a = 0, b = 0;
    std::thread t1{[&, sf] { both_waiting.count_down(); a = sf.get(); }};
    std::thread t2{[&, sf] { both_waiting.count_down(); b = sf.get(); }};
    both_waiting.wait();
    p.set_value(5);
    t1.join();
    t2.join();
    EXPECT_EQ(a + b, 10);
    EXPECT_EQ(sf.get(), 5);  // still valid
}

// "Gotchas": the destructor of an async-launched future blocks until the task is done.
TEST(Futures, AsyncFutureDestructorBlocks) {
    std::atomic<bool> done{false};
    {
        auto f = std::async(std::launch::async, [&] { done = true; });
    }
    EXPECT_TRUE(done);
}
