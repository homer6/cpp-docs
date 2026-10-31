// Checks claims made by docs/examples/language/expressions/constant-expressions.md.
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

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
