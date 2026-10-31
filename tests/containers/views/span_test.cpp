// Checks claims made by docs/examples/containers/views/span.md.
#include <array>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <version>

#include "support/feature.hpp"

#include <gtest/gtest.h>

// "Declaration": Extent defaults to dynamic_extent; a fixed extent isn't stored.
TEST(Span, DeclarationExtent) {
    static_assert(std::is_same_v<std::span<int>, std::span<int, std::dynamic_extent>>);
    static_assert(std::span<int>::extent == std::dynamic_extent);
    static_assert(std::span<int, 4>::extent == 4);
    // Fixed extent: just a pointer. Dynamic extent: pointer + size.
    static_assert(sizeof(std::span<int, 4>) == sizeof(int*));
    static_assert(sizeof(std::span<int>) == sizeof(int*) + sizeof(std::size_t));
}

// "Mental model": a span is pointer + length; copying it doesn't copy elements.
TEST(Span, MentalModelViewsStorage) {
    std::vector<int> v{10, 20, 30, 40, 50};
    std::span<int> s = v;
    EXPECT_EQ(s.data(), v.data());
    EXPECT_EQ(s.size(), 5u);
    std::span<int> copy = s;
    EXPECT_EQ(copy.data(), v.data());
}

// "Why it exists": one sum(span<const double>) works for C arrays, std::array, vector.
TEST(Span, SumAcceptsAnyContiguousSource) {
    auto sum = [](std::span<const double> xs) {
        double total = 0;
        for (double x : xs) total += x;
        return total;
    };
    double c[]{1, 2, 3};
    std::array<double, 3> a{1, 2, 3};
    std::vector<double> v{1, 2, 3};
    EXPECT_EQ(sum(c), 6.0);
    EXPECT_EQ(sum(a), 6.0);
    EXPECT_EQ(sum(v), 6.0);
    EXPECT_EQ(sum(std::span<const double>(v).first(2)), 3.0);
}

// "Constructing from various sources": sizes, and ptr + count takes the first n.
TEST(Span, ConstructingFromSources) {
    int c_arr[4]{1, 2, 3, 4};
    std::array<int, 4> std_arr{1, 2, 3, 4};
    std::vector<int> vec{1, 2, 3, 4};

    std::span<int> s1{c_arr};
    std::span<int> s2{std_arr};
    std::span<int> s3{vec};
    std::span<int> s4{vec.data(), 2};
    EXPECT_EQ(s1.size(), 4u);
    EXPECT_EQ(s2.size(), 4u);
    EXPECT_EQ(s3.size(), 4u);
    EXPECT_EQ(s4.size(), 2u);
    EXPECT_EQ(s4.data(), vec.data());

    // "deduces size 4": CTAD from a C array gives a fixed extent.
    std::span d{c_arr};
    static_assert(decltype(d)::extent == 4);
}

// "Subviews (no copying)": first/last/subspan view the same storage; writes go through.
TEST(Span, Subviews) {
    std::vector<int> vec{1, 2, 3, 4};
    std::span<int> s = vec;
    auto head = s.first(2);
    auto tail = s.last(2);
    auto mid = s.subspan(1, 2);
    EXPECT_EQ(std::vector<int>(head.begin(), head.end()), (std::vector<int>{1, 2}));
    EXPECT_EQ(std::vector<int>(tail.begin(), tail.end()), (std::vector<int>{3, 4}));
    EXPECT_EQ(std::vector<int>(mid.begin(), mid.end()), (std::vector<int>{2, 3}));
    EXPECT_EQ(mid.data(), vec.data() + 1);

    s[0] = 99;
    EXPECT_EQ(vec[0], 99);
    EXPECT_EQ(head[0], 99);
}

// "const view = read-only window": span<const int> elements can't be assigned.
TEST(Span, ConstViewIsReadOnly) {
    static_assert(std::is_same_v<std::span<const int>::reference, const int&>);
    static_assert(!std::is_assignable_v<std::span<const int>::reference, int>);
    static_assert(std::is_convertible_v<std::span<int>, std::span<const int>>);
}

// "As bytes (C++20)": as_bytes gives a span<const std::byte> over the same memory.
TEST(Span, AsBytes) {
    std::vector<int> vec{1, 2, 3, 4};
    std::span<const std::byte> raw = std::as_bytes(std::span{vec});
    EXPECT_EQ(raw.size(), vec.size() * sizeof(int));
    EXPECT_EQ(static_cast<const void*>(raw.data()), static_cast<const void*>(vec.data()));
}

// "Gotchas": at() is bounds-checked (C++26) and throws std::out_of_range.
TEST(Span, AtThrowsOutOfRange) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_span, 202311L);
#if defined(__cpp_lib_span) && __cpp_lib_span >= 202311L
    std::vector<int> vec{1, 2, 3};
    std::span<int> s = vec;
    EXPECT_EQ(s.at(2), 3);
    EXPECT_THROW((void)s.at(3), std::out_of_range);
#endif
}

// "Gotchas": fixed->dynamic converts implicitly; dynamic->fixed only explicitly.
TEST(Span, FixedAndDynamicConversions) {
    static_assert(!std::is_same_v<std::span<int, 4>, std::span<int>>);
    static_assert(std::is_convertible_v<std::span<int, 4>, std::span<int>>);
    static_assert(!std::is_convertible_v<std::span<int>, std::span<int, 4>>);
    static_assert(std::is_constructible_v<std::span<int, 4>, std::span<int>>);
}
