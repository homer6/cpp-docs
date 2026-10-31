// Checks claims made by docs/examples/containers/sequence/array.md.
#include <array>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

// "Mental model": the elements live inside the object, with no extra overhead.
TEST(Array, ElementsAreInline) {
    std::array<int, 4> a{1, 2, 3, 4};
    static_assert(sizeof(std::array<int, 4>) == sizeof(int[4]));
    const auto* lo = reinterpret_cast<const unsigned char*>(&a);
    const auto* p = reinterpret_cast<const unsigned char*>(a.data());
    EXPECT_GE(p, lo);
    EXPECT_LE(p + sizeof(int[4]), lo + sizeof a);
}

// "Declaration": an aggregate; N == 0 is allowed.
TEST(Array, AggregateAndEmpty) {
    static_assert(std::is_aggregate_v<std::array<int, 3>>);
    std::array<int, 0> e{};
    EXPECT_TRUE(e.empty());
    EXPECT_EQ(e.size(), 0u);
}

// "Initialization (mind the braces)": {} zeroes, {1} value-initializes the rest, CTAD.
TEST(Array, Initialization) {
    std::array<int, 3> a{1, 2, 3};
    EXPECT_EQ(a, (std::array<int, 3>{1, 2, 3}));
    std::array<int, 3> z{};
    EXPECT_EQ(z, (std::array<int, 3>{0, 0, 0}));
    std::array<int, 3> p{1};
    EXPECT_EQ(p, (std::array<int, 3>{1, 0, 0}));
    std::array b{1, 2, 3};
    static_assert(std::is_same_v<decltype(b), std::array<int, 3>>);
}

// "Access and iteration": at() throws std::out_of_range; size() is constexpr and == 3.
TEST(Array, Access) {
    std::array<int, 3> a{1, 2, 3};
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a.at(0), 1);
    EXPECT_THROW((void)a.at(3), std::out_of_range);
    EXPECT_EQ(a.front(), 1);
    EXPECT_EQ(a.back(), 3);
    EXPECT_EQ(a.data(), &a[0]);
    static_assert(a.size() == 3);
    int sum = 0;
    for (int x : a) sum += x;
    EXPECT_EQ(sum, 6);
}

// "Iterator & reference invalidation": swap keeps iterators valid but swaps the elements.
TEST(Array, SwapKeepsIteratorsSwapsElements) {
    std::array<int, 3> a{1, 2, 3}, b{7, 8, 9};
    auto it = a.begin();
    a.swap(b);
    EXPECT_EQ(it, a.begin());
    EXPECT_EQ(*it, 7);
    a.fill(5);
    EXPECT_EQ(a, (std::array<int, 3>{5, 5, 5}));
}

// "constexpr / compile-time use"
TEST(Array, Constexpr) {
    constexpr std::array<int, 5> squares = [] {
        std::array<int, 5> r{};
        for (int i = 0; i < 5; ++i) r[i] = i * i;
        return r;
    }();
    static_assert(squares[4] == 16);
}

// "Structured bindings & std::get"
TEST(Array, StructuredBindingsAndGet) {
    std::array<int, 3> pt{10, 20, 30};
    auto [x, y, z2] = pt;
    EXPECT_EQ(x, 10);
    EXPECT_EQ(y, 20);
    EXPECT_EQ(z2, 30);
    EXPECT_EQ(std::get<1>(pt), 20);
}

// "to_array": deduces the size; a string literal keeps its '\0'.
TEST(Array, ToArray) {
    auto a = std::to_array({1, 2, 3});
    static_assert(std::is_same_v<decltype(a), std::array<int, 3>>);
    auto s = std::to_array("hi");
    static_assert(std::is_same_v<decltype(s), std::array<char, 3>>);
    EXPECT_EQ(s[2], '\0');
}
