// Checks claims made by docs/examples/feature-test-macros/README.md.
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Two families": language macros come from the compiler, no header needed.
TEST(FeatureTestMacros, LanguageMacrosAreDates) {
#if !defined(__cpp_concepts) || !defined(__cpp_if_constexpr) || !defined(__cpp_consteval)
    FAIL() << "C++26 mode should define __cpp_concepts, __cpp_if_constexpr, __cpp_consteval";
#else
    // "defined to a date value (YYYYMM)L": six digits, a plausible year and month.
    for (long v : {long{__cpp_concepts}, long{__cpp_if_constexpr}, long{__cpp_consteval}}) {
        EXPECT_GE(v, 199711L);
        EXPECT_LE(v, 209912L);
        EXPECT_GE(v % 100, 1);
        EXPECT_LE(v % 100, 12);
    }
#endif
}

// "Two families": a language macro is defined only while its feature is on —
// GCC defines __cpp_modules only with -fmodules (not passed here); Clang 22
// defines it as 1.
TEST(FeatureTestMacros, ModulesMacroDependsOnToolchain) {
#if defined(__clang__)
#ifdef __cpp_modules
    EXPECT_EQ(__cpp_modules, 1);
#else
    FAIL() << "Clang 22 defines __cpp_modules in C++26 mode";
#endif
#elif defined(__GNUC__)
#ifdef __cpp_modules
    FAIL() << "GCC should define __cpp_modules only with -fmodules";
#endif
#else
    GTEST_SKIP() << "expectation recorded only for GCC and Clang";
#endif
}

// "Mental model": the std::format gate from the page takes the format branch.
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
#include <format>
#endif
TEST(FeatureTestMacros, FormatGate) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_format, 201907L);
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
    EXPECT_EQ(std::format("{}-{}", 1, 2), "1-2");
#endif
}

// "Examples": the __cpp_lib_expected-gated branch picks std::expected when available.
#if __cpp_lib_expected >= 202202L
#include <expected>
#endif
TEST(FeatureTestMacros, GateOnLibraryMacroValue) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_expected, 202202L);
#if __cpp_lib_expected >= 202202L
    std::expected<int, int> r = std::unexpected(3);
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), 3);
#endif
}

// "Examples": the __cpp_lib_ranges-gated views::filter keeps the evens.
#if __cpp_lib_ranges >= 201911L
#include <ranges>
#include <vector>
#endif
TEST(FeatureTestMacros, RangesBranch) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_ranges, 201911L);
#if __cpp_lib_ranges >= 201911L
    std::vector<int> data{1, 2, 3, 4};
    auto v = data | std::views::filter([](int x) { return x % 2 == 0; });
    EXPECT_EQ(std::ranges::to<std::vector>(v), (std::vector<int>{2, 4}));
#endif
}

// "Gotchas": __cplusplus says which standard was requested (C++26 mode is newer than C++23).
TEST(FeatureTestMacros, CplusplusReflectsRequestedStandard) {
#if defined(_MSC_VER) && !defined(__clang__) && __cplusplus <= 202302L
    GTEST_SKIP() << "MSVC has no C++26 mode; /std:c++latest sets __cplusplus to " << __cplusplus;
#else
    EXPECT_GT(__cplusplus, 202302L);
#endif
}
