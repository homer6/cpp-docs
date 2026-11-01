// Checks claims made by docs/examples/metaprogramming/constant-evaluation.md.
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
constexpr int where() {
    if (std::is_constant_evaluated()) return 1;
    else return 2;
}

int global_where = where();  // static storage: tried as constant initialization

consteval int sq(int x) { return x * x; }

constexpr int sq_or(int x) {
    if consteval { return sq(x); }
    else { return x * x; }
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconstant-evaluated"
#endif
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-compare"
#endif
constexpr int always() {
    if constexpr (std::is_constant_evaluated()) return 1;
    else return 2;
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#ifdef __cpp_lib_is_within_lifetime
union OptBool {
    char none = 2;
    bool b;
    constexpr OptBool() = default;
    constexpr OptBool(bool v) : b(v) {}
    constexpr bool has_value() const {
        if consteval { return std::is_within_lifetime(&b); }
        else { return none != 2; }
    }
};
#endif
}  // namespace

// "std::is_constant_evaluated (C++20)": 1 at compile time, 2 at run time, 1 for static storage.
TEST(ConstantEvaluation, IsConstantEvaluated) {
    constexpr int c = where();
    volatile int sink = 0;
    int r = where() + sink;
    static int s = where();
    EXPECT_EQ(c, 1);
    EXPECT_EQ(r, 2);
    EXPECT_EQ(s, 1);
    EXPECT_EQ(global_where, 1);
}

// "if consteval (C++23)": the compile-time branch may call a consteval function.
TEST(ConstantEvaluation, IfConsteval) {
    static_assert(sq_or(4) == 16);
    volatile int n = 5;
    EXPECT_EQ(sq_or(n), 25);
}

// "Gotchas": if constexpr (std::is_constant_evaluated()) is always true.
TEST(ConstantEvaluation, IfConstexprIsAlwaysTrue) {
    volatile int sink = 0;
    EXPECT_EQ(always() + sink, 1);
}

// "std::is_within_lifetime (C++26)": tells which union member is active (Clang 22 only).
TEST(ConstantEvaluation, IsWithinLifetime) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_is_within_lifetime, 202306L);
#ifdef __cpp_lib_is_within_lifetime
    static_assert(!OptBool{}.has_value());
    static_assert(OptBool{true}.has_value());
    static_assert(OptBool{false}.has_value());
    EXPECT_FALSE(OptBool{}.has_value());
    EXPECT_TRUE(OptBool{false}.has_value());
#endif
}
