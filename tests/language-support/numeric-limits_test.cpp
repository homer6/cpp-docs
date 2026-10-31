// Checks claims made by docs/examples/language-support/numeric-limits.md.
#include <cfloat>
#include <climits>
#include <cmath>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

using std::numeric_limits;

// "Examples": the values in the comments.
TEST(NumericLimits, Tour) {
    static_assert(numeric_limits<int>::max() == INT_MAX);
    static_assert(numeric_limits<int>::min() == INT_MIN);
    static_assert(numeric_limits<unsigned>::max() == ~0u);  // all bits set
    static_assert(numeric_limits<double>::lowest() == -numeric_limits<double>::max());
    static_assert(numeric_limits<double>::min() == DBL_MIN);
    static_assert(numeric_limits<double>::min() > 0);  // smallest POSITIVE normal
    static_assert(numeric_limits<double>::epsilon() == DBL_EPSILON);
    static_assert(numeric_limits<int>::digits == 31);  // 32-bit int
    static_assert(numeric_limits<int>::is_signed && !numeric_limits<unsigned>::is_signed);
    static_assert(numeric_limits<int>::is_integer && !numeric_limits<double>::is_integer);
    EXPECT_TRUE(std::isinf(numeric_limits<double>::infinity()));
    EXPECT_TRUE(std::isnan(numeric_limits<double>::quiet_NaN()));
}

// "Gotchas": epsilon is the gap at 1.0.
TEST(NumericLimits, EpsilonIsGapAtOne) {
    constexpr double eps = numeric_limits<double>::epsilon();
    EXPECT_EQ(std::nextafter(1.0, 2.0) - 1.0, eps);
    EXPECT_GT(std::nextafter(1e6, 2e6) - 1e6, eps);  // bigger gap at bigger magnitudes
}

// "Common uses": a running max starts at lowest(); nearly_equal scales epsilon.
TEST(NumericLimits, CommonUses) {
    double best = numeric_limits<double>::lowest();
    for (double v : {-1e300, -5.0}) best = v > best ? v : best;
    EXPECT_EQ(best, -5.0);
    auto nearly_equal = [](double a, double b, double scale) {
        return std::abs(a - b) <= numeric_limits<double>::epsilon() * scale;
    };
    EXPECT_TRUE(nearly_equal(0.1 + 0.2, 0.3, 1.0));
    EXPECT_FALSE(0.1 + 0.2 == 0.3);
}

// "Gotchas": all constexpr — usable as an array bound.
TEST(NumericLimits, Constexpr) {
    char buf[numeric_limits<unsigned char>::max() + 1];
    EXPECT_EQ(sizeof buf, 256u);
}

// "Declaration": lowest() equals min() for integers; digits10/max_digits10 values.
TEST(NumericLimits, DeclarationTable) {
    static_assert(numeric_limits<int>::lowest() == numeric_limits<int>::min());
    static_assert(numeric_limits<int>::digits10 == 9);
    static_assert(numeric_limits<double>::digits10 == 15);
    static_assert(numeric_limits<double>::max_digits10 == 17);
    static_assert(numeric_limits<int>::max_digits10 == 0);
    static_assert(numeric_limits<int>::epsilon() == 0);
    static_assert(numeric_limits<double>::digits == 53);  // mantissa bits
    static_assert(numeric_limits<const int>::is_specialized);
    static_assert(numeric_limits<double>::has_infinity && numeric_limits<double>::has_quiet_NaN);
}

// "Check before you overflow": checked_add from the page.
constexpr std::optional<int> checked_add(int a, int b) {
    if (b > 0 && a > numeric_limits<int>::max() - b) return std::nullopt;
    if (b < 0 && a < numeric_limits<int>::min() - b) return std::nullopt;
    return a + b;
}
TEST(NumericLimits, CheckedAdd) {
    static_assert(!checked_add(numeric_limits<int>::max(), 1));
    static_assert(!checked_add(numeric_limits<int>::min(), -1));
    static_assert(*checked_add(40, 2) == 42);
}

// "Gotchas": an unspecialized type compiles silently with is_specialized false.
TEST(NumericLimits, UnspecializedType) {
    struct Mine { int v = 0; };
    static_assert(!numeric_limits<Mine>::is_specialized);
    static_assert(numeric_limits<Mine>::max().v == 0);  // T()
}

// "Gotchas": char-sized results print as characters; unary + promotes.
TEST(NumericLimits, CharSizedPrintsAsChar) {
    std::ostringstream a, b;
    a << numeric_limits<unsigned char>::max();
    b << +numeric_limits<unsigned char>::max();
    EXPECT_EQ(a.str(), std::string(1, static_cast<char>(255)));
    EXPECT_EQ(b.str(), "255");
}
