// Checks claims made by docs/examples/language/expressions/constant-evaluation-rules.md.
#include <algorithm>
#include <array>
#include <bitset>
#include <charconv>
#include <cmath>
#include <memory>
#include <new>
#include <numeric>
#include <optional>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
struct Shape {
    virtual constexpr ~Shape() = default;
    virtual constexpr int area() const = 0;
};
struct Square : Shape {
    int side;
    constexpr explicit Square(int s) : side(s) {}
    constexpr int area() const override { return side * side; }
};
struct Circle : Shape {
    constexpr int area() const override { return 3; }
};

constexpr bool check_polymorphism() {
    Square sq{3};
    const Shape& s = sq;
    return s.area() == 9 && dynamic_cast<const Square*>(&s) != nullptr &&
           dynamic_cast<const Circle*>(&s) == nullptr && typeid(s) == typeid(Square);
}

constexpr int guarded(int x) {
    try {
        return x + 1;
    } catch (...) {
        return -1;
    }
}

union IntOrFloat {
    int i;
    float f;
};
constexpr float switch_member() {
    IntOrFloat u{.i = 1};
    u.f = 2.5f;
    return u.f;
}

constexpr int sum_to(int n) {
    std::vector<int> v(n);
    std::iota(v.begin(), v.end(), 1);
    return std::accumulate(v.begin(), v.end(), 0);
}

constexpr char hex_digit(int n) {
    static constexpr char digits[] = "0123456789abcdef";
    return digits[n];
}

constexpr int parse_digit(char c) {
    if (c < '0' || c > '9') goto fail;
    return c - '0';
fail:
    static int failures = 0;
    ++failures;
    return -1;
}

consteval int checked(int x) { return x; }
template <class T> constexpr int forward_checked(T x) { return checked(x); }

struct Point {
    int x, y;
};
constexpr int build() {
    std::allocator<Point> alloc;
    Point* p = alloc.allocate(2);
    std::construct_at(p, 1, 2);
    ::new (static_cast<void*>(p + 1)) Point{3, 4};
    int r = p[0].x + p[1].y;
    std::destroy(p, p + 2);
    alloc.deallocate(p, 2);
    return r;
}

struct Rec {
    int key;
    char tag;
};
constexpr bool keeps_order() {
    std::array<Rec, 4> r{{{2, 'a'}, {1, 'b'}, {2, 'c'}, {1, 'd'}}};
    std::stable_sort(r.begin(), r.end(), [](Rec x, Rec y) { return x.key < y.key; });
    return r[0].tag == 'b' && r[1].tag == 'd' && r[2].tag == 'a' && r[3].tag == 'c';
}

constexpr long spin(long n) {
    long s = 0;
    for (long i = 0; i < n; ++i) s += i;
    return s;
}
}  // namespace

// "virtual, dynamic_cast and typeid (C++20)": polymorphism works at compile time.
TEST(ConstantEvaluationRules, PolymorphismAtCompileTime) {
    static_assert(check_polymorphism());
    EXPECT_TRUE(check_polymorphism());
}

// "try/catch and union members (C++20)": a try block that doesn't throw; switching members.
TEST(ConstantEvaluationRules, TryCatchAndUnionMember) {
    static_assert(guarded(1) == 2);
    static_assert(switch_member() == 2.5f);
    EXPECT_EQ(guarded(41), 42);
}

// "Transient allocation (C++20)": a vector used and freed within one evaluation.
TEST(ConstantEvaluationRules, TransientAllocation) {
    static_assert(sum_to(100) == 5050);
    EXPECT_EQ(sum_to(4), 10);  // and at run time
}

// "static constexpr locals, non-literal variables, goto (C++23)": they compile; goto runs only
// at run time.
TEST(ConstantEvaluationRules, Cxx23Relaxations) {
    CPPDOCS_REQUIRE_VERSION(__cpp_constexpr, 202211L);
    static_assert(hex_digit(11) == 'b');
    static_assert(parse_digit('7') == 7);
    EXPECT_EQ(parse_digit('x'), -1);  // the goto path, at run time
}

// "consteval propagation (C++23)": a template calling a consteval function becomes immediate.
TEST(ConstantEvaluationRules, ConstevalPropagation) {
    CPPDOCS_REQUIRE_VERSION(__cpp_consteval, 202211L);
    static_assert(forward_checked(5) == 5);
    EXPECT_EQ(forward_checked(6), 6);  // constant argument: fine
}

// "Placement new (C++26) and std::construct_at (C++20)".
TEST(ConstantEvaluationRules, PlacementNew) {
    CPPDOCS_REQUIRE_VERSION(__cpp_constexpr, 202406L);
    static_assert(build() == 5);
    EXPECT_EQ(build(), 5);
}

// "Evaluation limits": two million iterations exceed both defaults at compile time, but run
// fine at run time.
TEST(ConstantEvaluationRules, LimitsApplyOnlyAtCompileTime) {
    volatile long n = 2'000'000;
    EXPECT_EQ(spin(n), 1'999'999'000'000L);
}

// "The constexpr standard library": stable_sort (C++26), containers and utilities.
TEST(ConstantEvaluationRules, ConstexprLibrary) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_constexpr_algorithms, 202306L);
#if __cpp_lib_constexpr_algorithms >= 202306L
    static_assert(keeps_order());
#endif
    static_assert(__cpp_lib_constexpr_vector >= 201907L);
    static_assert(__cpp_lib_constexpr_string >= 201907L);
    static_assert(__cpp_lib_constexpr_memory >= 202202L);  // unique_ptr (P2273)
    static_assert([] {
        std::optional<std::string> o = "abc";
        std::variant<int, std::string> v = std::string("de");
        auto p = std::make_unique<int>(41);
        ++*p;
        std::bitset<8> b;
        b.set(1).set(3);
        char buf[8]{};
        auto r = std::to_chars(buf, buf + 8, 42);
        int parsed = 0;
        std::from_chars(buf, r.ptr, parsed);
        return o->size() == 3 && std::get<1>(v).size() == 2 && *p == 42 && b.count() == 2 &&
               parsed == 42;
    }());
}

// "<cmath>": no toolchain defines the macro yet; GCC folds its builtins, libc++ doesn't.
TEST(ConstantEvaluationRules, ConstexprCmath) {
#if defined(__cpp_lib_constexpr_cmath) || (defined(__GNUC__) && !defined(__clang__))
    static_assert(std::fabs(-2.5) == 2.5);
    static_assert(std::sqrt(4.0) == 2.0);
    SUCCEED();
#else
    static_assert(!std::isnan(1.0) && std::signbit(-1.0));  // these do work
    GTEST_SKIP() << "std::fabs isn't constexpr in this library (no __cpp_lib_constexpr_cmath)";
#endif
}
