// Checks claims made by docs/security/resource-limits.md.
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

class Budget {
public:
    explicit Budget(std::size_t bytes) : left_{bytes} {}
    bool take(std::size_t n) noexcept {
        if (n > left_) return false;
        left_ -= n;
        return true;
    }
    std::size_t left() const noexcept { return left_; }

private:
    std::size_t left_;
};

template <class T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_{capacity} {}
    bool try_push(T value) {
        std::scoped_lock lock{m_};
        if (q_.size() >= capacity_) return false;
        q_.push_back(std::move(value));
        return true;
    }
    std::optional<T> try_pop() {
        std::scoped_lock lock{m_};
        if (q_.empty()) return std::nullopt;
        T v = std::move(q_.front());
        q_.pop_front();
        return v;
    }

private:
    std::mutex m_;
    std::deque<T> q_;
    std::size_t capacity_;
};

}  // namespace

// "Give each request a budget": many small allocations draw from one budget, so the
// total is limited even though each piece is small.
TEST(SecurityResourceLimits, BudgetLimitsTheTotal) {
    Budget b{1000};
    int accepted = 0;
    while (b.take(64)) ++accepted;
    EXPECT_EQ(accepted, 15);          // 15 * 64 = 960; the 16th would exceed 1000
    EXPECT_EQ(b.left(), 40u);
    EXPECT_FALSE(b.take(41));
    EXPECT_TRUE(b.take(40));
}

// "Bound queues and concurrency": a full queue rejects instead of growing.
TEST(SecurityResourceLimits, BoundedQueueRejectsWhenFull) {
    BoundedQueue<std::string> q{3};
    EXPECT_TRUE(q.try_push("a"));
    EXPECT_TRUE(q.try_push("b"));
    EXPECT_TRUE(q.try_push("c"));
    EXPECT_FALSE(q.try_push("d"));
    EXPECT_EQ(q.try_pop(), "a");
    EXPECT_TRUE(q.try_push("d"));
}

// "Handle allocation failure deliberately": asking a container for more than max_size()
// throws length_error rather than allocating.
TEST(SecurityResourceLimits, OverMaxSizeThrowsLengthError) {
    std::vector<int> v;
    EXPECT_THROW(v.reserve(v.max_size() + 1), std::length_error);
}
