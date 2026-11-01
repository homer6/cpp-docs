// Checks claims made by docs/examples/language/expressions/constant-expressions.md.
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
constexpr int square(int x) { return x * x; }
consteval int force_ct(int x) { return x * x; }
constexpr int compute_constant() { return 42; }
constinit int counter = compute_constant();

constexpr bool ran_at_compile_time() { return std::is_constant_evaluated(); }

template <class T> auto stringify(const T& v) {
    if constexpr (std::is_arithmetic_v<T>) return std::to_string(v);
    else return std::string{v};
}

struct S {
    int v = 0;
    constexpr int get() { return v; }  // constexpr, but not a const member function
};
}  // namespace

// "constexpr — may run at compile time": the same function at compile time and at run time.
TEST(ConstantExpressions, ConstexprCompileOrRunTime) {
    constexpr int a = square(5);
    static_assert(a == 25);
    constexpr int N = square(8);
    int arr[N];
    static_assert(sizeof(arr) == 64 * sizeof(int));
    static_assert(square(4) == 16);
    volatile int input = 3;  // a value the compiler can't treat as constant
    int b = square(input);
    EXPECT_EQ(b, 9);
}

// "Gotchas": constexpr is a permission — with non-constant input the call runs at run time.
TEST(ConstantExpressions, ConstexprIsAPermission) {
    static_assert(ran_at_compile_time());
    volatile bool sink = ran_at_compile_time();  // evaluated at run time here
    EXPECT_FALSE(sink);
}

// "consteval": every call is evaluated at compile time.
TEST(ConstantExpressions, Consteval) {
    constexpr int ok = force_ct(5);
    static_assert(ok == 25);
    EXPECT_EQ(force_ct(6), 36);  // still a constant expression, folded at compile time
}

// "constinit": initialized at compile time, but still mutable at run time.
TEST(ConstantExpressions, ConstinitIsMutable) {
    EXPECT_EQ(counter, 42);
    ++counter;
    EXPECT_EQ(counter, 43);
    static_assert(!std::is_const_v<decltype(counter)>);
}

// "if constexpr": arithmetic types go through to_string, others construct a string.
TEST(ConstantExpressions, IfConstexprDiscards) {
    EXPECT_EQ(stringify(42), "42");
    EXPECT_EQ(stringify("hi"), "hi");  // std::to_string(const char*) would not compile
}

// "Gotchas": constexpr on a variable implies const; on a member function it doesn't.
TEST(ConstantExpressions, ConstexprVariableIsConstMemberIsNot) {
    constexpr int c = 1;
    static_assert(std::is_same_v<decltype(c), const int>);
    static_assert(std::is_same_v<decltype(&S::get), int (S::*)()>);
    static_assert(!std::is_invocable_v<decltype(&S::get), const S&>);
}

namespace cpp26_constexpr {
template <std::size_t N>
struct Slots {
    static_assert(N <= 4, std::string("Slots holds at most 4, got ") + char('0' + N));
};
constexpr std::string slots_message(std::size_t n) {
    return std::string("Slots holds at most 4, got ") + char('0' + n);
}
constexpr int through_void(int v) {
    void* erased = &v;
    return *static_cast<int*>(erased);
}
#if __cpp_constexpr_exceptions >= 202411L
constexpr int checked_div(int a, int b) {
    if (b == 0) throw std::invalid_argument("divide by zero");
    return a / b;
}
constexpr int div_or_zero(int a, int b) {
    try { return checked_div(a, b); }
    catch (const std::invalid_argument&) { return 0; }
}
#endif
}  // namespace cpp26_constexpr

// "static_assert with a computed message (C++26)": the message is a constexpr std::string.
TEST(ConstantExpressions, ComputedStaticAssertMessage) {
    CPPDOCS_REQUIRE_VERSION(__cpp_static_assert, 202306L);
    [[maybe_unused]] cpp26_constexpr::Slots<3> fine;
    static_assert(cpp26_constexpr::slots_message(7) == "Slots holds at most 4, got 7");
}

// "Casting from void* (C++26)": static_cast back to the real type in a constant expression.
TEST(ConstantExpressions, ConstexprCastFromVoidPointer) {
    CPPDOCS_REQUIRE_VERSION(__cpp_constexpr, 202306L);
    static_assert(cpp26_constexpr::through_void(7) == 7);
}

// "Exceptions during constant evaluation (C++26)": thrown and caught at compile time.
TEST(ConstantExpressions, ConstexprExceptions) {
    CPPDOCS_REQUIRE_VERSION(__cpp_constexpr_exceptions, 202411L);
#if __cpp_constexpr_exceptions >= 202411L
    static_assert(cpp26_constexpr::div_or_zero(10, 2) == 5);
    static_assert(cpp26_constexpr::div_or_zero(1, 0) == 0);
    volatile int zero = 0;
    EXPECT_THROW(cpp26_constexpr::checked_div(1, zero), std::invalid_argument);  // still throws at run time
#endif
}
