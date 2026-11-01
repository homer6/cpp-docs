// Checks claims made by docs/examples/utilities/callable-wrappers.md.
#include <functional>
#include <memory>
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "std::function": reassign to std::multiplies, 6 * 7 == 42; calling an empty one throws bad_function_call.
TEST(CallableWrappers, StdFunction) {
    std::function<int(int, int)> op = [](int a, int b) { return a + b; };
    EXPECT_EQ(op(6, 7), 13);
    op = std::multiplies<int>{};
    EXPECT_EQ(op(6, 7), 42);
    EXPECT_TRUE(static_cast<bool>(op));

    std::function<int(int, int)> empty;
    EXPECT_FALSE(static_cast<bool>(empty));
    EXPECT_THROW(empty(1, 2), std::bad_function_call);
}

// Gotcha "std::function's const operator() calls non-const targets": a const function mutates a mutable lambda.
TEST(CallableWrappers, FunctionConstLies) {
    const std::function<int()> f = [n = 0]() mutable { return ++n; };
    EXPECT_EQ(f(), 1);
    EXPECT_EQ(f(), 2);
}

// "std::move_only_function (C++23)": holds a move-only callable and is itself move-only.
TEST(CallableWrappers, MoveOnlyFunction) {
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

// "std::copyable_function (C++26)": copies, converts to move_only_function, rejects a mutable lambda for a const signature.
TEST(CallableWrappers, CopyableFunction) {
    CPPDOCS_REQUIRE(__cpp_lib_copyable_function);
#ifdef __cpp_lib_copyable_function
    std::copyable_function<int(int) const> add3 = [k = 3](int x) { return x + k; };
    auto copy = add3;
    EXPECT_EQ(copy(1), 4);
    EXPECT_EQ(add3(1), 4);
    std::move_only_function<int(int) const> m = add3;
    EXPECT_EQ(m(1), 4);
    static_assert(std::is_copy_constructible_v<std::copyable_function<void()>>);

    using Counter = decltype([n = 0]() mutable { return ++n; });
    static_assert(!std::is_constructible_v<std::copyable_function<int() const>, Counter>);
    static_assert(std::is_constructible_v<std::function<int()>, Counter>);

    std::copyable_function<void()> empty;
    EXPECT_FALSE(static_cast<bool>(empty));
#endif
}

#ifdef __cpp_lib_function_ref
namespace {
int twice(int x) { return 2 * x; }
int apply(std::function_ref<int(int)> f, int x) { return f(x); }
struct Counter {
    int n = 0;
    int add(int k) { return n += k; }
};
}  // namespace
#endif

// "std::function_ref (C++26)": refers to a lambda or function, binds std::cw<f> and a member + object; trivially copyable, no default constructor.
TEST(CallableWrappers, FunctionRef) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_function_ref, 202603L);  // P3948: std::cw, not std::nontype
#if __cpp_lib_function_ref >= 202603L
    int base = 10;
    auto plus_base = [&](int x) { return base + x; };
    EXPECT_EQ(apply(plus_base, 1), 11);
    base = 20;  // refers to the lambda, which refers to base
    EXPECT_EQ(apply(plus_base, 1), 21);
    EXPECT_EQ(apply(twice, 4), 8);

    std::function_ref<int(int)> r{std::cw<twice>};
    EXPECT_EQ(r(5), 10);
    Counter c;
    std::function_ref<int(int)> add{std::cw<&Counter::add>, c};
    add(2);
    add(3);
    EXPECT_EQ(c.n, 5);

    static_assert(std::is_trivially_copyable_v<std::function_ref<void()>>);
    static_assert(!std::is_default_constructible_v<std::function_ref<void()>>);
#endif
}
