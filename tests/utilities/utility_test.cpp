// Checks claims made by docs/examples/utilities/utility.md.
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
struct Tracker {
    static inline int copies = 0, moves = 0;
    Tracker() = default;
    Tracker(const Tracker&) { ++copies; }
    Tracker(Tracker&&) noexcept { ++moves; }
    static void reset() { copies = moves = 0; }
};

std::string sink(const std::string&) { return "lvalue"; }
std::string sink(std::string&&) { return "rvalue"; }
template <class T>
std::string relay(T&& x) { return sink(std::forward<T>(x)); }

namespace lib {
struct Widget { int v; bool custom_swapped = false; };
void swap(Widget& a, Widget& b) noexcept {
    std::swap(a.v, b.v);
    a.custom_swapped = b.custom_swapped = true;
}
}  // namespace lib

Tracker make_moved() {
    Tracker local;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpessimizing-move"  // the page's anti-pattern, on purpose
    return std::move(local);
#pragma GCC diagnostic pop
}
}  // namespace

// "std::move": a cast to an rvalue reference that moves nothing by itself.
TEST(Utility, MoveIsJustACast) {
    std::string a = "hello";
    static_assert(std::is_same_v<decltype(std::move(a)), std::string&&>);
    std::string&& r = std::move(a);
    EXPECT_EQ(a, "hello");  // nothing stolen yet
    std::string b = std::move(r);  // the move constructor does the stealing
    EXPECT_EQ(b, "hello");
    a = "new value";  // assigning a moved-from object is fine
    EXPECT_EQ(a, "new value");
}

// "std::forward": lvalue stays lvalue, rvalue stays rvalue.
TEST(Utility, ForwardPreservesValueCategory) {
    std::string s = "x";
    EXPECT_EQ(relay(s), "lvalue");
    EXPECT_EQ(relay(std::string{"x"}), "rvalue");
    EXPECT_EQ(relay(std::move(s)), "rvalue");
}

// "std::swap": x==2, y==1; ADL finds a user swap; std types swap in O(1) (buffers exchanged).
TEST(Utility, Swap) {
    int x = 1, y = 2;
    std::swap(x, y);
    EXPECT_EQ(x, 2);
    EXPECT_EQ(y, 1);

    lib::Widget a{1}, b{2};
    using std::swap;
    swap(a, b);
    EXPECT_TRUE(a.custom_swapped);
    EXPECT_EQ(a.v, 2);

    std::vector<int> v1(1000, 1), v2(10, 2);
    const int* p1 = v1.data();
    const int* p2 = v2.data();
    std::swap(v1, v2);
    EXPECT_EQ(v1.data(), p2);  // no element copied: the buffers changed hands
    EXPECT_EQ(v2.data(), p1);
}

// "std::exchange": x becomes 42, old holds 7; the move-constructor idiom leaves the source null.
TEST(Utility, Exchange) {
    int x = 7;
    int old = std::exchange(x, 42);
    EXPECT_EQ(x, 42);
    EXPECT_EQ(old, 7);

    int target = 0;
    int* other = &target;
    int* ptr = std::exchange(other, nullptr);
    EXPECT_EQ(ptr, &target);
    EXPECT_EQ(other, nullptr);
}

// "Safe integer comparison (C++20)": -1 < 0u is false, cmp_less(-1, 0u) is true; in_range<size_t>(-1) is false.
TEST(Utility, SafeIntegerComparison) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
    static_assert(!(-1 < 0u));
#pragma GCC diagnostic pop
    static_assert(std::cmp_less(-1, 0u));
    int x = -1;
    unsigned y = 0;
    EXPECT_FALSE(std::cmp_greater_equal(x, y));
    EXPECT_FALSE(std::in_range<std::size_t>(-1));
    EXPECT_TRUE(std::in_range<std::size_t>(0));
        EXPECT_TRUE(std::cmp_equal(0, 0u));
    EXPECT_TRUE(std::cmp_not_equal(-1, static_cast<unsigned>(-1)));
    EXPECT_TRUE(std::cmp_greater(0u, -1));
    EXPECT_TRUE(std::cmp_less_equal(-1, 0u));
}

// "Smaller helpers": as_const gives a const&; to_underlying gives the enum's integer; pair binds.
TEST(Utility, SmallerHelpers) {
    std::string obj = "text";
    static_assert(std::is_same_v<decltype(std::as_const(obj)), const std::string&>);
    EXPECT_EQ(&std::as_const(obj), &obj);

    CPPDOCS_REQUIRE(__cpp_lib_to_underlying);
#ifdef __cpp_lib_to_underlying
    enum class Color : unsigned char { red, green };
    static_assert(std::is_same_v<decltype(std::to_underlying(Color::green)), unsigned char>);
    EXPECT_EQ(std::to_underlying(Color::green), 1);
#endif
    auto [a, b] = std::pair{1, 2};
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
}

// "Smaller helpers": std::unreachable (C++23) exists and is [[noreturn]] void; never call it.
TEST(Utility, Unreachable) {
    CPPDOCS_REQUIRE(__cpp_lib_unreachable);
#ifdef __cpp_lib_unreachable
    static_assert(std::is_same_v<decltype(std::unreachable()), void>);
    auto pick = [](int i) {
        switch (i) {
            case 0: return 10;
            case 1: return 20;
        }
        std::unreachable();
    };
    EXPECT_EQ(pick(1), 20);
#endif
}

// "Gotchas": std::move on a const object binds to the copy constructor — it silently copies.
TEST(Utility, MoveFromConstCopies) {
    const Tracker c;
    Tracker::reset();
    Tracker t = std::move(c);
    (void)t;
    EXPECT_EQ(Tracker::copies, 1);
    EXPECT_EQ(Tracker::moves, 0);
}

// "Gotchas": return std::move(local) disables elision — it always costs a move.
TEST(Utility, ReturnStdMoveForcesAMove) {
    Tracker::reset();
    Tracker t = make_moved();
    (void)t;
    EXPECT_EQ(Tracker::moves, 1);  // `return local;` may elide this (NRVO is permitted, not required)
    EXPECT_EQ(Tracker::copies, 0);
}
