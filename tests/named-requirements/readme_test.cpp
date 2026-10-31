// Checks claims made by docs/examples/named-requirements/README.md.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <concepts>
#include <list>
#include <random>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <ranges>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

namespace {

class SpinLock {
    std::atomic_flag flag_;
public:
    void lock() noexcept { while (flag_.test_and_set(std::memory_order_acquire)) {} }
    void unlock() noexcept { flag_.clear(std::memory_order_release); }
};

struct Point {
    int x, y;
    bool operator==(const Point&) const = default;
};

}  // namespace

template <>
struct std::hash<Point> {
    std::size_t operator()(const Point& p) const noexcept {
        return std::hash<int>{}(p.x) * 31 + std::hash<int>{}(p.y);
    }
};

// "Satisfying BasicLockable": lock_guard works with any lock()/unlock() pair.
TEST(NamedRequirements, BasicLockableWithLockGuard) {
    SpinLock lock;
    int counter = 0;
    {
        std::lock_guard guard{lock};
        ++counter;
    }
    // Released by the guard: we can take it again without spinning forever.
    lock.lock();
    lock.unlock();
    EXPECT_EQ(counter, 1);
}

// "Satisfying Hash for an unordered container": duplicates collapse, size is 2.
TEST(NamedRequirements, HashForUnorderedSet) {
    std::unordered_set<Point> s{{1, 2}, {1, 2}, {3, 4}};
    EXPECT_EQ(s.size(), 2u);
    EXPECT_EQ(std::hash<Point>{}({1, 2}), std::hash<Point>{}({1, 2}));
}

// "Concepts and named requirements are not the same test": iota_view's iterator.
TEST(NamedRequirements, IotaIteratorIsRandomAccessButLegacyInput) {
    using It = std::ranges::iterator_t<std::ranges::iota_view<int, int>>;
    static_assert(std::random_access_iterator<It>);
    static_assert(std::is_same_v<std::iterator_traits<It>::reference, int>);
    static_assert(std::is_same_v<std::iterator_traits<It>::iterator_category,
                                 std::input_iterator_tag>);
    SUCCEED();
}

// "Gotchas": requirements apply when used — vector<unique_ptr<int>> works until copied.
TEST(NamedRequirements, VectorOfMoveOnlyIsFineUntilCopied) {
    std::vector<std::unique_ptr<int>> v;
    v.push_back(std::make_unique<int>(7));
    auto moved = std::move(v);
    EXPECT_EQ(*moved[0], 7);
    static_assert(!std::is_copy_constructible_v<std::unique_ptr<int>>);
}

// "Same requirement, two eras of checking": ranges::sort is concept-checked,
// so a list's iterators are rejected at the call (detectable, unlike std::sort).
template <class R>
concept RangesSortable = requires(R& r) { std::ranges::sort(r); };
TEST(NamedRequirements, RangesSortRejectsList) {
    static_assert(!RangesSortable<std::list<int>>);
    static_assert(RangesSortable<std::vector<int>>);
    static_assert(!std::random_access_iterator<std::list<int>::iterator>);
}

// "Gotchas": satisfying a concept is not modeling it — any compiling == satisfies
// equality_comparable, even one that isn't an equivalence relation.
TEST(NamedRequirements, SatisfyingIsNotModeling) {
    struct Weird {
        bool operator==(const Weird&) const { return false; }  // not reflexive
    };
    static_assert(std::equality_comparable<Weird>);
    EXPECT_FALSE(Weird{} == Weird{});
}

// "The main groups": concept/trait counterparts for random numbers and clocks.
TEST(NamedRequirements, ConceptCounterparts) {
    static_assert(std::uniform_random_bit_generator<std::mt19937>);
    static_assert(std::chrono::is_clock_v<std::chrono::steady_clock>);
    static_assert(!std::chrono::is_clock_v<int>);
    static_assert(std::default_initializable<Point> && std::copy_constructible<Point>);
    SUCCEED();
}
