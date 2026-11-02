// Checks claims made by docs/examples/numerics/complex.md.
#include <cmath>
#include <complex>
#include <numbers>
#include <sstream>
#include <version>
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

// "Mental model": arg(3 + 4i) ≈ 0.927.
TEST(Complex, ArgValue) {
    EXPECT_NEAR(std::arg(std::complex<double>{3.0, 4.0}), 0.927, 0.0005);
}

// "Declaration": real(x)/imag(x) set a part; streams as (re,im).
TEST(Complex, SettersAndStreaming) {
    std::complex<double> z;
    z.real(1.5);
    z.imag(-2.0);
    std::ostringstream os;
    os << z;
    EXPECT_EQ(os.str(), "(1.5,-2)");
    std::istringstream in{"(3,4)"};
    in >> z;
    EXPECT_EQ(z, (std::complex<double>{3.0, 4.0}));
}

// "Compile-time arithmetic (C++20)": (1 + 2i)² = -3 + 4i in a constant expression;
// __cpp_lib_constexpr_complex is 201711L on GCC 15 and Clang 22.
TEST(Complex, ConstexprArithmetic) {
    constexpr std::complex<double> z{1.0, 2.0};
    const auto rt = z * z;  // the same product at run time
    EXPECT_EQ(rt, (std::complex<double>{-3.0, 4.0}));
#if defined(__clang__) && defined(__GLIBCXX__)
    // libstdc++ stores complex<double> as a GNU `__complex__ double` and multiplies with
    // `_M_value *= __t`, which Clang (18 through 23 so far) won't constant-evaluate
    // ("subexpression not valid in a constant expression").
    GTEST_SKIP() << "Clang " << __clang_major__ << " with libstdc++ " << _GLIBCXX_RELEASE
                 << " can't constant-evaluate libstdc++'s __complex__ arithmetic "
                    "(llvm/llvm-project#55370); run-time result checked";
#else
    constexpr auto z2 = z * z;
    static_assert(z2.real() == -3.0 && z2.imag() == 4.0);
#if defined(_LIBCPP_VERSION) || (defined(__GNUC__) && __GNUC__ == 15)
    EXPECT_EQ(__cpp_lib_constexpr_complex, 201711L);
#endif
#endif
}

// "Square root of a negative number": real overload gives NaN, complex gives i.
TEST(Complex, SqrtOfNegative) {
    EXPECT_TRUE(std::isnan(std::sqrt(-1.0)));
    auto r = std::sqrt(std::complex<double>{-1.0});  // "≈": libc++ gives (6.1e-17, 1)
    EXPECT_NEAR(r.real(), 0.0, 1e-15);
    EXPECT_DOUBLE_EQ(r.imag(), 1.0);
}

// "Gotchas": int * complex<double> doesn't compile; 2.0 * z does.
template <class A, class B>
concept Multipliable = requires(A a, B b) { a * b; };
TEST(Complex, NoMixedIntArithmetic) {
    static_assert(!Multipliable<int, std::complex<double>>);
    static_assert(Multipliable<double, std::complex<double>>);
}
