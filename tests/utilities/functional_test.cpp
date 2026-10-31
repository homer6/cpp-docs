// Checks claims made by docs/examples/utilities/functional.md.
#include <functional>
#include <memory>
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "std::function": reassign to std::multiplies, 6 * 7 == 42; calling an empty one throws bad_function_call.
TEST(Functional, StdFunction) {
    std::function<int(int, int)> op = [](int a, int b) { return a + b; };
    EXPECT_EQ(op(6, 7), 13);
    op = std::multiplies<int>{};
    EXPECT_EQ(op(6, 7), 42);
    EXPECT_TRUE(static_cast<bool>(op));

    std::function<int(int, int)> empty;
    EXPECT_FALSE(static_cast<bool>(empty));
    EXPECT_THROW(empty(1, 2), std::bad_function_call);
}

// "std::move_only_function (C++23)": holds a move-only callable and is itself move-only.
TEST(Functional, MoveOnlyFunction) {
    CPPDOCS_REQUIRE(__cpp_lib_move_only_function);
#ifdef __cpp_lib_move_only_function
    std::move_only_function<int()> task = [p = std::make_unique<int>(5)] { return *p; };
    EXPECT_EQ(task(), 5);
    static_assert(!std::is_copy_constructible_v<std::move_only_function<void()>>);
    // A noexcept-qualified signature is accepted.
    std::move_only_function<int() const noexcept> c = []() noexcept { return 1; };
    EXPECT_EQ(c(), 1);
#endif
}

// "std::invoke": calls a member function, reads a data member, calls a lambda.
TEST(Functional, Invoke) {
    struct S {
        int f(int x) const { return x + data; }
        int data;
    };
    S s{1};
    auto some_lambda = [](int a, int b) { return a * b; };
    EXPECT_EQ(std::invoke(&S::f, s, 10), 11);
    EXPECT_EQ(std::invoke(&S::data, s), 1);
    EXPECT_EQ(std::invoke(some_lambda, 3, 4), 12);
}

// "std::invoke": std::invoke_r<R> (C++23) converts the result to R.
TEST(Functional, InvokeR) {
    CPPDOCS_REQUIRE(__cpp_lib_invoke_r);
#ifdef __cpp_lib_invoke_r
    auto half = [](int x) { return x / 2.0; };
    static_assert(std::is_same_v<decltype(std::invoke_r<int>(half, 5)), int>);
    EXPECT_EQ(std::invoke_r<int>(half, 5), 2);
#endif
}

// "ref / cref": reference_wrapper reaches the callable as a reference; bind copies unless wrapped.
TEST(Functional, RefSurvivesCopy) {
    int counter = 0;
    auto inc = [](int& c) { ++c; };
    std::invoke(inc, std::ref(counter));
    EXPECT_EQ(counter, 1);

    auto copied = std::bind(inc, counter);  // bind stores a copy of counter
    copied();
    EXPECT_EQ(counter, 1);
    auto wrapped = std::bind(inc, std::ref(counter));
    wrapped();
    EXPECT_EQ(counter, 2);
}

// "Operator functors": plus<>{}(2, 3) == 5; transparent functors deduce operand types; greater<int> descends.
TEST(Functional, OperatorFunctors) {
    EXPECT_EQ(std::plus<>{}(2, 3), 5);
    EXPECT_EQ(std::plus<>{}(1, 0.5), 1.5);  // deduces: int + double
    static_assert(requires { typename std::less<>::is_transparent; });
    EXPECT_TRUE(std::less<>{}(1, 2));
    EXPECT_TRUE(std::greater<int>{}(2, 1));
}

// "std::bind vs. lambdas": the lambda and the bind expression compute the same thing.
TEST(Functional, BindVsLambda) {
    auto add = [](int a, int b) { return a + b; };
    auto add5 = [add](int b) { return add(5, b); };
    auto add5b = std::bind(add, 5, std::placeholders::_1);
    EXPECT_EQ(add5(10), 15);
    EXPECT_EQ(add5b(10), 15);
}
