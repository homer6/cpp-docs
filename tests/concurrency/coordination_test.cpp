// Checks claims made by docs/examples/concurrency/coordination.md.
#include <atomic>
#include <barrier>
#include <latch>
#include <semaphore>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

// "std::latch": wait() returns only after all N count_down()s; then it stays at zero.
TEST(Coordination, LatchCountsDown) {
    std::latch done{3};
    std::atomic<int> finished{0};
    {
        std::vector<std::jthread> workers;
        for (int i = 0; i < 3; ++i)
            workers.emplace_back([&] {
                ++finished;
                done.count_down();
            });
        done.wait();
        EXPECT_EQ(finished, 3);
    }
    EXPECT_TRUE(done.try_wait());  // one-shot: stays released
    done.wait();                   // returns immediately
}

// "std::latch": arrive_and_wait counts down and then waits.
TEST(Coordination, LatchArriveAndWait) {
    std::latch l{2};
    std::jthread other{[&] { l.arrive_and_wait(); }};
    l.arrive_and_wait();
    EXPECT_TRUE(l.try_wait());
}

// "std::barrier": the completion function runs once per phase, after all
// arrive and before any proceed; the barrier resets for the next phase.
TEST(Coordination, BarrierPhases) {
    constexpr int N = 10, kThreads = 4;
    int phases = 0;
    std::atomic<int> work[N]{};
    bool all_done_before_completion = true;
    std::barrier sync{kThreads, [&]() noexcept {
                          if (work[phases] != kThreads) all_done_before_completion = false;
                          ++phases;
                      }};
    {
        std::vector<std::jthread> ts;
        for (int t = 0; t < kThreads; ++t)
            ts.emplace_back([&] {
                for (int phase = 0; phase < N; ++phase) {
                    ++work[phase];
                    sync.arrive_and_wait();
                }
            });
    }
    EXPECT_EQ(phases, N);
    EXPECT_TRUE(all_done_before_completion);
}

// "std::counting_semaphore": permits run out and come back; max() is the template bound.
TEST(Coordination, CountingSemaphore) {
    std::counting_semaphore<8> slots{4};
    static_assert(std::counting_semaphore<8>::max() >= 8);
    for (int i = 0; i < 4; ++i) EXPECT_TRUE(slots.try_acquire());
    EXPECT_FALSE(slots.try_acquire());  // none left
    slots.release();
    EXPECT_TRUE(slots.try_acquire());
}

// "std::counting_semaphore" + "Gotchas": a binary_semaphore works as a one-shot
// event, and any thread may release — no ownership.
TEST(Coordination, BinarySemaphoreEventNoOwnership) {
    std::binary_semaphore event{0};
    EXPECT_FALSE(event.try_acquire());
    std::jthread notifier{[&] { event.release(); }};  // never acquired it
    event.acquire();
    SUCCEED();
}
