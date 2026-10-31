// Checks claims made by docs/examples/algorithms/modifying.md.
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

#include <gtest/gtest.h>

// "Copy / move / transform": copy appends, copy_if filters, transform maps and zips.
TEST(Modifying, CopyAndTransform) {
    std::vector<int> in{1, 2, 3, 4}, out;
    std::copy(in.begin(), in.end(), std::back_inserter(out));
    EXPECT_EQ(out, in);
    std::copy_if(in.begin(), in.end(), std::back_inserter(out), [](int x) { return x % 2 == 0; });
    EXPECT_EQ(out, (std::vector<int>{1, 2, 3, 4, 2, 4}));

    std::vector<int> sq(in.size());
    std::transform(in.begin(), in.end(), sq.begin(), [](int x) { return x * x; });
    EXPECT_EQ(sq, (std::vector<int>{1, 4, 9, 16}));
    std::vector<int> a{1, 2, 3}, b{10, 20, 30}, sum(3);
    std::transform(a.begin(), a.end(), b.begin(), sum.begin(), std::plus<>{});
    EXPECT_EQ(sum, (std::vector<int>{11, 22, 33}));
}

// "Gotchas": transform can write in place (out == in).
TEST(Modifying, TransformInPlace) {
    std::vector<int> v{1, 2, 3};
    std::transform(v.begin(), v.end(), v.begin(), [](int x) { return -x; });
    EXPECT_EQ(v, (std::vector<int>{-1, -2, -3}));
}

// "Gotchas": the std::move algorithm leaves the sources moved-from.
TEST(Modifying, MoveAlgorithmLeavesSourcesMovedFrom) {
    std::vector<std::unique_ptr<int>> in, out(2);
    in.push_back(std::make_unique<int>(1));
    in.push_back(std::make_unique<int>(2));
    std::move(in.begin(), in.end(), out.begin());
    EXPECT_EQ(*out[0], 1);
    EXPECT_EQ(*out[1], 2);
    EXPECT_EQ(in.size(), 2u);  // sources still there, but moved-from (null for unique_ptr)
    EXPECT_EQ(in[0], nullptr);
    EXPECT_EQ(in[1], nullptr);
}

// "Fill / generate": fill, fill_n, generate gives 0,1,2,..., iota gives 1,2,3,...
TEST(Modifying, FillAndGenerate) {
    std::vector<int> v(10);
    std::fill(v.begin(), v.end(), 0);
    EXPECT_EQ(v, std::vector<int>(10, 0));
    std::fill_n(v.begin(), 5, -1);
    EXPECT_EQ(v, (std::vector<int>{-1, -1, -1, -1, -1, 0, 0, 0, 0, 0}));
    std::generate(v.begin(), v.end(), [n = 0]() mutable { return n++; });
    EXPECT_EQ(v, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    std::iota(v.begin(), v.end(), 1);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
}

// "Replace": every 0 becomes -1, then every negative becomes 0.
TEST(Modifying, Replace) {
    std::vector<int> v{0, 5, 0, -3};
    std::replace(v.begin(), v.end(), 0, -1);
    EXPECT_EQ(v, (std::vector<int>{-1, 5, -1, -3}));
    std::replace_if(v.begin(), v.end(), [](int x) { return x < 0; }, 0);
    EXPECT_EQ(v, (std::vector<int>{0, 5, 0, 0}));
}

// "Remove": remove keeps the size, moves kept elements to the front and returns the new end.
TEST(Modifying, RemoveDoesNotShrink) {
    std::vector<int> v{0, 1, 0, 2, -3};
    auto end = std::remove(v.begin(), v.end(), 0);
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(end - v.begin(), 3);
    EXPECT_EQ((std::vector<int>(v.begin(), end)), (std::vector<int>{1, 2, -3}));
}

// "Remove — and the erase-remove idiom": erase-remove and C++20 std::erase / erase_if.
TEST(Modifying, EraseRemoveIdiom) {
    auto pred = [](int x) { return x < 0; };
    std::vector<int> v{0, 1, 0, 2, -3};
    v.erase(std::remove(v.begin(), v.end(), 0), v.end());
    EXPECT_EQ(v, (std::vector<int>{1, 2, -3}));
    v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
    EXPECT_EQ(v, (std::vector<int>{1, 2}));

    std::vector<int> w{0, 1, 0, 2, -3};
    EXPECT_EQ(std::erase(w, 0), 2u);  // returns how many were erased
    EXPECT_EQ(std::erase_if(w, pred), 1u);
    EXPECT_EQ(w, (std::vector<int>{1, 2}));
}

// "unique": collapses only consecutive duplicates; sorting first removes all of them.
TEST(Modifying, UniqueOnlyAdjacent) {
    std::vector<int> v{3, 1, 3, 2, 1};
    auto unsorted = v;
    EXPECT_EQ(std::unique(unsorted.begin(), unsorted.end()), unsorted.end());  // no adjacent dups

    std::sort(v.begin(), v.end());
    auto end = std::unique(v.begin(), v.end());
    EXPECT_EQ(v.size(), 5u);  // unique only marks, never resizes
    v.erase(end, v.end());
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
}

// "Reorder": reverse, rotate brings element 3 to the front, shuffle permutes, sample picks 5.
TEST(Modifying, Reorder) {
    std::vector<int> v{0, 1, 2, 3, 4, 5, 6, 7};
    std::reverse(v.begin(), v.end());
    EXPECT_EQ(v, (std::vector<int>{7, 6, 5, 4, 3, 2, 1, 0}));
    std::reverse(v.begin(), v.end());
    std::rotate(v.begin(), v.begin() + 3, v.end());
    EXPECT_EQ(v, (std::vector<int>{3, 4, 5, 6, 7, 0, 1, 2}));

    std::mt19937 rng{42};
    auto before = v;
    std::shuffle(v.begin(), v.end(), rng);
    EXPECT_TRUE(std::is_permutation(v.begin(), v.end(), before.begin(), before.end()));

    const std::vector<int> in{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> out;
    std::sample(in.begin(), in.end(), std::back_inserter(out), 5, rng);
    ASSERT_EQ(out.size(), 5u);
    std::sort(out.begin(), out.end());
    EXPECT_EQ(std::adjacent_find(out.begin(), out.end()), out.end());  // 5 distinct picks
    EXPECT_TRUE(std::includes(in.begin(), in.end(), out.begin(), out.end()));
}
