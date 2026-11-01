// Checks claims made by docs/examples/utilities/functional.md.
#include <algorithm>
#include <functional>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

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

// "bind_front / bind_back": front prepends, back appends; a member function binds to an object pointer.
TEST(Functional, BindFrontBack) {
    auto sub = [](int a, int b) { return a - b; };
    auto from10 = std::bind_front(sub, 10);
    EXPECT_EQ(from10(3), 7);
    CPPDOCS_REQUIRE(__cpp_lib_bind_back);
#ifdef __cpp_lib_bind_back
    auto minus10 = std::bind_back(sub, 10);
    EXPECT_EQ(minus10(3), -7);
#endif
    struct Counter {
        int n = 0;
        int add(int k) { return n += k; }
    };
    Counter c;
    auto add_to_c = std::bind_front(&Counter::add, &c);
    add_to_c(5);
    EXPECT_EQ(c.n, 5);
}

// Gotcha "bind_front copies its arguments": binding obj by value leaves the original alone.
TEST(Functional, BindFrontCopies) {
    struct Counter {
        int n = 0;
        int add(int k) { return n += k; }
    };
    Counter c;
    auto on_copy = std::bind_front(&Counter::add, c);
    EXPECT_EQ(on_copy(5), 5);
    EXPECT_EQ(c.n, 0);
    auto on_ref = std::bind_front(&Counter::add, std::ref(c));
    on_ref(5);
    EXPECT_EQ(c.n, 5);
}

// "not_fn (C++17)": counts the odd numbers in {1..5} == 3.
TEST(Functional, NotFn) {
    auto is_even = [](int x) { return x % 2 == 0; };
    std::vector<int> xs{1, 2, 3, 4, 5};
    EXPECT_EQ(std::count_if(xs.begin(), xs.end(), std::not_fn(is_even)), 3);
}

namespace {
struct Person {
    std::string name;
    int age;
    bool adult() const { return age >= 18; }
};
[[maybe_unused]] bool is_even_fn(int x) { return x % 2 == 0; }
[[maybe_unused]] int sub_fn(int a, int b) { return a - b; }
template <auto F>
concept has_constant_bind_front = requires { std::bind_front<F>(10); };
}  // namespace

// "mem_fn (C++11)": member function predicate, data member through a reference and a pointer; ranges need no mem_fn.
TEST(Functional, MemFn) {
    std::vector<Person> people{{"Ann", 30}, {"Bo", 10}};
    EXPECT_EQ(std::count_if(people.begin(), people.end(), std::mem_fn(&Person::adult)), 1);
    auto age = std::mem_fn(&Person::age);
    EXPECT_EQ(age(people[0]), 30);
    EXPECT_EQ(age(&people[1]), 10);
    EXPECT_EQ(std::ranges::count_if(people, &Person::adult), 1);
}

// "Constant-callable forms (C++26)": not_fn<f>() (GCC 16, libc++ 22).
TEST(Functional, NotFnConstant) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_not_fn, 202306L);
#if __cpp_lib_not_fn >= 202306L
    auto odd = std::not_fn<is_even_fn>();
    EXPECT_TRUE(odd(3));
    EXPECT_FALSE(odd(4));
#endif
}

// "Constant-callable forms (C++26)": bind_front<f> / bind_back<f> store only the bound int (GCC 16).
TEST(Functional, BindConstant) {
#ifdef _LIBCPP_VERSION
    GTEST_SKIP() << "libc++ defines __cpp_lib_bind_front 202306L but lacks bind_front<f>() (the page says so)";
#else
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_bind_front, 202306L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_bind_back, 202306L);
#if __cpp_lib_bind_front >= 202306L && __cpp_lib_bind_back >= 202306L
    auto from10 = std::bind_front<sub_fn>(10);
    auto minus10 = std::bind_back<sub_fn>(10);
    EXPECT_EQ(from10(3), 7);
    EXPECT_EQ(minus10(3), -7);
    static_assert(sizeof(from10) == sizeof(int));
#endif
#endif
}

// "Constant-callable forms (C++26)": libc++ 22 advertises 202306L for bind_front, but bind_back is still 202202L.
TEST(Functional, LibcxxNttpGap) {
#ifndef _LIBCPP_VERSION
    GTEST_SKIP() << "libc++-specific claim";
#else
#if _LIBCPP_VERSION >= 220000 && _LIBCPP_VERSION < 230000
    EXPECT_EQ(__cpp_lib_bind_front, 202306L);
    EXPECT_EQ(__cpp_lib_bind_back, 202202L);
    static_assert(!has_constant_bind_front<sub_fn>);
#else
    GTEST_SKIP() << "claim is about libc++ 22";
#endif
#endif
}
