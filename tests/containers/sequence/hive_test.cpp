// Checks claims made by docs/examples/containers/sequence/hive.md.
#include <version>

#include <algorithm>
#include <concepts>
#include <iterator>
#include <memory_resource>
#include <type_traits>
#include <vector>
#ifdef __cpp_lib_hive
#include <hive>
#endif

#include <gtest/gtest.h>

template <class C>
concept HasSubscript = requires(C c) { c[0]; };
template <class C>
concept HasData = requires(C c) { c.data(); };

#include "support/feature.hpp"

// "Declaration": pmr alias; bidirectional (not random-access) iterators; no ==/!=.
TEST(Hive, DeclarationAndRequirements) {
    CPPDOCS_REQUIRE(__cpp_lib_hive);
#ifdef __cpp_lib_hive
    static_assert(std::is_same_v<std::pmr::hive<int>,
                                 std::hive<int, std::pmr::polymorphic_allocator<int>>>);
    using It = std::hive<int>::iterator;
    static_assert(std::bidirectional_iterator<It>);
    static_assert(!std::random_access_iterator<It>);
    static_assert(!std::equality_comparable<std::hive<int>>);
    static_assert(!HasSubscript<std::hive<int>>);
    static_assert(!HasData<std::hive<int>>);
#endif
}

// "Examples" + "Iterator & reference invalidation": insert returns an iterator that stays
// valid across later inserts; erase removes only that element.
TEST(Hive, StableAcrossInsertAndErase) {
    CPPDOCS_REQUIRE(__cpp_lib_hive);
#ifdef __cpp_lib_hive
    std::hive<int> h;
    auto it = h.insert(42);
    const int* addr = &*it;
    auto seven = h.insert(7);
    for (int i = 0; i < 10'000; ++i) h.insert(i);
    auto ninety_nine = h.emplace(99);
    EXPECT_EQ(*it, 42);
    EXPECT_EQ(&*it, addr);

    h.erase(it);
    EXPECT_EQ(*seven, 7);
    EXPECT_EQ(*ninety_nine, 99);
    EXPECT_EQ(h.size(), 10'002u);
    EXPECT_EQ(std::count(h.begin(), h.end(), 42), 0);
#endif
}

// "Pre-reserve and tune block sizes": reserve gives capacity without elements;
// hive_limits sets the block capacity limits.
TEST(Hive, ReserveAndLimits) {
    CPPDOCS_REQUIRE(__cpp_lib_hive);
#ifdef __cpp_lib_hive
    std::hive<int> g;
    g.reserve(10'000);
    EXPECT_GE(g.capacity(), 10'000u);
    EXPECT_TRUE(g.empty());

    std::hive<int> limited{std::hive_limits{64, 1024}};
    EXPECT_EQ(limited.block_capacity_limits().min, 64u);
    EXPECT_EQ(limited.block_capacity_limits().max, 1024u);
#endif
}

// "Member operations": sort orders the elements; unique then removes consecutive duplicates.
TEST(Hive, SortUnique) {
    CPPDOCS_REQUIRE(__cpp_lib_hive);
#ifdef __cpp_lib_hive
    std::hive<int> h{3, 1, 3, 2};
    h.sort();
    EXPECT_EQ(std::vector<int>(h.begin(), h.end()), (std::vector<int>{1, 2, 3, 3}));
    h.unique();
    EXPECT_EQ(std::vector<int>(h.begin(), h.end()), (std::vector<int>{1, 2, 3}));
#endif
}

// "C++20-style uniform erase"
TEST(Hive, UniformErase) {
    CPPDOCS_REQUIRE(__cpp_lib_hive);
#ifdef __cpp_lib_hive
    std::hive<int> h{7, -1, 3, 7};
    EXPECT_EQ(std::erase(h, 7), 2u);
    EXPECT_EQ(std::erase_if(h, [](int x) { return x < 0; }), 1u);
    EXPECT_EQ(std::vector<int>(h.begin(), h.end()), (std::vector<int>{3}));
#endif
}
