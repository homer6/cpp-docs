// Checks claims made by docs/examples/language/preprocessor/preprocessor.md.
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <version>

#include <gtest/gtest.h>

#include "support/capture.hpp"

#define SQUARE(x) ((x) * (x))
#define SQ_BAD(x) x*x
#define BUFFER_SIZE 1024

// "Gotchas": an unparenthesized macro gives SQ(1+2) == 5; the parenthesized one gives 9.
TEST(Preprocessor, FunctionLikeMacroParentheses) {
    EXPECT_EQ(SQ_BAD(1 + 2), 5);  // expands to 1+2*1+2
    EXPECT_EQ(SQUARE(1 + 2), 9);
    // "...and even then, double-evaluation bites": the argument is evaluated twice.
    int calls = 0;
    auto next = [&] { return ++calls; };
    (void)SQUARE(next());
    EXPECT_EQ(calls, 2);
}

// "Macros (#define)": #undef removes the definition.
TEST(Preprocessor, Undef) {
    EXPECT_EQ(BUFFER_SIZE, 1024);
#undef BUFFER_SIZE
#ifdef BUFFER_SIZE
    FAIL() << "BUFFER_SIZE still defined";
#endif
}

namespace {
const char* where() { return __func__; }  // __func__ is the enclosing function's name
}  // namespace

// "Useful predefined macros": __FILE__, __LINE__, __func__, __cplusplus.
TEST(Preprocessor, PredefinedMacros) {
    EXPECT_NE(std::string_view(__FILE__).find("preprocessor_test.cpp"), std::string_view::npos);
    const long line = __LINE__;
    EXPECT_EQ(__LINE__, line + 1);
    EXPECT_STREQ(where(), "where");
#if defined(_MSC_VER) && !defined(__clang__)
    // MSVC /std:c++latest (with /Zc:__cplusplus): "at least one higher than the highest supported
    // __cplusplus standard value" (Microsoft Learn, /Zc:__cplusplus), e.g. 202004L in VS 2022.
    EXPECT_GT(__cplusplus, 202002L);
#elif defined(CPPDOCS_CXX_STANDARD) && CPPDOCS_CXX_STANDARD < 26
    // Built as C++23 (GCC 13: no C++26 flag CMake knows); GCC 13 reports the
    // pre-publication 202100L there, so only "newer than C++20" holds.
    EXPECT_GT(__cplusplus, 202002L);
#else
    EXPECT_GT(__cplusplus, 202302L);  // built with -std=c++26: newer than C++23's value
#endif
}

// "Conditional compilation": __cpp_lib_format >= 201907L means <format> is usable.
TEST(Preprocessor, FeatureTestMacroGatesInclude) {
#if __cpp_lib_format >= 201907L
    SUCCEED();
#else
    GTEST_SKIP() << "needs __cpp_lib_format >= 201907L";
#endif
}

// "#include": the classic include guard skips the body the second time.
TEST(Preprocessor, IncludeGuard) {
    int bodies = 0;
#ifndef PREPROCESSOR_TEST_GUARD_H
#define PREPROCESSOR_TEST_GUARD_H
    ++bodies;
#endif
#ifndef PREPROCESSOR_TEST_GUARD_H  // the same guarded text pasted again
#define PREPROCESSOR_TEST_GUARD_H
    ++bodies;
#endif
    EXPECT_EQ(bodies, 1);
}

namespace preprocessor_test_scope {
#define PREPROCESSOR_TEST_NO_SCOPE 7  // "defined" inside a namespace...
}  // namespace preprocessor_test_scope

// "Macros": no scope — a macro defined inside a namespace is visible everywhere after it.
TEST(Preprocessor, MacrosIgnoreScope) {
    EXPECT_EQ(PREPROCESSOR_TEST_NO_SCOPE, 7);
#undef PREPROCESSOR_TEST_NO_SCOPE
}

// "Conditional compilation": exactly one platform branch (or none) is kept.
TEST(Preprocessor, PlatformBranches) {
    std::string_view path = "other";
#if defined(_WIN32)
    path = "windows";
#elif defined(__linux__)
    path = "linux";
#endif
#if defined(__linux__)
    EXPECT_EQ(path, "linux");
#elif defined(__APPLE__)
    EXPECT_EQ(path, "other");  // neither _WIN32 nor __linux__ on macOS
#elif defined(_WIN32)
    EXPECT_EQ(path, "windows");
#endif
#ifdef DEBUG
    GTEST_SKIP() << "DEBUG is defined by this build";
#endif
}

#define NDEBUG
#include <cassert>  // <cassert> is re-includable: assert now follows NDEBUG
namespace {
int assert_with_ndebug() {
    int evaluated = 0;
    assert(++evaluated == 100);  // expands to nothing: no evaluation, no abort
    return evaluated;
}
}  // namespace
#undef NDEBUG
#include <cassert>

// "Useful predefined macros": assert is compiled out when NDEBUG is defined.
TEST(Preprocessor, AssertAndNdebug) {
    EXPECT_EQ(assert_with_ndebug(), 0);
    int x = 0;
    EXPECT_DEATH(assert(x == 1), "");
}

#define CPPDOCS_LOG(fmt, ...) std::printf(fmt __VA_OPT__(,) __VA_ARGS__)

// "Variadic macros and __VA_OPT__ (C++20)": no dangling comma when the pack is empty.
TEST(Preprocessor, VaOptDropsComma) {
    auto out = cppdocs::capture_stdout([] {
        CPPDOCS_LOG("starting\n");
        CPPDOCS_LOG("%d items\n", 3);
    });
    EXPECT_EQ(out, "starting\n3 items\n");
#if defined(__cpp_va_opt) && !(defined(_MSC_VER) && !defined(__clang__))  // the page covers GCC and Clang
    ADD_FAILURE() << "the page says no toolchain defines __cpp_va_opt";
#endif
}

#define CPPDOCS_SAFE_PATH
// "Conditional compilation": #elifdef / #elifndef (C++23) and __has_include (C++17).
TEST(Preprocessor, ElifdefAndHasInclude) {
#ifdef CPPDOCS_FAST_PATH
    constexpr int path = 1;
#elifdef CPPDOCS_SAFE_PATH
    constexpr int path = 2;
#elifndef NDEBUG
    constexpr int path = 3;
#else
    constexpr int path = 4;
#endif
    EXPECT_EQ(path, 2);
#ifdef CPPDOCS_FAST_PATH
    constexpr int second = 1;
#elifndef CPPDOCS_NOT_DEFINED
    constexpr int second = 2;
#endif
    EXPECT_EQ(second, 2);
#if __has_include(<vector>) && !__has_include(<no_such_header_cppdocs.h>)
    constexpr bool has_include_works = true;
#else
    constexpr bool has_include_works = false;
#endif
    EXPECT_TRUE(has_include_works);
}
