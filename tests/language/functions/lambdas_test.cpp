// Checks claims made by docs/examples/language/functions/lambdas.md.
#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Anatomy" and "Captures": by-value captures copy, by-reference captures alias.
TEST(Lambdas, CaptureByValueVersusReference) {
    int factor = 3;
    auto f = [factor](int x) -> int { return x * factor; };
    factor = 10;
    EXPECT_EQ(f(2), 6);  // the copy was taken at creation

    int a = 1;
    [&a] { a = 5; }();
    EXPECT_EQ(a, 5);

    auto ptr = std::make_unique<int>(7);
    auto g = [p = std::move(ptr)] { return *p; };  // move-capture (C++14)
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(g(), 7);
    EXPECT_EQ([x = a + 1] { return x; }(), 6);  // init-capture
}

// "Generic, mutable, constexpr": generic lambdas, a mutable counter, compile-time use.
TEST(Lambdas, GenericMutableConstexpr) {
    auto add = [](auto a, auto b) { return a + b; };
    EXPECT_EQ(add(1, 2), 3);
    EXPECT_EQ(add(std::string("a"), std::string("b")), "ab");

    auto counter = [n = 0]() mutable { return ++n; };
    EXPECT_EQ(counter(), 1);
    EXPECT_EQ(counter(), 2);

    constexpr auto square = [](int x) { return x * x; };
    static_assert(square(4) == 16);
}

// "Generic, mutable, constexpr, consteval": lambdas are implicitly constexpr (C++17);
// a consteval lambda (C++20) runs only at compile time.
TEST(Lambdas, ImplicitConstexprAndConsteval) {
    auto twice = [](int x) { return 2 * x; };
    static_assert(twice(21) == 42);
    std::array<int, twice(2)> four{};
    static_assert(four.size() == 4);
    volatile int n = 5;
    EXPECT_EQ(twice(n), 10);  // the same lambda at run time

    auto cube = [](int x) consteval { return x * x * x; };
    static_assert(cube(3) == 27);
    EXPECT_EQ(cube(2), 8);  // a constant argument: evaluated at compile time
}

// "Gotchas": by-value captures are const unless mutable (the call operator is const).
TEST(Lambdas, ByValueCapturesAreConstWithoutMutable) {
    int n = 0;
    auto plain = [n] { return n; };
    auto mut = [n]() mutable { return ++n; };
    static_assert(std::is_invocable_v<const decltype(plain)&>);
    static_assert(!std::is_invocable_v<const decltype(mut)&>);
    EXPECT_EQ(mut(), 1);
    EXPECT_EQ(plain(), 0);
}

// "Gotchas": each lambda has a unique type, even with identical text.
TEST(Lambdas, EachLambdaHasAUniqueType) {
    auto l1 = [] { return 1; };
    auto l2 = [] { return 1; };
    static_assert(!std::is_same_v<decltype(l1), decltype(l2)>);
    EXPECT_EQ(l1(), l2());
}

namespace {
struct Counter {
    int value = 1;
    auto by_copy() { return [*this] { return value; }; }  // [*this] copies the object (C++17)
    auto by_pointer() { return [this] { return value; }; }
};
}  // namespace

// "Captures": [this] captures the pointer; [*this] captures a copy of the object.
TEST(Lambdas, ThisVersusStarThis) {
    Counter c;
    auto copy = c.by_copy();
    auto ptr = c.by_pointer();
    c.value = 2;
    EXPECT_EQ(copy(), 1);
    EXPECT_EQ(ptr(), 2);
}

namespace {
template <class... Ts> auto sum_later(Ts... xs) {
    return [... xs = std::move(xs)] { return (xs + ...); };
}
int global_hits = 0;
}  // namespace

// "Captures": C++20 pack init-capture, structured-binding capture; globals aren't captured.
TEST(Lambdas, Cxx20CaptureForms) {
    EXPECT_EQ(sum_later(1, 2, 3)(), 6);
    auto [x, y] = std::pair{1, 2};
    auto add = [x, y] { return x + y; };
    EXPECT_EQ(add(), 3);
    auto bump = [] { return ++global_hits; };
    bump();
    EXPECT_EQ(global_hits, 1);
}

// "Captures": [a, &b], [=], [&] and [=, &b] defaults.
TEST(Lambdas, DefaultAndMixedCaptures) {
    int a = 1, b = 2;
    [a, &b] { b = a + 10; }();
    EXPECT_EQ(b, 11);
    EXPECT_EQ([=] { return a + b; }(), 12);
    [&] { a = b; }();
    EXPECT_EQ(a, 11);
    auto f = [=, &b] { b = 0; return a; };
    a = 99;
    EXPECT_EQ(f(), 11);  // a was copied, b aliased
    EXPECT_EQ(b, 0);
}

// "Generic, mutable, constexpr": explicit template parameters (C++20).
TEST(Lambdas, ExplicitTemplateParameters) {
    auto g = []<class T>(T x) { return x; };
    static_assert(std::is_same_v<decltype(g(1)), int>);
    static_assert(std::is_same_v<decltype(g(1.5)), double>);
    EXPECT_EQ(g(std::string("s")), "s");
}

// "Where they shine": sort with a comparator, erase_if with a predicate.
TEST(Lambdas, WhereTheyShine) {
    struct Player { std::string name; int score; };
    std::vector<Player> players{{"a", 1}, {"b", 3}, {"c", 2}};
    std::sort(players.begin(), players.end(), [](auto& x, auto& y) { return x.score > y.score; });
    EXPECT_EQ(players[0].name + players[1].name + players[2].name, "bca");

    std::map<std::string, int> m{{"x", 0}, {"y", 1}};
    std::erase_if(m, [](const auto& kv) { return kv.second == 0; });
    EXPECT_EQ(m.size(), 1u);
    EXPECT_TRUE(m.contains("y"));
}

// "Where they shine": std::function / std::move_only_function give a uniform type.
TEST(Lambdas, StoredInTypeErasedWrappers) {
    std::vector<std::function<int(int)>> fs{[](int x) { return x + 1; }, [k = 2](int x) { return x * k; }};
    EXPECT_EQ(fs[0](5), 6);
    EXPECT_EQ(fs[1](5), 10);
    CPPDOCS_REQUIRE(__cpp_lib_move_only_function);
#ifdef __cpp_lib_move_only_function
    std::move_only_function<int()> mo = [p = std::make_unique<int>(7)] { return *p; };  // move-only capture
    EXPECT_EQ(mo(), 7);
#endif
}
