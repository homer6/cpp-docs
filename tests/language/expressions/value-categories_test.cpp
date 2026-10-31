// Checks claims made by docs/examples/language/expressions/value-categories.md.
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

// Declared only; used inside decltype, never called.
int f_value();
int&& f_rref();

namespace {
std::string sink(const std::string&) { return "const&"; }
std::string sink(std::string&&) { return "&&"; }

template <class T> constexpr const char* category = "prvalue";
template <class T> constexpr const char* category<T&> = "lvalue";
template <class T> constexpr const char* category<T&&> = "xvalue";
#define CATEGORY(...) category<decltype((__VA_ARGS__))>

std::string named_rref(std::string&& x) { return sink(x); }
std::string forwarded_rref(std::string&& x) { return sink(std::move(x)); }

struct Moves {
    static inline int moves = 0;
    Moves() = default;
    Moves(const Moves&) = default;
    Moves(Moves&&) noexcept { ++moves; }
};
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wpessimizing-move"
#else
#pragma GCC diagnostic ignored "-Wpessimizing-move"
#endif
Moves make_plain() { Moves m; return m; }
Moves make_moved() { Moves m; return std::move(m); }  // the page's anti-pattern
}  // namespace

// "The taxonomy": examples of each category, checked with decltype((expr)).
TEST(ValueCategories, TaxonomyExamples) {
    int x = 0, a[2]{};
    int* p = &x;
    (void)a;
    EXPECT_STREQ(CATEGORY(x), "lvalue");
    EXPECT_STREQ(CATEGORY(*p), "lvalue");
    EXPECT_STREQ(CATEGORY(a[1]), "lvalue");
    EXPECT_STREQ(CATEGORY(42), "prvalue");
    EXPECT_STREQ(CATEGORY(x + 1), "prvalue");
    EXPECT_STREQ(CATEGORY(f_value()), "prvalue");
    EXPECT_STREQ(CATEGORY(std::move(x)), "xvalue");
    EXPECT_STREQ(CATEGORY(f_rref()), "xvalue");
}

// "Why it matters": overload resolution by category.
TEST(ValueCategories, OverloadsBindByCategory) {
    std::string s = "hi";
    EXPECT_EQ(sink(s), "const&");
    EXPECT_EQ(sink(std::move(s)), "&&");
    EXPECT_EQ(sink("temp"), "&&");  // converts to a temporary std::string
}

// "Reference binding rules": auto&& d = x is int&, auto&& e = 42 is int&&.
TEST(ValueCategories, ForwardingReferenceDeduction) {
    int x = 0;
    auto&& d = x;
    auto&& e = 42;
    static_assert(std::is_same_v<decltype(d), int&>);
    static_assert(std::is_same_v<decltype(e), int&&>);
    const int& b = 42;  // const lvalue ref binds to a temporary and extends its lifetime
    EXPECT_EQ(b, 42);
    EXPECT_EQ(d, 0);
    EXPECT_EQ(e, 42);
}

// "Gotchas": a named rvalue reference is itself an lvalue.
TEST(ValueCategories, NamedRvalueReferenceIsLvalue) {
    EXPECT_EQ(named_rref(std::string{"x"}), "const&");
}

// "Reference binding rules": int& takes lvalues only, int&& rvalues only, const int& anything.
TEST(ValueCategories, ReferenceBindingRules) {
    static_assert(std::is_constructible_v<int&, int&>);
    static_assert(!std::is_constructible_v<int&, int>);   // int& a = 42; is ill-formed
    static_assert(std::is_constructible_v<int&&, int>);
    static_assert(!std::is_constructible_v<int&&, int&>);  // int&& c = x; is ill-formed
    static_assert(std::is_constructible_v<const int&, int>);
    static_assert(std::is_constructible_v<const int&, int&>);
}

// "Why it matters": std::move is only a cast to an xvalue; it doesn't move anything.
TEST(ValueCategories, StdMoveIsACast) {
    std::string s = "kept";
    static_assert(std::is_same_v<decltype(std::move(s)), std::string&&>);
    [[maybe_unused]] std::string&& r = std::move(s);
    EXPECT_EQ(s, "kept");  // nothing moved: no move overload was selected
    EXPECT_EQ(forwarded_rref(std::string{"x"}), "&&");  // std::move(x) passes it on as movable
}

// "Gotchas": return std::move(local) forces a move and disables NRVO.
TEST(ValueCategories, ReturnStdMoveDisablesNrvo) {
    Moves::moves = 0;
    Moves a = make_moved();
    (void)a;
    EXPECT_EQ(Moves::moves, 1);
    Moves::moves = 0;
    Moves b = make_plain();
    (void)b;
    if (Moves::moves != 0) GTEST_SKIP() << "this toolchain didn't apply NRVO (it's optional)";
    EXPECT_EQ(Moves::moves, 0);
}
