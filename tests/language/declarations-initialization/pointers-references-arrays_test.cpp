// Checks claims made by docs/examples/language/declarations-initialization/pointers-references-arrays.md.
#include <array>
#include <cstddef>
#include <iterator>
#include <span>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

namespace {
void inc(int& n) { ++n; }

using F = void(int arr[], int n);  // the page's f, as a function type
std::size_t size_of_span(std::span<const int> s) { return s.size(); }

std::string pick(int) { return "int"; }
std::string pick(int*) { return "pointer"; }
}  // namespace

// "Pointers": nullable and rebindable; dereference reads/writes the pointee.
TEST(PointersReferencesArrays, Pointers) {
    int x = 10;
    int* p = &x;
    *p = 20;
    EXPECT_EQ(x, 20);
    p = nullptr;
    EXPECT_EQ(p, nullptr);
}

// "References": an alias — writes through it change the original; rvalue refs bind temporaries.
TEST(PointersReferencesArrays, References) {
    int x = 10;
    int& r = x;
    r = 30;
    EXPECT_EQ(x, 30);
    inc(x);
    EXPECT_EQ(x, 31);  // "x is now 31"
    EXPECT_EQ(&r, &x);

    int&& rr = 5;
    EXPECT_EQ(rr, 5);
    static_assert(!std::is_constructible_v<int&, int>);   // a non-const lvalue ref can't bind a temporary
    static_assert(std::is_constructible_v<int&&, int>);
    static_assert(!std::is_default_constructible_v<int&>);  // must be initialized
}

// "Arrays": missing elements are zero; std::size is 5; passing decays to a pointer.
TEST(PointersReferencesArrays, Arrays) {
    int a[5] = {1, 2, 3};
    EXPECT_EQ(a[3], 0);
    EXPECT_EQ(a[4], 0);
    int n = std::size(a);
    EXPECT_EQ(n, 5);
    EXPECT_EQ(std::size(a), sizeof(a) / sizeof(a[0]));

    static_assert(std::is_same_v<std::decay_t<int[5]>, int*>);
    static_assert(std::is_same_v<F, void(int*, int)>);  // the parameter is a pointer: size lost
    EXPECT_EQ(size_of_span(a), 5u);      // a span keeps it
    static_assert(std::is_same_v<std::decay_t<std::array<int, 5>>, std::array<int, 5>>);  // no decay
}

// Gotchas: nullptr has its own type and picks the pointer overload.
TEST(PointersReferencesArrays, NullptrPicksPointerOverload) {
    static_assert(std::is_same_v<decltype(nullptr), std::nullptr_t>);
    EXPECT_EQ(pick(nullptr), "pointer");
    EXPECT_EQ(pick(0), "int");
}
