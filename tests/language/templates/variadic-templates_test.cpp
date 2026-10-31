// Checks claims made by docs/examples/language/templates/variadic-templates.md.
#include <iostream>
#include <utility>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "support/capture.hpp"
#include "support/feature.hpp"

namespace {
template <class... Args> constexpr auto count(Args&&... args) {
    static_assert(sizeof...(Args) == sizeof...(args));
    return sizeof...(args);
}

template <class... Ts> auto make_vec(Ts... xs) { return std::vector{xs...}; }

template <class... T> auto sum(T... xs) { return (xs + ...); }
template <class... T> bool all(T... bs) { return (... && bs); }
template <class... T> void print(const T&... xs) { ((std::cout << xs << ' '), ...); }

// Left vs right fold, made visible with subtraction.
template <class... T> constexpr int right_minus(T... xs) { return (xs - ...); }
template <class... T> constexpr int left_minus(T... xs) { return (... - xs); }

void print_rec() {}
template <class T, class... Rest> void print_rec(const T& first, const Rest&... rest) {
    std::cout << first << ' ';
    print_rec(rest...);
}

std::string sink(int&) { return "&"; }
std::string sink(int&&) { return "&&"; }
template <class... Args> std::string forward_to(Args&&... args) {
    return (sink(std::forward<Args>(args)) + ...);  // each argument keeps its value category
}

template <class... T> constexpr int right_init(T... xs) { return (xs - ... - 100); }  // x0 - (x1 - (... - 100))
template <class... T> constexpr int left_init(T... xs) { return (100 - ... - xs); }   // ((100 - x0) - x1) ...
template <class... T> constexpr bool any(T... bs) { return (bs || ...); }
template <class... T> void comma_fold(T... xs) { (xs, ...); }

constexpr int g(int x) { return x * 10; }
template <class... T> constexpr int g(T... xs) { return (xs + ... + 0); }
template <class... T> constexpr int count_args(T...) { return sizeof...(T); }
template <class... T> constexpr int each(T... args) { return count_args(g(args)...); }    // g on each arg
template <class... T> constexpr int whole(T... args) { return count_args(g(args...)); }   // one g on all

#ifdef __cpp_pack_indexing
template <class... Ts> auto first_and_last(Ts... args) {
    using First = Ts...[0];
    static_assert(std::is_same_v<First, std::remove_cvref_t<decltype(args...[0])>>);
    return std::pair{args...[0], args...[sizeof...(args) - 1]};
}
#endif
}  // namespace

// "Parameter packs" and "Expansion": sizeof... counts; xs... expands into a braced list.
TEST(VariadicTemplates, SizeofAndExpansion) {
    static_assert(count() == 0);  // "Empty packs ... legal"
    EXPECT_EQ(count(1, 'a', 2.0), 3u);
    auto v = make_vec(1, 2, 3);
    static_assert(std::is_same_v<decltype(v), std::vector<int>>);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
}

// "Fold expressions": sum, all, and right vs left association; empty && fold is true.
TEST(VariadicTemplates, FoldExpressions) {
    EXPECT_EQ(sum(1, 2, 3, 4), 10);
    EXPECT_TRUE(all(true, true));
    EXPECT_FALSE(all(true, false));
    EXPECT_TRUE(all());                          // unary && fold over an empty pack is true
    static_assert(right_minus(10, 3, 2) == 9);   // 10 - (3 - 2)
    static_assert(left_minus(10, 3, 2) == 5);    // (10 - 3) - 2
}

// "Fold expressions" and "Recursive expansion": both print each element followed by a space.
TEST(VariadicTemplates, CommaFoldAndRecursionPrintTheSame) {
    EXPECT_EQ(cppdocs::capture_stdout([] { print(1, "two", 3.5); }), "1 two 3.5 ");
    EXPECT_EQ(cppdocs::capture_stdout([] { print_rec(1, "two", 3.5); }), "1 two 3.5 ");
}

// "Pack indexing (C++26)": Ts...[0] / args...[i] pick an element directly.
TEST(VariadicTemplates, PackIndexing) {
    CPPDOCS_REQUIRE(__cpp_pack_indexing);
#ifdef __cpp_pack_indexing
    auto [first, last] = first_and_last(1, 2.5, 'z');
    EXPECT_EQ(first, 1);
    EXPECT_EQ(last, 'z');
#endif
}

// "Expansion": std::forward<Args>(args)... preserves each argument's value category.
TEST(VariadicTemplates, PerfectForwardingExpansion) {
    int x = 0;
    EXPECT_EQ(forward_to(x, 1, std::move(x)), "&&&&&");  // "&" + "&&" + "&&"
    EXPECT_EQ(forward_to(x), "&");
}

// "Fold expressions": the two init-value forms; empty || is false and empty , is void.
TEST(VariadicTemplates, InitFoldsAndEmptyPacks) {
    static_assert(right_init(1, 2) == 1 - (2 - 100));
    static_assert(left_init(1, 2) == (100 - 1) - 2);
    static_assert(right_init() == 100);  // binary folds handle empty packs via the init
    static_assert(!any());
    static_assert(std::is_void_v<decltype(comma_fold())>);
}

// "Gotchas": f(g(args)...) calls g per argument; f(g(args...)) passes the whole pack to one g.
TEST(VariadicTemplates, EllipsisPlacementDefinesThePattern) {
    static_assert(each(1, 2, 3) == 3);
    static_assert(whole(1, 2, 3) == 1);
}
