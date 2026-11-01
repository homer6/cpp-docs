// Checks claims made by docs/examples/algorithms/sorting-searching.md.
#include <algorithm>
#include <functional>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <list>
#include <numeric>
#include <random>
#include <ranges>
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

namespace {
struct Player { int score; };

std::vector<int> shuffled(int n) {
    std::vector<int> v(n);
    std::iota(v.begin(), v.end(), 0);
    std::shuffle(v.begin(), v.end(), std::mt19937{42});
    return v;
}
}  // namespace

// "Sorting": sort by a field with a lambda; is_sorted checks the result.
TEST(SortingSearching, SortByFieldAndIsSorted) {
    std::vector<Player> players{{30}, {10}, {20}};
    std::sort(players.begin(), players.end(), [](auto& a, auto& b) { return a.score < b.score; });
    EXPECT_EQ(players[0].score, 10);
    EXPECT_EQ(players[2].score, 30);
    std::vector<int> v{3, 1, 2};
    EXPECT_FALSE(std::is_sorted(v.begin(), v.end()));
    std::sort(v.begin(), v.end());
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

// "Partial / selection": nth_element is O(n) — far fewer comparisons than a full sort.
TEST(SortingSearching, NthElementIsLinear) {
    constexpr int n = 100000;
    long nth_calls = 0, sort_calls = 0;
    auto v = shuffled(n);
    std::nth_element(v.begin(), v.begin() + n / 2, v.end(),
                     [&](int a, int b) { ++nth_calls; return a < b; });
    EXPECT_EQ(v[n / 2], n / 2);  // the median of 0..n-1
    v = shuffled(n);
    std::sort(v.begin(), v.end(), [&](int a, int b) { ++sort_calls; return a < b; });
    EXPECT_GT(sort_calls, 10L * n);           // ~n log2 n (log2 100000 ~ 17)
#if defined(_MSVC_STL_VERSION) && _ITERATOR_DEBUG_LEVEL == 2
    (void)nth_calls;
    GTEST_SKIP() << "MS STL debug iterators call the comparator a second time to validate it "
                    "(_DEBUG_LT_PRED), so nth_element's count isn't the algorithm's";
#else
    EXPECT_LT(nth_calls, 5L * n);             // a small multiple of n
#endif
}

// "Binary search": O(log n) — about log2(n) comparisons on a sorted range.
TEST(SortingSearching, BinarySearchIsLogarithmic) {
    std::vector<int> v(1 << 20);
    std::iota(v.begin(), v.end(), 0);
    long calls = 0;
    auto it = std::lower_bound(v.begin(), v.end(), 123456, [&](int a, int b) { ++calls; return a < b; });
    EXPECT_EQ(*it, 123456);
    EXPECT_LE(calls, 21);  // log2(2^20) + 1
}

// "Gotchas": binary search on unsorted input gives a wrong answer, with no error.
TEST(SortingSearching, UnsortedInputGivesWrongAnswer) {
    std::vector<int> v{5, 1, 4, 2, 3};
    EXPECT_FALSE(std::binary_search(v.begin(), v.end(), 1));  // 1 is present
}

// "Gotchas": std::sort needs random-access iterators; list has a member sort instead.
TEST(SortingSearching, SortNeedsRandomAccess) {
    static_assert(!std::random_access_iterator<std::list<int>::iterator>);
    static_assert(!std::invocable<decltype(std::ranges::sort), std::list<int>&>);
    static_assert(std::invocable<decltype(std::ranges::sort), std::vector<int>&>);
    std::list<int> l{3, 1, 2};
    l.sort();
    EXPECT_EQ(l, (std::list<int>{1, 2, 3}));
}

// "Gotchas": binary_search only says yes/no; lower_bound + *it == x gives the position.
TEST(SortingSearching, LowerBoundForPosition) {
    std::vector<int> v{1, 2, 2, 2, 5};
    auto it = std::lower_bound(v.begin(), v.end(), 5);
    ASSERT_NE(it, v.end());
    EXPECT_EQ(*it, 5);
    EXPECT_EQ(it - v.begin(), 4);
    auto miss = std::lower_bound(v.begin(), v.end(), 3);
    EXPECT_NE(*miss, 3);  // not found: lands on the next larger element
}

// "Gotchas": after nth_element neither side is necessarily sorted (checked on a large range).
TEST(SortingSearching, NthElementSidesNotSorted) {
    constexpr int n = 1000;
    auto v = shuffled(n);
    auto nth = v.begin() + n / 2;
    std::nth_element(v.begin(), nth, v.end());
    EXPECT_TRUE(std::all_of(v.begin(), nth, [&](int x) { return x <= *nth; }));
    // Not required by the standard either way, but on these toolchains it holds.
    EXPECT_FALSE(std::is_sorted(v.begin(), nth) && std::is_sorted(nth, v.end()));
}
