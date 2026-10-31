// Checks claims made by docs/examples/language/declarations-initialization/declarations-and-auto.md.
#include <array>
#include <map>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace {
int global = 0;
decltype(auto) ref_to_global() { return (global); }  // decltype(auto) keeps the reference
auto copy_of_global() { return (global); }           // plain auto strips it

auto add(int a, int b) -> int { return a + b; }
auto multiply(int a, int b) { return a * b; }

template <class T, class U>
auto sum(T t, U u) -> decltype(t + u) { return t + u; }

struct Point { int x, y; };
}  // namespace

// "auto": the deduced type of each declaration.
TEST(DeclarationsAuto, AutoDeduction) {
    using namespace std::string_literals;
    int x = 0;
    auto i = 42;
    auto d = 3.14;
    auto s = "hi"s;
    auto& r = x;
    const auto& cr = x;
    auto* p = &x;
    static_assert(std::is_same_v<decltype(i), int>);
    static_assert(std::is_same_v<decltype(d), double>);
    static_assert(std::is_same_v<decltype(s), std::string>);
    static_assert(std::is_same_v<decltype(r), int&>);
    static_assert(std::is_same_v<decltype(cr), const int&>);
    static_assert(std::is_same_v<decltype(p), int*>);
    r = 5;
    EXPECT_EQ(x, 5);
    EXPECT_EQ(&cr, &x);  // no copy

    auto lambda = [](int v) { return v * v; };
    EXPECT_EQ(lambda(4), 16);

    // auto strips top-level const and references unless you write them.
    const int& cref = x;
    auto c1 = cref;
    auto& c2 = cref;
    static_assert(std::is_same_v<decltype(c1), int>);
    static_assert(std::is_same_v<decltype(c2), const int&>);
    EXPECT_NE(&c1, &x);
    EXPECT_EQ(&c2, &x);
}

// "decltype": decltype(x) is int, decltype((x)) is int&; decltype(auto) keeps references.
TEST(DeclarationsAuto, Decltype) {
    int x = 0;
    static_assert(std::is_same_v<decltype(x), int>);
    static_assert(std::is_same_v<decltype((x)), int&>);
    static_assert(std::is_same_v<decltype(ref_to_global()), int&>);
    static_assert(std::is_same_v<decltype(copy_of_global()), int>);
    ref_to_global() = 9;
    EXPECT_EQ(global, 9);
    EXPECT_EQ(copy_of_global(), 9);
}

// "Trailing return types & deduced returns".
TEST(DeclarationsAuto, ReturnTypes) {
    EXPECT_EQ(add(2, 3), 5);
    static_assert(std::is_same_v<decltype(multiply(2, 3)), int>);
    EXPECT_EQ(multiply(2, 3), 6);
    static_assert(std::is_same_v<decltype(sum(1, 2.5)), double>);
    EXPECT_EQ(sum(1, 2.5), 3.5);
}

// "Structured bindings": by copy from a pair, by reference to mutate, from std::array.
TEST(DeclarationsAuto, StructuredBindings) {
    Point pt{1, 2};
    std::pair<int, std::string> p{1, "one"};
    auto [id, name] = p;
    EXPECT_EQ(id, 1);
    EXPECT_EQ(name, "one");

    auto& [a, b] = pt;
    a = 10;
    b = 20;
    EXPECT_EQ(pt.x, 10);
    EXPECT_EQ(pt.y, 20);

    auto [x, y, z] = std::array{1, 2, 3};
    EXPECT_EQ(x + y + z, 6);

    std::map<std::string, int> m{{"k", 7}};
    for (const auto& [k, v] : m) {
        EXPECT_EQ(k, "k");
        EXPECT_EQ(v, 7);
    }
}

// Gotchas: auto&& is a forwarding reference — lvalue ref for lvalues, rvalue ref for rvalues.
TEST(DeclarationsAuto, AutoForwardingReference) {
    int x = 0;
    auto&& l = x;
    auto&& rv = 5;
    static_assert(std::is_same_v<decltype(l), int&>);
    static_assert(std::is_same_v<decltype(rv), int&&>);
    auto total = 0;
    static_assert(std::is_same_v<decltype(total), int>);  // "hides that it's an int"
    EXPECT_EQ(&l, &x);
    EXPECT_EQ(rv + total, 5);
}
