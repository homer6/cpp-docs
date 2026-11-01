// Checks claims made by docs/examples/algorithms/numeric.md.
#include <algorithm>
#include <climits>
#include <functional>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>
#include <version>
#if __has_include(<execution>)
#include <execution>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "accumulate — the classic left fold": sum, product, string join (left to right).
TEST(Numeric, AccumulateFolds) {
    std::vector<int> v{1, 2, 3, 4};
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 10);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 1, std::multiplies<>{}), 24);
    std::vector<std::string> words{"a", "b", "c"};
    auto joined = std::accumulate(words.begin(), words.end(), std::string{},
                                  [](std::string acc, const std::string& w) { return acc + w; });
    EXPECT_EQ(joined, "abc");
}

// "accumulate": the init value's type drives the accumulation; 0 over doubles truncates.
TEST(Numeric, InitTypeDrivesAccumulation) {
    std::vector<double> d{0.5, 0.5, 0.5, 0.5};
    auto as_int = std::accumulate(d.begin(), d.end(), 0);
    static_assert(std::is_same_v<decltype(as_int), int>);
    EXPECT_EQ(as_int, 0);  // each step truncates 0 + 0.5 back to int 0
    EXPECT_DOUBLE_EQ(std::accumulate(d.begin(), d.end(), 0.0), 2.0);
}

// "Gotchas": pick a wide enough init type to avoid overflow.
TEST(Numeric, WideInitAvoidsOverflow) {
    std::vector<int> v{INT_MAX, INT_MAX};
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0LL), 2LL * INT_MAX);
}

// "reduce / transform_reduce": sum, dot product, count of positives.
TEST(Numeric, ReduceAndTransformReduce) {
    std::vector<int> v{-1, 2, 3, -4, 5};
    EXPECT_EQ(std::reduce(v.begin(), v.end()), 5);
    std::vector<double> a{1, 2, 3}, b{4, 5, 6};
    EXPECT_DOUBLE_EQ(std::transform_reduce(a.begin(), a.end(), b.begin(), 0.0), 32.0);
    EXPECT_DOUBLE_EQ(std::inner_product(a.begin(), a.end(), b.begin(), 0.0), 32.0);
    int n = std::transform_reduce(v.begin(), v.end(), 0, std::plus<>{}, [](int x) { return x > 0; });
    EXPECT_EQ(n, 3);
}

// "reduce / transform_reduce": the parallel overload with std::execution::par.
TEST(Numeric, ParallelReduce) {
    CPPDOCS_REQUIRE(__cpp_lib_parallel_algorithm);
#ifdef __cpp_lib_parallel_algorithm
    std::vector<int> v(1000);
    std::iota(v.begin(), v.end(), 1);
    EXPECT_EQ(std::reduce(std::execution::par, v.begin(), v.end()), 500500);
#endif
}

// "Gotchas": accumulate is a strict left fold, so non-associative ops give a defined result.
TEST(Numeric, AccumulateKeepsOrder) {
    std::vector<int> v{1, 2, 3};
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 10, std::minus<>{}), ((10 - 1) - 2) - 3);
}

// "Scans — running totals": inclusive, exclusive, adjacent difference.
TEST(Numeric, Scans) {
    std::vector<int> v{1, 2, 3, 4};
    std::vector<int> out(v.size());
    std::partial_sum(v.begin(), v.end(), out.begin());
    EXPECT_EQ(out, (std::vector<int>{1, 3, 6, 10}));
    std::inclusive_scan(v.begin(), v.end(), out.begin());
    EXPECT_EQ(out, (std::vector<int>{1, 3, 6, 10}));
    std::exclusive_scan(v.begin(), v.end(), out.begin(), 0);
    EXPECT_EQ(out, (std::vector<int>{0, 1, 3, 6}));
    std::adjacent_difference(v.begin(), v.end(), out.begin());
    EXPECT_EQ(out, (std::vector<int>{1, 1, 1, 1}));  // out[0] = v[0]
}

// "Small but handy": iota, gcd, lcm, midpoint (overflow-safe).
TEST(Numeric, SmallButHandy) {
    std::vector<int> v(5);
    std::iota(v.begin(), v.end(), 1);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3, 4, 5}));
    EXPECT_EQ(std::gcd(12, 18), 6);
    EXPECT_EQ(std::lcm(4, 6), 12);
    EXPECT_EQ(std::midpoint(7, 10), 8);  // rounds toward the first argument
    EXPECT_EQ(std::midpoint(INT_MAX - 2, INT_MAX), INT_MAX - 1);  // (a+b)/2 would overflow
}

// "Gotchas": accumulate(doubles, 0) works in int; ranges::fold_left deduces double from the operation.
TEST(Numeric, FoldLeftDeducesResultType) {
    std::vector<double> d{0.5, 0.5};
    EXPECT_EQ(std::accumulate(d.begin(), d.end(), 0), 0);
    auto f = std::ranges::fold_left(d, 0, std::plus<>{});
    static_assert(std::is_same_v<decltype(f), double>);
    EXPECT_EQ(f, 1.0);
}
