// Checks claims made by docs/examples/numerics/complex.md.
#include <cmath>
#include <complex>
#include <numbers>
#include <type_traits>

#include <gtest/gtest.h>

using namespace std::complex_literals;

// "Examples": the values in the comments for a = 3 + 4i.
TEST(Complex, AccessorsAndFunctions) {
    std::complex<double> a{3.0, 4.0};
    std::complex<double> b = 1.0 + 2.0i;
    EXPECT_EQ(a.real(), 3.0);
    EXPECT_EQ(a.imag(), 4.0);
    EXPECT_EQ(b, (std::complex<double>{1.0, 2.0}));
    EXPECT_DOUBLE_EQ(std::abs(a), 5.0);
    EXPECT_DOUBLE_EQ(std::arg(a), std::atan2(4.0, 3.0));
    EXPECT_EQ(std::conj(a), (std::complex<double>{3.0, -4.0}));
    EXPECT_DOUBLE_EQ(std::norm(a), 25.0);  // magnitude squared, not magnitude
}

// "Examples": operators, exp, and polar (magnitude & angle).
TEST(Complex, OperatorsExpPolar) {
    std::complex<double> a{3.0, 4.0}, b{1.0, 2.0};
    EXPECT_EQ(a * b, (std::complex<double>{-5.0, 10.0}));
    auto e = std::exp(1.0i);  // cos 1 + i sin 1
    EXPECT_DOUBLE_EQ(e.real(), std::cos(1.0));
    EXPECT_DOUBLE_EQ(e.imag(), std::sin(1.0));
    auto p = std::polar(2.0, std::numbers::pi / 2);
    EXPECT_NEAR(p.real(), 0.0, 1e-15);
    EXPECT_DOUBLE_EQ(p.imag(), 2.0);
    EXPECT_NEAR(std::abs(std::sqrt(std::complex<double>{-4.0, 0.0}) - 2.0i), 0.0, 1e-15);
}

// "Gotchas": layout is compatible with an array of two T (real, then imag).
TEST(Complex, ArrayOfTwoLayout) {
    std::complex<double> z{1.5, -2.5};
    static_assert(sizeof z == 2 * sizeof(double));
    auto* parts = reinterpret_cast<double*>(&z);
    EXPECT_EQ(parts[0], 1.5);
    EXPECT_EQ(parts[1], -2.5);
}

// "Examples": the i / if / il literals give complex<double/float/long double>.
TEST(Complex, LiteralTypes) {
    static_assert(std::is_same_v<decltype(1.0i), std::complex<double>>);
    static_assert(std::is_same_v<decltype(1.0if), std::complex<float>>);
    static_assert(std::is_same_v<decltype(1.0il), std::complex<long double>>);
}
