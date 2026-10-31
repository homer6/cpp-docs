// Checks claims made by docs/examples/language/expressions/operators.md.
#include <compare>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

#include "support/capture.hpp"

// The expressions the page warns about (unparenthesized mixes, i = i++ + 1) are
// what these tests check, so the compilers' warnings for them are silenced here.
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wshift-op-parentheses"
#pragma clang diagnostic ignored "-Wparentheses"
#pragma clang diagnostic ignored "-Wunsequenced"
#else
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wsequence-point"  // GCC also flags C++17-defined forms
#endif

namespace {
struct Vec2 {
    double x, y;
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    bool operator==(const Vec2&) const = default;
    auto operator<=>(const Vec2&) const = default;
};
Vec2 operator+(Vec2 a, const Vec2& b) { return a += b; }

std::string order;
int f() { order += 'f'; return 1; }
int g() { order += 'g'; return 2; }
}  // namespace

// "Precedence": * before +, << below +, comparisons above bitwise &, assignment right-to-left.
TEST(Operators, Precedence) {
    static_assert(1 + 2 * 3 == 7);
    static_assert((1 << 1 + 1) == 4);  // + binds tighter than <<
    int x = 6, mask = 4;
    EXPECT_EQ(x & mask == 0, 0);  // "Gotchas": parses as x & (mask == 0)
    EXPECT_FALSE((x & mask) == 0);
    int a, b;
    a = b = 3;
    EXPECT_EQ(a, 3);
    static_assert(!true ? false : true);  // unary ! before ?:
}

// "Overloading operators": += returns *this, + is built from +=, defaulted == and <=>.
TEST(Operators, OverloadingVec2) {
    Vec2 a{1, 2}, b{3, 4};
    Vec2& r = (a += b);
    EXPECT_EQ(&r, &a);
    EXPECT_EQ(a, (Vec2{4, 6}));
    EXPECT_EQ((Vec2{1, 1} + Vec2{2, 3}), (Vec2{3, 4}));
    EXPECT_TRUE((Vec2{1, 2} < Vec2{1, 3}));
    EXPECT_TRUE((Vec2{2, 0} >= Vec2{1, 9}));
    static_assert(std::is_same_v<decltype(Vec2{} <=> Vec2{}), std::partial_ordering>);
}

// "Evaluation order": C++17 rules for <<, a[i] = i++, and i = i++ + 1.
TEST(Operators, EvaluationOrderCpp17) {
    order.clear();
    auto out = cppdocs::capture_stdout([] { std::cout << f() << g(); });
    EXPECT_EQ(out, "12");
    EXPECT_EQ(order, "fg");  // left-to-right for << chains

    int a[3]{};
    int i = 1;
    a[i] = i++;  // RHS (i++) is sequenced before the assignment's left operand
    EXPECT_EQ(i, 2);
    EXPECT_EQ(a[2], 1);

    int j = 5;
    j = j++ + 1;  // defined since C++17: the assignment happens after j++
    EXPECT_EQ(j, 6);
}

// "Evaluation order": &&, || and ?: order left-to-right and short-circuit.
TEST(Operators, ShortCircuit) {
    order.clear();
    bool r1 = (f() == 0) && (g() == 2);
    EXPECT_FALSE(r1);
    EXPECT_EQ(order, "f");
    order.clear();
    bool r2 = (f() == 1) || (g() == 2);
    EXPECT_TRUE(r2);
    EXPECT_EQ(order, "f");
    order.clear();
    int r3 = f() == 1 ? 10 : g();
    EXPECT_EQ(r3, 10);
    EXPECT_EQ(order, "f");
    order.clear();
    (void)(f(), g());
    EXPECT_EQ(order, "fg");
}
