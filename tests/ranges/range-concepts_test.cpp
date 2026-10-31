// Checks claims made by docs/examples/ranges/range-concepts.md.
#include <algorithm>
#include <concepts>
#include <forward_list>
#include <list>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

// "A range is anything with begin/end": containers, arrays, string_view, span and views are ranges.
TEST(RangeConcepts, RangeIsAnythingWithBeginEnd) {
    static_assert(std::ranges::range<std::vector<int>>);
    static_assert(std::ranges::range<int[4]>);
    static_assert(!std::ranges::range<int>);
    static_assert(std::ranges::range<std::string_view>);
    static_assert(std::ranges::range<std::span<int>>);
    static_assert(std::ranges::range<std::ranges::iota_view<int>>);
    // The end may be a sentinel of another type (an unbounded iota).
    static_assert(!std::ranges::common_range<std::ranges::iota_view<int>>);
    SUCCEED();
}

// "The refinement hierarchy": each row on standard types.
TEST(RangeConcepts, RefinementHierarchy) {
    static_assert(std::ranges::contiguous_range<std::vector<int>>);
    static_assert(std::ranges::bidirectional_range<std::list<int>>);
    static_assert(!std::ranges::random_access_range<std::list<int>>);
    static_assert(std::ranges::sized_range<std::vector<int>>);
    static_assert(!std::ranges::sized_range<std::forward_list<int>>);  // no O(1) size
    static_assert(std::ranges::common_range<std::vector<int>>);
    static_assert(std::ranges::borrowed_range<std::span<int>>);
    static_assert(std::ranges::borrowed_range<std::string_view>);
    static_assert(std::ranges::borrowed_range<std::vector<int>&>);
    static_assert(!std::ranges::borrowed_range<std::vector<int>>);
    static_assert(std::ranges::view<std::string_view>);
    static_assert(!std::ranges::view<std::vector<int>>);  // owning: not a view
    static_assert(std::ranges::viewable_range<std::vector<int>&>);
    SUCCEED();
}

// "Range access": the customization points on vector<int>{1, 2, 3}, and on a raw array.
TEST(RangeConcepts, RangeAccess) {
    std::vector<int> r{1, 2, 3};
    EXPECT_EQ(*std::ranges::begin(r), 1);
    EXPECT_EQ(std::ranges::end(r) - std::ranges::begin(r), 3);
    EXPECT_EQ(std::ranges::size(r), 3u);
    EXPECT_FALSE(std::ranges::empty(r));
    EXPECT_EQ(std::ranges::data(r), r.data());
    static_assert(std::same_as<decltype(*std::ranges::cbegin(r)), const int&>);
    EXPECT_EQ(std::ranges::cend(r) - std::ranges::cbegin(r), 3);

    int a[4]{};
    EXPECT_EQ(std::ranges::begin(a), a);  // works for arrays, which have no .begin()
    EXPECT_EQ(std::ranges::size(a), 4u);
}

// "view vs. container": a view owns nothing and is lazy; copying it copies no elements.
TEST(RangeConcepts, ViewVersusContainer) {
    std::vector<int> v{1, 2, 3, 4};
    int calls = 0;
    auto evens = v | std::views::filter([&](int x) { ++calls; return x % 2 == 0; });
    EXPECT_EQ(calls, 0);  // lazy: no work until iteration
    auto copy = evens;
    v[0] = 10;            // both the view and its copy see the change: they refer to v
    std::vector<int> a(evens.begin(), evens.end());
    std::vector<int> b(copy.begin(), copy.end());
    EXPECT_EQ(a, (std::vector<int>{10, 2, 4}));
    EXPECT_EQ(b, a);
}

// "Dangling protection": an algorithm over a temporary vector returns ranges::dangling; a borrowed range doesn't.
TEST(RangeConcepts, DanglingProtection) {
    static_assert(std::same_as<decltype(std::ranges::find(std::vector{1, 2, 3}, 2)), std::ranges::dangling>);
    std::vector<int> v{1, 2, 3};
    auto it = std::ranges::find(std::span{v}, 2);  // span temporary, but borrowed
    EXPECT_EQ(*it, 2);
    auto it2 = std::ranges::find(v, 3);            // lvalue container
    EXPECT_EQ(it2 - v.begin(), 2);
}

// "Gotchas": wrap a container with views::all (an lvalue becomes a ref_view) instead of making it a view.
TEST(RangeConcepts, ViewsAllWrapsContainers) {
    std::vector<int> v{1, 2, 3};
    auto all = std::views::all(v);
    static_assert(std::same_as<decltype(all), std::ranges::ref_view<std::vector<int>>>);
    static_assert(std::ranges::view<decltype(all)>);
    EXPECT_EQ(all.data(), v.data());
}
