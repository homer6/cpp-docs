// Checks claims made by docs/examples/language/statements/iteration.md.
#include <algorithm>
#include <map>
#if __has_include(<print>)  // GCC 13 has no <print>
#include <print>
#endif
#include <ranges>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/capture.hpp"
#include "support/feature.hpp"

namespace {
bool owner_alive = false;
struct Owner {  // a temporary that owns the range it hands out
    std::vector<int> v{1, 2, 3};
    Owner() { owner_alive = true; }
    ~Owner() { owner_alive = false; }
    const std::vector<int>& items() const { return v; }
};
[[maybe_unused]] Owner make_owner() { return {}; }  // unused before C++23's P2718
}  // namespace

// "range-for": auto& mutates in place, init-statement, structured bindings over a map.
TEST(Iteration, RangeFor) {
    std::vector<int> container{1, 2, 3};
    for (auto& x : container) x *= 2;
    EXPECT_EQ(container, (std::vector<int>{2, 4, 6}));

    auto make = [] { return std::vector<int>{1, 2, 3}; };
    int total = 0;
    for (auto vec = make(); const auto& x : vec) total += x;  // C++20 init-statement
    EXPECT_EQ(total, 6);

    std::map<std::string, int> m{{"one", 1}, {"two", 2}};
    std::string seen;
    for (const auto& [key, value] : m) seen += key + std::to_string(value) + " ";
    EXPECT_EQ(seen, "one1 two2 ");
}

// "Classic for / while / do-while": while runs zero times, do-while at least once.
TEST(Iteration, ClassicLoops) {
    int n = 3, fors = 0, whiles = 0, dos = 0;
    bool cond = false;
    for (int i = 0; i < n; ++i) ++fors;
    while (cond) ++whiles;
    do ++dos; while (cond);
    EXPECT_EQ(fors, 3);
    EXPECT_EQ(whiles, 0);
    EXPECT_EQ(dos, 1);
}

// "break / continue": negatives are skipped, the loop stops at the first x > 100.
TEST(Iteration, BreakContinue) {
    std::vector<int> xs{1, -2, 3, 500, 4};
    auto skip = [](int x) { return x < 0; };
    auto done = [](int x) { return x > 100; };
    std::vector<int> visited;
    for (auto& x : xs) {
        if (skip(x)) continue;
        if (done(x)) break;
        visited.push_back(x);
    }
    EXPECT_EQ(visited, (std::vector<int>{1, 3}));
}

// "template for": one body per tuple element, each with its own type (C++26).
TEST(Iteration, ExpansionStatement) {
    CPPDOCS_REQUIRE(__cpp_expansion_statements);
    CPPDOCS_REQUIRE(__cpp_lib_print);
#if defined(__cpp_expansion_statements) && defined(__cpp_lib_print)
    std::tuple<int, std::string, double> t{1, "two", 3.0};
    std::string types;
    auto out = cppdocs::capture_stdout([&] {
        template for (const auto& elem : t) {
            std::println("{}", elem);
            types += std::is_same_v<std::remove_cvref_t<decltype(elem)>, std::string> ? "s" : "n";
        }
    });
    EXPECT_EQ(out, "1\ntwo\n3\n");
    EXPECT_EQ(types, "nsn");
#endif
}

// "Prefer algorithms over hand loops": any_of and a filter view.
TEST(Iteration, AlgorithmsOverLoops) {
    std::vector<int> xs{1, 2, 3, 4};
    auto is_bad = [](int n) { return n < 0; };
    EXPECT_FALSE(std::ranges::any_of(xs, is_bad));
    auto evens = xs | std::views::filter([](int n) { return n % 2 == 0; });
    EXPECT_TRUE(std::ranges::equal(evens, std::vector<int>{2, 4}));
}

// "Gotchas": C++23 (P2718) keeps temporaries in the range-initializer alive for the loop.
TEST(Iteration, TemporaryRangeLifetimeExtended) {
    CPPDOCS_REQUIRE_VERSION(__cpp_range_based_for, 202211L);
#if __cpp_range_based_for >= 202211L
    int sum = 0;
    for (int x : make_owner().items()) {
        EXPECT_TRUE(owner_alive);  // before C++23 the Owner was already destroyed here
        sum += x;
    }
    EXPECT_FALSE(owner_alive);
    EXPECT_EQ(sum, 6);
#endif
}
