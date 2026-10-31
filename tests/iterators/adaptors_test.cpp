// Checks claims made by docs/examples/iterators/adaptors.md.
#include <algorithm>
#include <array>
#include <concepts>
#include <iterator>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "support/capture.hpp"

namespace {
template <class C>
concept HasPushBack = requires(C c, typename C::value_type v) { c.push_back(v); };

// Records whether it was copied or moved from.
struct Tracked {
    std::string s;
    bool moved_from = false;
    explicit Tracked(std::string v) : s(std::move(v)) {}
    Tracked(const Tracked&) = default;
    Tracked(Tracked&& o) noexcept : s(std::move(o.s)) { o.moved_from = true; }
};
}  // namespace

// "Insert iterators": back_inserter appends, front_inserter prepends (reversing), inserter inserts.
TEST(IteratorAdaptors, InsertIterators) {
    const std::vector<int> in{3, 1, 2};
    std::vector<int> out{0};
    std::list<int> lst;
    std::set<int> s;
    std::copy(in.begin(), in.end(), std::back_inserter(out));
    std::copy(in.begin(), in.end(), std::front_inserter(lst));
    std::copy(in.begin(), in.end(), std::inserter(s, s.end()));
    EXPECT_EQ(out, (std::vector<int>{0, 3, 1, 2}));
    EXPECT_EQ(lst, (std::list<int>{2, 1, 3}));
    EXPECT_EQ(s, (std::set<int>{1, 2, 3}));
}

// "Gotchas": back_inserter needs push_back, which array and set don't have.
TEST(IteratorAdaptors, BackInserterNeedsPushBack) {
    static_assert(HasPushBack<std::vector<int>>);
    static_assert(!HasPushBack<std::set<int>>);
    static_assert(!HasPushBack<std::array<int, 3>>);
    SUCCEED();
}

// "reverse_iterator": rbegin..rend walks last to first; a reversed copy.
TEST(IteratorAdaptors, ReverseIterator) {
    std::vector<int> v{1, 2, 3};
    std::vector<int> seen;
    for (auto it = v.rbegin(); it != v.rend(); ++it) seen.push_back(*it);
    EXPECT_EQ(seen, (std::vector<int>{3, 2, 1}));
    std::vector<int> rev(v.rbegin(), v.rend());
    EXPECT_EQ(rev, (std::vector<int>{3, 2, 1}));
}

// "Gotchas": a reverse iterator points one before its base(): &*ri == &*(ri.base() - 1).
TEST(IteratorAdaptors, ReverseBaseIsOffByOne) {
    std::vector<int> v{1, 2, 3};
    auto ri = v.rbegin();
    EXPECT_EQ(*ri, 3);
    EXPECT_EQ(ri.base(), v.end());
    EXPECT_EQ(&*ri, &*(ri.base() - 1));
}

// "move_iterator": dereferencing yields an rvalue, so elements are moved, leaving the source moved-from.
TEST(IteratorAdaptors, MoveIterator) {
    static_assert(std::same_as<decltype(*std::make_move_iterator(std::declval<std::vector<int>::iterator>())), int&&>);
    std::vector<Tracked> src{Tracked{"a"}, Tracked{"b"}, Tracked{"c"}};
    std::vector<Tracked> dst;
    dst.reserve(3);
    std::copy(std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()),
              std::back_inserter(dst));
    ASSERT_EQ(dst.size(), 3u);
    EXPECT_EQ(dst[0].s, "a");
    EXPECT_EQ(dst[2].s, "c");
    EXPECT_TRUE(std::all_of(src.begin(), src.end(), [](const Tracked& t) { return t.moved_from; }));
}

// "Stream iterators": read ints until end of stream, write space-separated.
TEST(IteratorAdaptors, StreamIterators) {
    std::istringstream in{"1 2\n  3"};
    std::vector<int> nums{std::istream_iterator<int>{in}, std::istream_iterator<int>{}};
    EXPECT_EQ(nums, (std::vector<int>{1, 2, 3}));
    auto out = cppdocs::capture_stdout([&] {
        std::copy(nums.begin(), nums.end(), std::ostream_iterator<int>{std::cout, " "});
    });
    EXPECT_EQ(out, "1 2 3 ");  // the delimiter follows every element
}

// "Gotchas": istream_iterator skips whitespace and silently stops at the first failed extraction.
TEST(IteratorAdaptors, IstreamIteratorStopsOnBadInput) {
    std::istringstream in{"  4 5 x 6"};
    std::vector<int> nums{std::istream_iterator<int>{in}, std::istream_iterator<int>{}};
    EXPECT_EQ(nums, (std::vector<int>{4, 5}));
    EXPECT_TRUE(in.fail());
}

// "C++20 ranges-related iterators": counted_iterator + default_sentinel; common_iterator unifies the pair.
TEST(IteratorAdaptors, CountedAndCommonIterators) {
    std::list<int> l{1, 2, 3, 4, 5};
    std::counted_iterator first{l.begin(), 3};
    EXPECT_EQ(first.count(), 3);
    std::vector<int> taken;
    for (auto it = first; it != std::default_sentinel; ++it) taken.push_back(*it);
    EXPECT_EQ(taken, (std::vector<int>{1, 2, 3}));

    using CI = std::common_iterator<std::counted_iterator<std::list<int>::iterator>, std::default_sentinel_t>;
    std::vector<int> legacy(CI{first}, CI{std::default_sentinel});  // iterator-pair constructor
    EXPECT_EQ(legacy, taken);
}
