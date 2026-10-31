// Checks claims made by docs/examples/algorithms/sorting-searching.md.
#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

// "Sorting": ascending, descending with std::greater<>, stable_sort keeps equal elements in order.
TEST(SortingSearching, SortAndStableSort) {
    std::vector<int> v{3, 1, 2};
    std::sort(v.begin(), v.end());
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
    std::sort(v.begin(), v.end(), std::greater<>{});
    EXPECT_EQ(v, (std::vector<int>{3, 2, 1}));

    std::vector<std::pair<int, char>> p{{2, 'a'}, {1, 'b'}, {2, 'c'}, {1, 'd'}};
    std::stable_sort(p.begin(), p.end(), [](auto& a, auto& b) { return a.first < b.first; });
    EXPECT_EQ(p, (std::vector<std::pair<int, char>>{{1, 'b'}, {1, 'd'}, {2, 'a'}, {2, 'c'}}));
}

// "Partial / selection": partial_sort sorts the first 10; nth_element places the nth and partitions.
TEST(SortingSearching, PartialSortAndNthElement) {
    std::vector<int> v(100);
    std::iota(v.rbegin(), v.rend(), 0);  // 99..0
    std::partial_sort(v.begin(), v.begin() + 10, v.end());
    std::vector<int> top10(v.begin(), v.begin() + 10);
    EXPECT_EQ(top10, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));

    std::iota(v.rbegin(), v.rend(), 0);
    auto nth = v.begin() + 50;
    std::nth_element(v.begin(), nth, v.end());
    EXPECT_EQ(*nth, 50);
    EXPECT_TRUE(std::all_of(v.begin(), nth, [&](int x) { return x <= *nth; }));
    EXPECT_TRUE(std::all_of(nth, v.end(), [&](int x) { return x >= *nth; }));
}

// "Binary search": lower_bound is first >= x, upper_bound first > x, equal_range the run of x.
TEST(SortingSearching, BinarySearch) {
    std::vector<int> v{1, 2, 2, 2, 5};
    EXPECT_TRUE(std::binary_search(v.begin(), v.end(), 2));
    EXPECT_FALSE(std::binary_search(v.begin(), v.end(), 3));
    auto lo = std::lower_bound(v.begin(), v.end(), 2);
    auto hi = std::upper_bound(v.begin(), v.end(), 2);
    EXPECT_EQ(lo - v.begin(), 1);
    EXPECT_EQ(hi - v.begin(), 4);
    auto [a, b] = std::equal_range(v.begin(), v.end(), 2);
    EXPECT_EQ(a, lo);
    EXPECT_EQ(b, hi);
    // "lower_bound doubles as where would x go?"
    EXPECT_EQ(std::lower_bound(v.begin(), v.end(), 3) - v.begin(), 4);
}

// "Merge & set operations": results on the page's inputs.
TEST(SortingSearching, MergeAndSetOperations) {
    std::vector<int> a{1, 2, 3, 4}, b{2, 4};
    std::vector<int> out;
    std::merge(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
    EXPECT_EQ(out, (std::vector<int>{1, 2, 2, 3, 4, 4}));

    std::vector<int> v{1, 4, 7, 2, 5, 8};
    std::inplace_merge(v.begin(), v.begin() + 3, v.end());
    EXPECT_EQ(v, (std::vector<int>{1, 2, 4, 5, 7, 8}));

    std::vector<int> u, i, d;
    std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(u));
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(i));
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(d));
    EXPECT_EQ(u, (std::vector<int>{1, 2, 3, 4}));
    EXPECT_EQ(i, (std::vector<int>{2, 4}));
    EXPECT_EQ(d, (std::vector<int>{1, 3}));
    EXPECT_TRUE(std::includes(a.begin(), a.end(), b.begin(), b.end()));
}
