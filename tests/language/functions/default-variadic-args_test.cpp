// Checks claims made by docs/examples/language/functions/default-variadic-args.md.
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include <gtest/gtest.h>

#include "support/capture.hpp"

namespace {
std::string last_call;
void connect(std::string_view host, int port = 8080, bool tls = true) {
    last_call = std::string(host) + ":" + std::to_string(port) + (tls ? " tls" : " plain");
}

template <class... Args> void log(Args&&... args) { (std::cout << ... << args) << '\n'; }
template <class... Args> constexpr std::size_t count(Args&&... args) { return sizeof...(args); }

template <class... T> auto sum(T... xs) { return (xs + ...); }
template <class... T> bool all(T... bs) { return (bs && ...); }
template <class... T> void print(T... xs) { ((std::cout << xs << ' '), ...); }
template <class... T> int right_minus(T... xs) { return (xs - ...); }         // (pack op ...)
template <class... T> int left_minus(T... xs) { return (... - xs); }          // (... op pack)
template <class... T> int right_init(T... xs) { return (xs - ... - 100); }    // (pack op ... op init)
template <class... T> int left_init(T... xs) { return (100 - ... - xs); }     // (init op ... op pack)

int next_id() { static int n = 0; return ++n; }
int take_id(int id = next_id()) { return id; }  // default evaluated at each call

int twice(int x) { return 2 * x; }
template <class... A> int add_all(A... a) { return (0 + ... + a); }
template <class... A> int add_twice(A... a) { return add_all(twice(a)...); }  // g applied to each

std::string category(int&) { return "lvalue"; }
std::string category(int&&) { return "rvalue"; }
template <class... Args> std::string forward_all(Args&&... args) {
    return ((category(std::forward<Args>(args)) + " ") + ...);
}
}  // namespace

namespace default_variadic_args_overloads {
int h(int);
int h(int, int = 0);  // h(1) now matches both
}  // namespace default_variadic_args_overloads

namespace {
template <class T> concept CallableH = requires(T t) { default_variadic_args_overloads::h(t); };
}  // namespace

// "Default arguments": omitted trailing arguments take the declared defaults.
TEST(DefaultVariadicArgs, DefaultArguments) {
    connect("example.com");
    EXPECT_EQ(last_call, "example.com:8080 tls");
    connect("example.com", 443);
    EXPECT_EQ(last_call, "example.com:443 tls");
    connect("example.com", 443, false);
    EXPECT_EQ(last_call, "example.com:443 plain");
}

// "Variadic templates": log(...) prints every argument; sizeof... gives the count.
TEST(DefaultVariadicArgs, VariadicTemplates) {
    EXPECT_EQ(cppdocs::capture_stdout([] { log("x=", 42, " y=", 3.14); }), "x=42 y=3.14\n");
    static_assert(count() == 0 && count(1, "a", 2.0) == 3);
}

// "Fold expressions": sum/all/print, and the four fold forms' associativity.
TEST(DefaultVariadicArgs, FoldExpressions) {
    EXPECT_EQ(sum(1, 2, 3), 6);
    EXPECT_TRUE(all(true, true));
    EXPECT_FALSE(all(true, false, true));
    EXPECT_EQ(cppdocs::capture_stdout([] { print(1, "two", 3); }), "1 two 3 ");
    EXPECT_EQ(right_minus(10, 4, 3), 9);   // 10 - (4 - 3)
    EXPECT_EQ(left_minus(10, 4, 3), 3);    // (10 - 4) - 3
    EXPECT_EQ(right_init(10, 4), 106);     // 10 - (4 - 100)
    EXPECT_EQ(left_init(10, 4), 86);       // (100 - 10) - 4
}

// "Gotchas": defaults are evaluated at the call site on every call.
TEST(DefaultVariadicArgs, DefaultEvaluatedEachCall) {
    const int a = take_id();
    const int b = take_id();
    EXPECT_EQ(b, a + 1);
    EXPECT_EQ(take_id(42), 42);
}

// "Gotchas": a defaulted parameter can make two overloads ambiguous.
TEST(DefaultVariadicArgs, DefaultsPlusOverloadingAmbiguous) {
    static_assert(!CallableH<int>);  // h(int) vs h(int, int = 0): ambiguous
}

// "Gotchas": `...` placement, and perfect forwarding preserving value categories.
TEST(DefaultVariadicArgs, PackExpansionAndForwarding) {
    EXPECT_EQ(add_all(1, 2, 3), 6);
    EXPECT_EQ(add_twice(1, 2, 3), 12);  // add_all(twice(1), twice(2), twice(3))
    int x = 0;
    EXPECT_EQ(forward_all(x, 1, std::move(x)), "lvalue rvalue rvalue ");
}
