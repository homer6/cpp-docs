// Checks claims made by docs/examples/language/functions/functions-and-overloading.md.
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <gtest/gtest.h>

namespace {
std::string mutate(std::string& s) { return s += "!"; }
std::string take(std::string s) { s += "?"; return s; }

std::string_view print(int) { return "int"; }
std::string_view print(double) { return "double"; }
std::string_view print(const std::string&) { return "string"; }

std::string_view rank(int) { return "int"; }
[[maybe_unused]] std::string_view rank(long) { return "long"; }

[[noreturn]] void fail() { throw std::runtime_error("fail"); }
int g(int x [[maybe_unused]]) { return 0; }  // no -Wunused-parameter

struct S {
    S() = default;
    S(const S&) = delete;
};
}  // namespace

namespace functions_and_overloading_decls {
void f(int);
void f(double) = delete;  // bans f(3.14), which would otherwise convert double -> int
void amb(long);
void amb(double);
}  // namespace functions_and_overloading_decls

namespace {
template <class T> concept CallableF = requires(T t) { functions_and_overloading_decls::f(t); };
template <class T> concept CallableAmb = requires(T t) { functions_and_overloading_decls::amb(t); };
}  // namespace

// "Passing parameters": & modifies the caller's object; by value works on a copy.
TEST(FunctionsAndOverloading, PassingParameters) {
    std::string s = "hi";
    EXPECT_EQ(take(s), "hi?");
    EXPECT_EQ(s, "hi");
    mutate(s);
    EXPECT_EQ(s, "hi!");
}

// "Overloading": the call picks print(int) / print(double) by argument type.
TEST(FunctionsAndOverloading, OverloadResolution) {
    EXPECT_EQ(print(42), "int");
    EXPECT_EQ(print(3.14), "double");
    EXPECT_EQ(print(std::string("x")), "string");
    // exact > promotion > conversion
    EXPECT_EQ(print('a'), "int");     // char -> int is a promotion; char -> double a conversion
    EXPECT_EQ(print(1.0f), "double");  // float -> double is a promotion
    EXPECT_EQ(rank(short{1}), "int");  // promotion beats short -> long conversion
    static_assert(!CallableAmb<int>);  // int -> long and int -> double: both conversions, ambiguous
}

// "Attributes that matter": [[noreturn]] fail() leaves by throwing; maybe_unused silences.
TEST(FunctionsAndOverloading, Attributes) {
    EXPECT_THROW(fail(), std::runtime_error);
    EXPECT_EQ(g(1), 0);
    static_assert(__has_cpp_attribute(nodiscard) >= 201907L);  // nodiscard("reason"): C++20
}

// "Default vs deleted": = default / = delete members, and a deleted overload.
TEST(FunctionsAndOverloading, DefaultVersusDeleted) {
    static_assert(std::is_default_constructible_v<S>);
    static_assert(!std::is_copy_constructible_v<S>);
    static_assert(CallableF<int>);
    static_assert(!CallableF<double>);  // picks the deleted overload
}
