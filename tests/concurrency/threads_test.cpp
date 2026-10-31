// Checks claims made by docs/examples/concurrency/threads.md.
#include <atomic>
#include <chrono>
#include <functional>
#include <latch>
#include <stop_token>
#include <string>
#include <thread>
#include <type_traits>

#include <gtest/gtest.h>

// "std::thread — the original": runs on another thread; join() waits for it.
TEST(Threads, RunsElsewhereJoinWaits) {
    std::thread::id worker_id;
    bool done = false;
    std::thread t{[&] {
        worker_id = std::this_thread::get_id();
        done = true;
    }};
    t.join();
    EXPECT_TRUE(done);
    EXPECT_NE(worker_id, std::this_thread::get_id());
    EXPECT_FALSE(t.joinable());
}

// "Gotchas": arguments are copied into the thread; std::ref passes a reference.
TEST(Threads, ArgumentsAreCopied) {
    int n = 0;
    std::thread{[](int& x) { x = 1; }, std::ref(n)}.join();
    EXPECT_EQ(n, 1);
    std::string s = "orig";
    std::thread{[](std::string copy) { copy += "!"; }, s}.join();
    EXPECT_EQ(s, "orig");
    static_assert(!std::is_invocable_v<std::thread(void (*)(int&), int)>);  // can't bind int& to a copy
}

// "Gotchas": destroying a thread that's neither joined nor detached calls std::terminate.
TEST(ThreadsDeathTest, UnjoinedThreadTerminates) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_DEATH(
        {
            std::latch never{1};
            std::thread t{[&] { never.wait(); }};
        },
        "");
}

// "std::jthread": a leading stop_token parameter is supplied; the destructor
// requests stop and joins.
TEST(Threads, JthreadStopsAndJoins) {
    std::atomic<bool> started{false}, finished{false};
    {
        std::jthread jt{[&](std::stop_token st) {
            started = true;
            started.notify_one();
            while (!st.stop_requested()) std::this_thread::yield();
            finished = true;
        }};
        started.wait(false);  // the worker is inside its loop
    }  // no explicit request_stop() or join()
    EXPECT_TRUE(finished);
}

// "std::jthread": request_stop() is seen through the token.
TEST(Threads, RequestStop) {
    std::jthread jt{[](std::stop_token st) {
        while (!st.stop_requested()) std::this_thread::yield();
    }};
    EXPECT_TRUE(jt.request_stop());
    jt.join();
    EXPECT_TRUE(jt.get_stop_token().stop_requested());
}

// "this_thread": sleep_until doesn't return before the deadline; hardware_concurrency is a hint.
TEST(Threads, ThisThread) {
    using namespace std::chrono_literals;
    auto deadline = std::chrono::steady_clock::now() + 1ms;
    std::this_thread::sleep_until(deadline);
    EXPECT_GE(std::chrono::steady_clock::now(), deadline);
    static_assert(std::is_same_v<decltype(std::thread::hardware_concurrency()), unsigned>);
}
