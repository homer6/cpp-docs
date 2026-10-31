// Checks claims made by docs/examples/ranges/factories.md.
#include <concepts>
#include <map>
#include <ranges>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace rv = std::views;

namespace {
template <class R>
std::vector<int> collect(R&& r) {
    std::vector<int> out;
    for (int x : r) out.push_back(x);
    return out;
}
}  // namespace

// "Factories": iota bounded and unbounded, single, empty.
TEST(RangeFactories, IotaSingleEmpty) {
    auto a = rv::iota(0);
    static_assert(std::same_as<decltype(a.end()), std::unreachable_sentinel_t>);  // infinite
    static_assert(!std::ranges::sized_range<decltype(a)>);
    EXPECT_EQ(collect(a | rv::take(4)), (std::vector<int>{0, 1, 2, 3}));
    EXPECT_EQ(collect(rv::iota(1, 5)), (std::vector<int>{1, 2, 3, 4}));
    EXPECT_EQ(collect(rv::single(42)), (std::vector<int>{42}));
    auto d = rv::empty<int>;
    EXPECT_TRUE(d.empty());
    EXPECT_EQ(d.size(), 0u);
}

// "Factories": repeat(7) is infinite, repeat(7, 3) is 7,7,7 (C++23).
TEST(RangeFactories, Repeat) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_repeat);
#ifdef __cpp_lib_ranges_repeat
    EXPECT_EQ(collect(rv::repeat(7) | rv::take(4)), (std::vector<int>{7, 7, 7, 7}));
    static_assert(!std::ranges::sized_range<decltype(rv::repeat(7))>);
    EXPECT_EQ(collect(rv::repeat(7, 3)), (std::vector<int>{7, 7, 7}));
#endif
}

// "Factories": views::istream reads ints lazily and is single-pass (an input range).
TEST(RangeFactories, IstreamIsLazySinglePass) {
    std::istringstream in{"1 2 3"};
    auto g = rv::istream<int>(in);
    static_assert(std::ranges::input_range<decltype(g)>);
    static_assert(!std::ranges::forward_range<decltype(g)>);
    EXPECT_EQ(collect(g), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(collect(g), (std::vector<int>{}));  // can't restart it
}

// "Factories": the first 10 squares, lazily, from iota | transform | take.
TEST(RangeFactories, FirstTenSquares) {
    auto sq = rv::iota(1) | rv::transform([](int n) { return n * n; }) | rv::take(10);
    EXPECT_EQ(collect(sq), (std::vector<int>{1, 4, 9, 16, 25, 36, 49, 64, 81, 100}));
}

// "ranges::to": vector deduced from filter, vector<int> of squares, and a map from pairs (C++23).
TEST(RangeFactories, RangesTo) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_to_container);
#ifdef __cpp_lib_ranges_to_container
    std::vector<int> data{1, 2, 3, 4};
    std::vector<std::pair<int, std::string>> pairs{{1, "one"}, {2, "two"}};
    auto evens = data | rv::filter([](int x) { return x % 2 == 0; }) | std::ranges::to<std::vector>();
    static_assert(std::same_as<decltype(evens), std::vector<int>>);
    EXPECT_EQ(evens, (std::vector<int>{2, 4}));

    auto squared = rv::iota(1, 6) | rv::transform([](int n) { return n * n; })
                 | std::ranges::to<std::vector<int>>();
    EXPECT_EQ(squared, (std::vector<int>{1, 4, 9, 16, 25}));

    auto m = pairs | std::ranges::to<std::map<int, std::string>>();
    EXPECT_EQ(m, (std::map<int, std::string>{{1, "one"}, {2, "two"}}));
#endif
}

// "Gotchas": materializing runs the pipeline once; reusing the container doesn't re-run it.
TEST(RangeFactories, MaterializingRunsPipelineOnce) {
    int calls = 0;
    auto view = rv::iota(1, 6) | rv::transform([&](int n) { ++calls; return n * n; });
    std::vector<int> out(view.begin(), view.end());  // pre-C++23 way: needs a common_range
    static_assert(std::ranges::common_range<decltype(view)>);
    EXPECT_EQ(calls, 5);
    EXPECT_EQ(out.back(), 25);
    EXPECT_EQ(calls, 5);
}

// "Gotchas": a non-common pipeline is wrapped with views::common for the iterator-pair constructor.
TEST(RangeFactories, CommonForLegacyConstructor) {
    auto view = rv::iota(1) | rv::take_while([](int n) { return n < 4; });
    static_assert(!std::ranges::common_range<decltype(view)>);
    auto c = view | rv::common;
    std::vector<int> out(c.begin(), c.end());
    EXPECT_EQ(out, (std::vector<int>{1, 2, 3}));
}
