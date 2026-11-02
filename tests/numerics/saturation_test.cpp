// Checks claims made by docs/examples/numerics/saturation.md.
#include <climits>
#include <cstdint>
#include <numeric>
#include <version>

// <stdckdint.h> (C23, C++26 by P3370R1): GCC, Clang and libc++/libstdc++ ship it; MSVC/UCRT
// doesn't yet. GCC's own header up to 14.2 is the C one, whose ckd_add casts to _Bool and
// doesn't compile in C++; libstdc++'s C++ header (GCC 15, also in GCC 14.4) defines
// _GLIBCXX_STDCKDINT_H, and GCC 16 adds __cpp_lib_stdckdint_h. Clang's own header works in C++.
#if __has_include(<stdckdint.h>)
#include <stdckdint.h>
#if !defined(__GNUC__) || defined(__clang__) || defined(_GLIBCXX_STDCKDINT_H) || \
    defined(__cpp_lib_stdckdint_h)
#define CPPDOCS_HAS_STDCKDINT 1
#else
#define CPPDOCS_STDCKDINT_C_ONLY 1
#endif
#endif

#include "support/feature.hpp"

#include <gtest/gtest.h>

namespace {
#if __cpp_lib_saturation_arithmetic >= 202603L
template <class A, class B>
concept SaturatingAddable = requires(A a, B b) { std::saturating_add(a, b); };
#endif
}  // namespace

// "Mental model": ordinary uint8_t arithmetic stored back wraps 300 to 44.
TEST(Saturation, PlainArithmeticWraps) {
    std::uint8_t a = 200, b = 100;
    std::uint8_t sum = static_cast<std::uint8_t>(a + b);
    EXPECT_EQ(sum, 44);
}

// "Clamp instead of wrap": the P4052 names (macro 202603L) clamp to the type's range.
TEST(Saturation, SaturatingNamesClamp) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_saturation_arithmetic, 202603L);
#if __cpp_lib_saturation_arithmetic >= 202603L
    EXPECT_EQ(std::saturating_add<std::uint8_t>(200, 100), 255);
    EXPECT_EQ(std::saturating_sub<std::uint8_t>(10, 20), 0);
    EXPECT_EQ(std::saturating_sub(3u, 5u), 0u);
    EXPECT_EQ(std::saturating_mul<std::int8_t>(-100, 2), -128);
    EXPECT_EQ(std::saturating_div(INT_MIN, -1), INT_MAX);
    EXPECT_EQ(std::saturating_cast<std::uint8_t>(300), 255);
    EXPECT_EQ(std::saturating_cast<std::int8_t>(-1000), -128);
    static_assert(std::saturating_add(INT_MAX, 1) == INT_MAX);  // constexpr
    static_assert(noexcept(std::saturating_add(1, 2)));
#endif
}

// "Gotchas" ("Integers only, same type only"): mixed types and bool don't compile.
TEST(Saturation, IntegersOnlySameType) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_saturation_arithmetic, 202603L);
#if __cpp_lib_saturation_arithmetic >= 202603L
    static_assert(SaturatingAddable<int, int>);
    static_assert(!SaturatingAddable<int, unsigned>);
    static_assert(!SaturatingAddable<bool, bool>);
    static_assert(!SaturatingAddable<double, double>);
#endif
}

// "The pre-rename names": GCC 15 and Clang 22 ship add_sat / saturate_cast at
// 202311L; the values are the same.
TEST(Saturation, PreRenameNames) {
    CPPDOCS_REQUIRE(__cpp_lib_saturation_arithmetic);
#if __cpp_lib_saturation_arithmetic >= 202603L
    GTEST_SKIP() << "this toolchain has the P4052 names (202603L), not add_sat";
#elif defined(__cpp_lib_saturation_arithmetic)
    EXPECT_EQ(__cpp_lib_saturation_arithmetic, 202311L);
    EXPECT_EQ(std::add_sat<std::uint8_t>(200, 100), 255);
    EXPECT_EQ(std::sub_sat<std::uint8_t>(10, 20), 0);
    EXPECT_EQ(std::mul_sat<std::int8_t>(-100, 2), -128);
    EXPECT_EQ(std::div_sat(INT_MIN, -1), INT_MAX);
    EXPECT_EQ(std::saturate_cast<std::uint8_t>(300), 255);
#endif
}

// "The pre-rename names": GCC 16 has the new names, GCC 15 and Clang 22 the old.
TEST(Saturation, ToolchainSupport) {
#if defined(__GLIBCXX__) && !defined(__clang__) && __GNUC__ >= 16
    EXPECT_GE(__cpp_lib_saturation_arithmetic, 202603L);
#elif defined(__GLIBCXX__) && !defined(__clang__) && __GNUC__ == 15
    EXPECT_EQ(__cpp_lib_saturation_arithmetic, 202311L);
#elif defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 220000 && _LIBCPP_VERSION < 230000
    EXPECT_EQ(__cpp_lib_saturation_arithmetic, 202311L);
#else
    GTEST_SKIP() << "toolchain not covered by the page's support note";
#endif
}

// "Brighten" snippet: switching on the macro works on every toolchain.
TEST(Saturation, PortableSwitch) {
    CPPDOCS_REQUIRE(__cpp_lib_saturation_arithmetic);
#if __cpp_lib_saturation_arithmetic >= 202603L
    EXPECT_EQ(std::saturating_add<std::uint8_t>(250, 10), 255);
#elif defined(__cpp_lib_saturation_arithmetic)
    EXPECT_EQ(std::add_sat<std::uint8_t>(250, 10), 255);
#endif
}

// "Gotchas" ("Not a checked operation"): ckd_add reports overflow by returning true.
TEST(Saturation, CkdAddReportsOverflow) {
#ifdef CPPDOCS_HAS_STDCKDINT
    int r = 0;
    EXPECT_TRUE(ckd_add(&r, INT_MAX, 1));
    EXPECT_FALSE(ckd_add(&r, 2, 3));
    EXPECT_EQ(r, 5);
#elif defined(CPPDOCS_STDCKDINT_C_ONLY)
    GTEST_SKIP() << "<stdckdint.h> here is GCC's C header (ckd_add uses _Bool), not usable from "
                    "C++; libstdc++'s C++26 <stdckdint.h> (P3370R1) arrived in GCC 15";
#else
    GTEST_SKIP() << "<stdckdint.h> not provided by this toolchain (MSVC/UCRT lacks it)";
#endif
}
