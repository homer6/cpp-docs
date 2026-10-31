// Checks claims made by docs/examples/containers/sequence/inplace_vector.md.
#include <version>

#include <cstddef>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#ifdef __cpp_lib_inplace_vector
#include <inplace_vector>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Declaration": capacity() and max_size() are N, constexpr and noexcept; N == 0 is allowed.
TEST(InplaceVector, CapacityIsN) {
    CPPDOCS_REQUIRE(__cpp_lib_inplace_vector);
#ifdef __cpp_lib_inplace_vector
    using V = std::inplace_vector<int, 4>;
    static_assert(V::capacity() == 4 && V::max_size() == 4);
    static_assert(noexcept(V::capacity()) && noexcept(V::max_size()));
    std::inplace_vector<int, 0> z;
    EXPECT_EQ(z.capacity(), 0u);
    EXPECT_FALSE(z.try_push_back(1));
#endif
}

// "Mental model": the elements live inside the object (no heap).
TEST(InplaceVector, StorageIsInline) {
    CPPDOCS_REQUIRE(__cpp_lib_inplace_vector);
#ifdef __cpp_lib_inplace_vector
    std::inplace_vector<int, 4> v{1, 2};
    const auto* lo = reinterpret_cast<const unsigned char*>(&v);
    const auto* p = reinterpret_cast<const unsigned char*>(v.data());
    EXPECT_GE(p, lo);
    EXPECT_LE(p + 4 * sizeof(int), lo + sizeof v);
#endif
}

// "Inserting when full — three flavors": return types and overflow behaviour
// (try_* return type depends on the __cpp_lib_inplace_vector value; see the page).
TEST(InplaceVector, ThreeInsertFlavors) {
    CPPDOCS_REQUIRE(__cpp_lib_inplace_vector);
#ifdef __cpp_lib_inplace_vector
    using V = std::inplace_vector<int, 2>;
    static_assert(std::is_same_v<decltype(std::declval<V&>().push_back(1)), int&>);
    static_assert(std::is_same_v<decltype(std::declval<V&>().emplace_back(1)), int&>);
#if __cpp_lib_inplace_vector >= 202603L  // P3981: try_* return optional<T&>
    using Try = std::optional<int&>;
#else  // earlier drafts: a raw pointer, nullptr when full
    using Try = int*;
#endif
    static_assert(std::is_same_v<decltype(std::declval<V&>().try_push_back(1)), Try>);
    static_assert(std::is_same_v<decltype(std::declval<V&>().try_emplace_back(1)), Try>);
    static_assert(std::is_same_v<decltype(std::declval<V&>().unchecked_push_back(1)), int&>);
    static_assert(std::is_same_v<decltype(std::declval<V&>().unchecked_emplace_back(1)), int&>);

    V small;
    small.push_back(1);
    auto p = small.try_push_back(2);
    ASSERT_TRUE(p);
    EXPECT_EQ(&*p, &small.back());
    EXPECT_FALSE(small.try_push_back(3));  // full: empty result, no throw
    EXPECT_FALSE(small.try_emplace_back(3));
    EXPECT_EQ(small.size(), 2u);
    EXPECT_THROW(small.push_back(3), std::bad_alloc);
    EXPECT_THROW(small.emplace_back(3), std::bad_alloc);
    EXPECT_EQ(small.size(), small.capacity());
#endif
}

// "Examples": size after two push_backs; capacity stays 4.
TEST(InplaceVector, Example) {
    CPPDOCS_REQUIRE(__cpp_lib_inplace_vector);
#ifdef __cpp_lib_inplace_vector
    std::inplace_vector<int, 4> v;
    v.push_back(1);
    v.push_back(2);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v.capacity(), 4u);
#endif
}

// "Iterator & reference invalidation": push_back never moves existing elements;
// moving the whole container moves the storage with it.
TEST(InplaceVector, Invalidation) {
    CPPDOCS_REQUIRE(__cpp_lib_inplace_vector);
#ifdef __cpp_lib_inplace_vector
    std::inplace_vector<int, 4> v{1};
    const int* first = &v[0];
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    EXPECT_EQ(&v[0], first);
    auto w = std::move(v);
    EXPECT_NE(w.data(), first);
    EXPECT_EQ(w[0], 1);
#endif
}

// "constexpr use"
TEST(InplaceVector, Constexpr) {
    CPPDOCS_REQUIRE(__cpp_lib_inplace_vector);
#ifdef __cpp_lib_inplace_vector
    constexpr auto build = [] {
        std::inplace_vector<int, 8> r;
        for (int i = 0; i < 5; ++i) r.push_back(i * i);
        return r;
    }();
    static_assert(build.size() == 5 && build[4] == 16);
#endif
}

// "Gotchas": reserve(n) is a no-op for n <= N and throws std::bad_alloc for n > N;
// shrink_to_fit is a no-op.
TEST(InplaceVector, ReserveAndShrink) {
    CPPDOCS_REQUIRE(__cpp_lib_inplace_vector);
#ifdef __cpp_lib_inplace_vector
    std::inplace_vector<int, 4> v{1, 2};
    v.reserve(4);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v.capacity(), 4u);
    EXPECT_THROW(v.reserve(5), std::bad_alloc);
    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), 4u);
    EXPECT_EQ(v.size(), 2u);
#endif
}
