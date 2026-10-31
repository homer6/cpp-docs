// Checks claims made by docs/examples/iterators/operations.md.
#include <concepts>
#include <cstddef>
#include <forward_list>
#include <iterator>
#include <list>
#include <set>
#include <sstream>
#include <ranges>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {
template <class I>
concept HasPlusInt = requires(I i) { i + 2; };
}  // namespace

// "Moving and measuring": advance mutates, next/prev don't, distance counts steps.
TEST(IteratorOperations, AdvanceNextPrevDistance) {
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto it = v.begin();
    std::advance(it, 3);
    EXPECT_EQ(*it, 4);
    EXPECT_EQ(*std::next(it), 5);
    EXPECT_EQ(*std::next(it, 5), 9);
    EXPECT_EQ(*std::prev(it), 3);
    EXPECT_EQ(*it, 4);  // next/prev left it alone
    EXPECT_EQ(std::distance(v.begin(), v.end()), 10);
}

// "Gotchas": next works on forward-only iterators where + doesn't compile.
TEST(IteratorOperations, NextWorksOnForwardIterators) {
    std::forward_list<int> fl{1, 2, 3};
    static_assert(!HasPlusInt<std::forward_list<int>::iterator>);
    static_assert(HasPlusInt<std::vector<int>::iterator>);
    EXPECT_EQ(*std::next(fl.begin(), 2), 3);
}

// "Swapping/moving the pointed-to elements": iter_swap swaps values; iter_move moves out.
TEST(IteratorOperations, IterSwapAndIterMove) {
    std::vector<std::string> v{"a", "b"};
    std::iter_swap(v.begin(), v.begin() + 1);
    EXPECT_EQ(v, (std::vector<std::string>{"b", "a"}));
    static_assert(std::same_as<decltype(std::ranges::iter_move(v.begin())), std::string&&>);
    auto val = std::ranges::iter_move(v.begin());  // moves "b" out of v[0]
    EXPECT_EQ(val, "b");
    EXPECT_EQ(v[1], "a");
}

// "std::iterator_traits": the associated types for vector<int>::iterator and int*.
TEST(IteratorOperations, IteratorTraits) {
    using It = std::vector<int>::iterator;
    static_assert(std::same_as<std::iterator_traits<It>::value_type, int>);
    static_assert(std::same_as<std::iterator_traits<It>::difference_type, std::ptrdiff_t>);
    static_assert(std::same_as<std::iterator_traits<It>::reference, int&>);
    static_assert(std::same_as<std::iterator_traits<It>::iterator_category,
                               std::random_access_iterator_tag>);
    static_assert(std::same_as<std::iterator_traits<int*>::value_type, int>);
    SUCCEED();
}

// "ranges::begin / end and sentinels": end may be a sentinel of a different type.
TEST(IteratorOperations, RangesSentinels) {
    std::vector<int> rng{1, 2, 3};
    EXPECT_EQ(std::ranges::distance(rng), 3);
    auto inf = std::views::iota(0);  // unbounded: end is unreachable_sentinel_t
    static_assert(!std::same_as<decltype(std::ranges::begin(inf)), decltype(std::ranges::end(inf))>);
}

// "Moving and measuring": next/prev step through list and set; advance goes back with a negative count.
TEST(IteratorOperations, BidirectionalStepping) {
    std::list<int> l{10, 20, 30, 40};
    auto it = std::next(l.begin(), 3);
    EXPECT_EQ(*it, 40);
    std::advance(it, -2);  // negative: bidirectional+
    EXPECT_EQ(*it, 20);
    std::set<int> s{5, 1, 3};
    EXPECT_EQ(*std::prev(s.end()), 5);
    EXPECT_EQ(std::distance(s.begin(), s.end()), 3);
}

// "Gotchas": distance on single-pass iterators consumes the input.
TEST(IteratorOperations, DistanceConsumesInputIterators) {
    std::istringstream in{"1 2 3"};
    std::istream_iterator<int> first{in}, last;
    EXPECT_EQ(std::distance(first, last), 3);
    std::istream_iterator<int> again{in};
    EXPECT_EQ(again, last);  // nothing left to read
}

// "ranges::begin / end and sentinels": default_sentinel ends a counted iterator after N.
TEST(IteratorOperations, DefaultSentinelEndsCountedIterator) {
    std::list<int> l{1, 2, 3, 4};
    std::counted_iterator it{l.begin(), 2};
    EXPECT_EQ(std::ranges::distance(it, std::default_sentinel), 2);
    std::ranges::advance(it, 2);
    EXPECT_TRUE(it == std::default_sentinel);
}
