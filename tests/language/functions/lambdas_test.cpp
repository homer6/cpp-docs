// Checks claims made by docs/examples/language/functions/lambdas.md.
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

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
