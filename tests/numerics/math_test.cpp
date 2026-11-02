// Checks claims made by docs/examples/numerics/math.md.
#include <cfloat>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "<cmath> — the workhorse functions": a value for each family.
TEST(Math, WorkhorseFunctions) {
    EXPECT_DOUBLE_EQ(std::cbrt(27.0), 3.0);
    EXPECT_EQ(std::pow(2.0, 10), 1024.0);
    EXPECT_EQ(std::log2(8.0), 3.0);
    EXPECT_EQ(std::floor(-2.5), -3.0);
    EXPECT_EQ(std::ceil(-2.5), -2.0);
    EXPECT_EQ(std::round(-2.5), -3.0);  // halves away from zero
    EXPECT_EQ(std::trunc(-2.5), -2.0);
    EXPECT_EQ(std::fmod(7.0, 4.0), 3.0);
    EXPECT_EQ(std::remainder(7.0, 4.0), -1.0);  // rounds the quotient to nearest
    EXPECT_EQ(std::fma(2.0, 3.0, 1.0), 7.0);
    EXPECT_EQ(std::lerp(10.0, 20.0, 0.25), 12.5);
    EXPECT_EQ(std::midpoint(1.0, 4.0), 2.5);
}

// "<cmath>": hypot avoids the overflow that sqrt(x*x + y*y) hits; midpoint
// avoids integer overflow.
TEST(Math, HypotAndMidpointAvoidOverflow) {
    double big = 1e200;
    EXPECT_TRUE(std::isinf(std::sqrt(big * big + big * big)));
    EXPECT_DOUBLE_EQ(std::hypot(big, big), big * std::numbers::sqrt2);
    int imax = std::numeric_limits<int>::max();
    EXPECT_EQ(std::midpoint(imax, imax - 2), imax - 1);
}

// "Classification": sqrt(-1.0) is NaN.
TEST(Math, Classification) {
    double x = std::sqrt(-1.0);
    EXPECT_TRUE(std::isnan(x));
    EXPECT_FALSE(std::isinf(x));
    EXPECT_FALSE(std::isfinite(x));
    EXPECT_FALSE(std::isnormal(x));
    EXPECT_TRUE(std::isnormal(1.0));
    EXPECT_FALSE(std::isnormal(DBL_MIN / 2));  // subnormal
    EXPECT_TRUE(std::signbit(-0.0));
}

// "Special functions (C++17)": libstdc++ has them all; libc++ (Clang 22) only hermite.
TEST(Math, SpecialFunctions) {
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 190000
    GTEST_SKIP() << "libc++ added std::hermite in LLVM 19; libc++ " << _LIBCPP_VERSION
                 << " has no special math functions (and no feature-test macro for hermite)";
#else
    EXPECT_EQ(std::hermite(2, 3.0), 34.0);  // H2(x) = 4x² − 2
#endif
#if defined(_LIBCPP_VERSION)
#ifdef __cpp_lib_math_special_functions
    FAIL() << "page says libc++ provides only std::hermite";
#endif
#else
    CPPDOCS_REQUIRE(__cpp_lib_math_special_functions);
#ifdef __cpp_lib_math_special_functions
    EXPECT_NEAR(std::riemann_zeta(2.0), std::numbers::pi * std::numbers::pi / 6, 1e-12);
    EXPECT_DOUBLE_EQ(std::legendre(2, 0.5), -0.125);
    EXPECT_DOUBLE_EQ(std::beta(2.0, 3.0), 1.0 / 12);
#endif
#endif
}

// "<numbers>": the constants and their precision variants.
TEST(Math, NumbersConstants) {
    namespace nb = std::numbers;
    static_assert(std::is_same_v<decltype(nb::pi), const double>);
    static_assert(std::is_same_v<decltype(nb::pi_v<float>), const float>);
    EXPECT_DOUBLE_EQ(nb::pi, std::acos(-1.0));
    EXPECT_DOUBLE_EQ(nb::e, std::exp(1.0));
    EXPECT_DOUBLE_EQ(nb::sqrt2, std::sqrt(2.0));
    EXPECT_DOUBLE_EQ(nb::ln2, std::log(2.0));
    EXPECT_DOUBLE_EQ(nb::phi, (1 + std::sqrt(5.0)) / 2);
    EXPECT_DOUBLE_EQ(nb::inv_pi, 1 / nb::pi);
}

// "Gotchas: std::abs": since C++17 <cmath> and <cstdlib> both declare every
// std::abs overload, so the integer call returns int and the float call double.
TEST(Math, StdAbsOverloads) {
    static_assert(std::is_same_v<decltype(std::abs(-3)), int>);
    static_assert(std::is_same_v<decltype(std::abs(-3L)), long>);
    static_assert(std::is_same_v<decltype(std::abs(-2.5)), double>);
    EXPECT_EQ(std::abs(-2.5), 2.5);
}
