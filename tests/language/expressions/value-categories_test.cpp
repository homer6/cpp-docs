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
