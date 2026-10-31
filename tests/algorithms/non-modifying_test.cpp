// Checks claims made by docs/examples/algorithms/non-modifying.md.
#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

// "Searching": find / find_if / find_if_not return the first match or end().
TEST(NonModifying, FindFamily) {
    const std::vector<int> v{5, 42, -1, 12, 42};
    EXPECT_EQ(std::find(v.begin(), v.end(), 42) - v.begin(), 1);
    EXPECT_EQ(std::find(v.begin(), v.end(), 7), v.end());
    EXPECT_EQ(std::find_if(v.begin(), v.end(), [](int x) { return x > 10; }) - v.begin(), 1);
    auto pred = [](int x) { return x >= 0; };
    EXPECT_EQ(std::find_if_not(v.begin(), v.end(), pred) - v.begin(), 2);
}

// "Searching": search finds a subrange, find_first_of any of a set, adjacent_find an equal pair.
TEST(NonModifying, SubsequenceAndSetSearches) {
    const std::string hay = "a needle in a haystack", needle = "needle";
    EXPECT_EQ(std::search(hay.begin(), hay.end(), needle.begin(), needle.end()) - hay.begin(), 2);

    const std::vector<int> v{1, 2, 3, 3, 4}, set{9, 4, 3};
    EXPECT_EQ(std::find_first_of(v.begin(), v.end(), set.begin(), set.end()) - v.begin(), 2);
    EXPECT_EQ(std::adjacent_find(v.begin(), v.end()) - v.begin(), 2);
}

// "Counting & testing": results on the page's v{7, 1, 7, 4} with pred x > 3.
TEST(NonModifying, CountingAndTesting) {
    std::vector<int> v{7, 1, 7, 4};
    auto pred = [](int x) { return x > 3; };
    EXPECT_EQ(std::count(v.begin(), v.end(), 7), 2);
    EXPECT_EQ(std::count_if(v.begin(), v.end(), pred), 3);
    EXPECT_FALSE(std::all_of(v.begin(), v.end(), pred));
    EXPECT_TRUE(std::any_of(v.begin(), v.end(), pred));
    EXPECT_FALSE(std::none_of(v.begin(), v.end(), pred));
}

// "Comparing ranges": a{1,2,3} vs b{1,2,4} — not equal, mismatch at index 2, a < b.
TEST(NonModifying, ComparingRanges) {
    std::vector<int> a{1, 2, 3}, b{1, 2, 4};
    EXPECT_FALSE(std::equal(a.begin(), a.end(), b.begin()));
    EXPECT_FALSE(std::equal(a.begin(), a.end(), b.begin(), b.end()));
    auto [i, j] = std::mismatch(a.begin(), a.end(), b.begin());
    EXPECT_EQ(i - a.begin(), 2);
    EXPECT_EQ(*i, 3);
    EXPECT_EQ(*j, 4);
    EXPECT_TRUE(std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()));
}

// "Gotchas": the 4-iterator equal/mismatch respect the second range's end.
TEST(NonModifying, FourIteratorFormsCheckBothLengths) {
    std::vector<int> a{1, 2, 3}, shorter{1, 2};
    EXPECT_FALSE(std::equal(a.begin(), a.end(), shorter.begin(), shorter.end()));
    auto [i, j] = std::mismatch(a.begin(), a.end(), shorter.begin(), shorter.end());
    EXPECT_EQ(i - a.begin(), 2);
    EXPECT_EQ(j, shorter.end());
}

// "Traversal": for_each_n applies fn to the first 5 elements and returns the next position.
TEST(NonModifying, ForEachN) {
    std::vector<int> v{1, 2, 3, 4, 5, 6};
    auto fn = [](int& x) { x *= 2; };
    auto it = std::for_each_n(v.begin(), 5, fn);
    EXPECT_EQ(v, (std::vector<int>{2, 4, 6, 8, 10, 6}));
    EXPECT_EQ(it - v.begin(), 5);
}

// "Gotchas": find on a sorted range is still linear; binary_search is O(log n).
TEST(NonModifying, FindIsLinearBinarySearchLogarithmic) {
    std::vector<int> v(1024);
    std::iota(v.begin(), v.end(), 0);
    int eq = 0;
    auto it = std::find_if(v.begin(), v.end(), [&](int x) { ++eq; return x == 1023; });
    EXPECT_EQ(it - v.begin(), 1023);
    EXPECT_EQ(eq, 1024);  // visited every element

    int cmp = 0;
    auto less = [&](int a, int b) { ++cmp; return a < b; };
    EXPECT_TRUE(std::binary_search(v.begin(), v.end(), 1023, less));
    EXPECT_LE(cmp, 2 + static_cast<int>(std::log2(v.size())));  // log2(N) + O(1)
}

// "Gotchas": associative containers' member find is O(log n).
TEST(NonModifying, SetMemberFindIsLogarithmic) {
    int cmp = 0;
    auto less = [&](int a, int b) { ++cmp; return a < b; };
    std::set<int, decltype(less)> s(less);
    for (int i = 0; i < 1024; ++i) s.insert(i);
    cmp = 0;
    EXPECT_NE(s.find(1023), s.end());
    // A red-black tree's height is at most 2*log2(n+1) = 22 here; libstdc++ makes about one
    // comparison per level, libc++ up to two. Either way, nowhere near the 1024 of a linear scan.
    EXPECT_LE(cmp, 2 * 22 + 1);
}
