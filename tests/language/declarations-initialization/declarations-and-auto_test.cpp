// Checks claims made by docs/examples/language/declarations-initialization/declarations-and-auto.md.
#include <array>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

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

namespace sb_cpp26 {
#if __cpp_structured_bindings >= 202411L
template <class Tuple>
constexpr auto pack_sum(const Tuple& t) {
    auto [...xs] = t;
    return (xs + ... + 0);
}
constexpr auto head_and_count = [](auto t) {
    auto [head, ...rest] = t;
    return head * 100 + sizeof...(rest);
};
struct Point2 { int x, y; };
constexpr Point2 origin{3, 4};
constexpr auto [ox, oy] = origin;
constexpr std::array table{1, 2, 3};
constexpr auto& [t0, t1, t2] = table;
#endif
}  // namespace sb_cpp26

// "`_` — the placeholder name (C++26)": `_` may be declared more than once in a scope.
TEST(DeclarationsAuto, PlaceholderName) {
    CPPDOCS_REQUIRE_VERSION(__cpp_placeholder_variables, 202306L);
#if __cpp_placeholder_variables >= 202306L
    auto [x, _] = std::pair{1, 2.0};
    auto [y, _] = std::pair{3, 4.0};
    static std::mutex m;
    std::lock_guard _{m};
    EXPECT_EQ(x + y, 4);
    EXPECT_FALSE(m.try_lock());  // the unnamed guard still holds the lock
#endif
}

// "Binding a pack (C++26)": `...xs` binds every element; only inside a template.
TEST(DeclarationsAuto, StructuredBindingPack) {
    CPPDOCS_REQUIRE_VERSION(__cpp_structured_bindings, 202411L);
#if __cpp_structured_bindings >= 202411L
    static_assert(sb_cpp26::pack_sum(std::tuple{1, 2, 3}) == 6);
    static_assert(sb_cpp26::head_and_count(std::tuple{1, 2, 3}) == 102);
    EXPECT_EQ(sb_cpp26::pack_sum(std::tuple{}), 0);  // zero elements is fine
#endif
}

// "`constexpr` structured bindings (C++26)". P2686 has no macro of its own; the
// toolchains that have it (GCC 16, Clang 22) are the ones defining the P1061 value.
TEST(DeclarationsAuto, ConstexprStructuredBindings) {
    CPPDOCS_REQUIRE_VERSION(__cpp_structured_bindings, 202411L);
#if __cpp_structured_bindings >= 202411L
    static_assert(sb_cpp26::ox + sb_cpp26::oy == 7);
    static_assert(sb_cpp26::t2 == 3);
    EXPECT_EQ(&sb_cpp26::t0, &sb_cpp26::table[0]);  // constexpr auto& refers to the static object
#endif
}
