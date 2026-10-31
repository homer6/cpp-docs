// Checks claims made by docs/examples/language/coroutines/coroutines.md.
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>
#include <version>

#if __has_include(<generator>)
#include <generator>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

#ifdef __cpp_lib_generator
namespace {
std::generator<int> fibonacci() {  // the page's example
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        std::tie(a, b) = std::tuple{b, a + b};
    }
}

int produced = 0;
std::generator<int> counting() {
    for (int i = 0;; ++i) {
        ++produced;
        co_yield i;
    }
}
}  // namespace
#endif

// "std::generator (C++23)": the first ten Fibonacci numbers are 0 1 1 2 3 5 8 13 21 34.
TEST(Coroutines, GeneratorFibonacciFirstTen) {
    CPPDOCS_REQUIRE(__cpp_lib_generator);
#ifdef __cpp_lib_generator
    std::vector<int> got;
    for (int x : fibonacci() | std::views::take(10)) got.push_back(x);
    EXPECT_EQ(got, (std::vector<int>{0, 1, 1, 2, 3, 5, 8, 13, 21, 34}));
#endif
}

// "std::generator (C++23)": lazy — values are computed on demand. take(n) stops the
// loop after n values, but its counted_iterator's last ++ resumes the coroutine
// once more, so n + 1 values are produced (and the n + 1th is never seen).
TEST(Coroutines, GeneratorIsLazy) {
    CPPDOCS_REQUIRE(__cpp_lib_generator);
#ifdef __cpp_lib_generator
    produced = 0;
    auto g = counting();
    EXPECT_EQ(produced, 0);  // nothing runs until iteration starts
    int n = 0;
    for (int x : std::move(g) | std::views::take(3)) n += x;  // a generator is move-only
    EXPECT_EQ(n, 0 + 1 + 2);
    EXPECT_EQ(produced, 4);
#endif
}

// "Gotchas": std::generator is single-pass (an input range, not a forward range).
TEST(Coroutines, GeneratorIsSinglePass) {
    CPPDOCS_REQUIRE(__cpp_lib_generator);
#ifdef __cpp_lib_generator
    static_assert(std::ranges::input_range<std::generator<int>>);
    static_assert(!std::ranges::forward_range<std::generator<int>>);
#endif
}
