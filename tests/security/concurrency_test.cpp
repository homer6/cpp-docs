// Checks claims made by docs/security/concurrency.md.
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {

class Account {
public:
    bool withdraw(int amount) {
        std::scoped_lock lock{m_};
        if (balance_ < amount) return false;
        balance_ -= amount;
        return true;
    }
    int balance() {
        std::scoped_lock lock{m_};
        return balance_;
    }

private:
    std::mutex m_;
    int balance_ = 100;
};

}  // namespace

// "One synchronization rule per shared object": with the check and the debit in one
// critical section, concurrent withdrawals of 60 from 100 let exactly one succeed.
TEST(SecurityConcurrency, CheckAndActInOneCriticalSection) {
    for (int round = 0; round < 50; ++round) {
        Account account;
        std::atomic<int> successes{0};
        std::vector<std::thread> threads;
        for (int i = 0; i < 8; ++i)
            threads.emplace_back([&] { if (account.withdraw(60)) ++successes; });
        for (auto& t : threads) t.join();
        ASSERT_EQ(successes.load(), 1);
        ASSERT_EQ(account.balance(), 40);
    }
}

// "One synchronization rule per shared object": scoped_lock takes two mutexes together,
// so opposite acquisition orders in two threads can't deadlock.
TEST(SecurityConcurrency, ScopedLockTakesBothMutexes) {
    std::mutex a, b;
    int shared = 0;
    std::thread t1([&] { for (int i = 0; i < 1000; ++i) { std::scoped_lock l{a, b}; ++shared; } });
    std::thread t2([&] { for (int i = 0; i < 1000; ++i) { std::scoped_lock l{b, a}; ++shared; } });
    t1.join();
    t2.join();
    EXPECT_EQ(shared, 2000);
}

// "shared_ptr is ownership, not synchronization": copies of one shared_ptr in many threads
// keep the count right (the count is atomic).
TEST(SecurityConcurrency, SharedPtrCountIsAtomic) {
    auto p = std::make_shared<int>(7);
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i)
        threads.emplace_back([p] { for (int j = 0; j < 1000; ++j) { auto copy = p; (void)copy; } });
    for (auto& t : threads) t.join();
    EXPECT_EQ(p.use_count(), 1);
}

// "shared_ptr is ownership, not synchronization": atomic<shared_ptr> (C++20) swaps a shared
// pointer variable safely; libc++ 22 doesn't have it yet.
TEST(SecurityConcurrency, AtomicSharedPtr) {
    CPPDOCS_REQUIRE(__cpp_lib_atomic_shared_ptr);
#if defined(__cpp_lib_atomic_shared_ptr)
    std::atomic<std::shared_ptr<int>> current{std::make_shared<int>(1)};
    std::thread writer([&] { for (int i = 2; i < 100; ++i) current.store(std::make_shared<int>(i)); });
    std::thread reader([&] { for (int i = 0; i < 100; ++i) { auto v = current.load(); ASSERT_TRUE(v); } });
    writer.join();
    reader.join();
    EXPECT_EQ(*current.load(), 99);
#endif
}
