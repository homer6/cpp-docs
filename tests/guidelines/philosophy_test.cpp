// Checks claims made by docs/guidelines/core-guidelines/examples/philosophy.md.
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

namespace {
class Month {
public:
    explicit Month(int m) : m_{m} {
        if (m < 1 || m > 12) throw std::invalid_argument("month out of range");
    }
    int value() const { return m_; }
private:
    int m_;
};

int destroyed = 0;
struct Tracked { ~Tracked() { ++destroyed; } };

bool handle(bool skip, bool fail) {
    auto t = std::make_unique<Tracked>();
    if (skip) return false;
    if (fail) throw std::runtime_error("failed");
    return true;
}

int fill(std::span<int> s) {
    if (s.size() > 9) { s[9] = 1; return 1; }
    return 0;
}
}  // namespace

// P.1: std::ranges::find says "search" and finds what the hand-written loop finds.
TEST(CoreGuidelinesP, P1_AlgorithmExpressesTheSearch) {
    const std::vector<int> v{4, 8, 15, 16};
    EXPECT_NE(std::ranges::find(v, 15), v.end());
    EXPECT_EQ(std::ranges::find(v, 15) - v.begin(), 2);
    EXPECT_EQ(std::ranges::find(v, 42), v.end());
}

// P.4: a std::variant knows which type it holds, so the wrong type can't be read by mistake.
TEST(CoreGuidelinesP, P4_VariantIsTypeSafe) {
    const std::variant<int, float> v = 1.5f;
    EXPECT_EQ(std::get_if<int>(&v), nullptr);
    ASSERT_NE(std::get_if<float>(&v), nullptr);
    EXPECT_FLOAT_EQ(*std::get_if<float>(&v), 1.5f);
}

// P.5: an assumption about types is checked by the compiler: long long has at least 64 bits.
TEST(CoreGuidelinesP, P5_CompileTimeChecks) {
    static_assert(std::numeric_limits<long long>::digits >= 63);
    static_assert(std::is_same_v<std::int64_t, std::int64_t>);
    SUCCEED();
}

// P.6: a span carries its size, so a callee can check before it writes.
TEST(CoreGuidelinesP, P6_SpanCarriesItsSize) {
    std::array<int, 10> big{};
    std::array<int, 3> small{};
    EXPECT_EQ(fill(big), 1);
    EXPECT_EQ(big[9], 1);
    EXPECT_EQ(fill(small), 0);
}

// P.7: a value checked once when it enters the program (here, in a constructor) is rejected at the source.
TEST(CoreGuidelinesP, P7_CheckAtTheBoundary) {
    EXPECT_THROW(Month{13}, std::invalid_argument);
    EXPECT_THROW(Month{0}, std::invalid_argument);
    EXPECT_EQ(Month{12}.value(), 12);
}

// P.8: an owning object releases its resource on early return and when an exception is thrown.
TEST(CoreGuidelinesP, P8_RaiiDoesNotLeak) {
    destroyed = 0;
    EXPECT_FALSE(handle(true, false));
    EXPECT_EQ(destroyed, 1);
    EXPECT_THROW(handle(false, true), std::runtime_error);
    EXPECT_EQ(destroyed, 2);
    EXPECT_TRUE(handle(false, false));
    EXPECT_EQ(destroyed, 3);
}

// P.10: a const object can't be assigned, so it can't change unexpectedly.
TEST(CoreGuidelinesP, P10_ImmutableData) {
    constexpr double tax_rate = 0.13;
    static_assert(!std::is_assignable_v<const double&, double>);
    EXPECT_DOUBLE_EQ(100 * (1 + tax_rate), 113.0);
}

// P.13: the library sort does the job of a hand-written one.
TEST(CoreGuidelinesP, P13_UseTheLibrary) {
    std::vector<int> v{5, 3, 9, 1};
    std::ranges::sort(v);
    EXPECT_EQ(v, (std::vector<int>{1, 3, 5, 9}));
}
