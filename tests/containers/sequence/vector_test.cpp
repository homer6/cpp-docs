// Checks claims made by docs/examples/containers/sequence/vector.md.
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

// "Construction (modern)": what each constructor form produces.
TEST(Vector, ConstructionForms) {
    EXPECT_EQ(std::vector<int>(5), (std::vector<int>{0, 0, 0, 0, 0}));
    EXPECT_EQ(std::vector<int>(5, 7), (std::vector<int>{7, 7, 7, 7, 7}));
    std::vector e{1, 2, 3};
    static_assert(std::is_same_v<decltype(e), std::vector<int>>);  // CTAD

    std::vector<int> a;
    EXPECT_TRUE(a.empty());
    std::vector<int> d{1, 2, 3};
    EXPECT_EQ(d, (std::vector<int>{1, 2, 3}));
    std::vector<int> f(d.begin(), d.end());
    EXPECT_EQ(f, d);
}

// "Mental model": growth is geometric, so n push_backs reallocate only O(log n) times.
TEST(Vector, GeometricGrowth) {
    std::vector<int> v;
    std::size_t reallocations = 0;
    auto cap = v.capacity();
    for (int i = 0; i < 100'000; ++i) {
        v.push_back(i);
        if (v.capacity() != cap) {
            EXPECT_GT(v.capacity(), cap);
            cap = v.capacity();
            ++reallocations;
        }
    }
    EXPECT_LT(reallocations, 64u);  // linear growth would need ~100'000
}

// "Growth and the reserve idiom": refs stay valid while filling below the reservation;
// emplace_back appends.
TEST(Vector, ReserveKeepsReferencesValid) {
    std::vector<int> v;
    v.reserve(1000);
    v.push_back(0);
    const int& first = v.front();
    for (int i = 1; i < 1000; ++i) v.push_back(i);
    EXPECT_EQ(&first, &v.front());
    v.emplace_back(42);
    EXPECT_EQ(v.size(), 1001u);
    EXPECT_EQ(v.back(), 42);
}

// "Iterator & reference invalidation": without reallocation, insertion leaves elements
// before the insertion point in place; erase leaves elements before the erased one.
TEST(Vector, InvalidationWithoutReallocation) {
    std::vector<int> v{1, 2, 3, 4};
    v.reserve(10);
    const int* p0 = &v[0];
    const int* p1 = &v[1];
    v.insert(v.begin() + 2, 99);
    EXPECT_EQ(&v[0], p0);
    EXPECT_EQ(&v[1], p1);
    EXPECT_EQ(*p1, 2);
    v.erase(v.begin() + 2);
    EXPECT_EQ(&v[1], p1);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3, 4}));
}

// "Gotchas": reserve changes capacity only; resize changes size; clear keeps capacity.
TEST(Vector, ReserveResizeClear) {
    std::vector<int> v;
    v.reserve(1000);
    EXPECT_EQ(v.size(), 0u);
    EXPECT_GE(v.capacity(), 1000u);

    // "no reallocations below 1000": data() stays put while filling.
    const int* before = v.data();
    for (int i = 0; i < 1000; ++i) v.push_back(i);
    EXPECT_EQ(v.data(), before);

    const auto cap = v.capacity();
    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.capacity(), cap);  // clear() keeps capacity

    v.resize(3);
    EXPECT_EQ(v, (std::vector<int>{0, 0, 0}));  // value-initialized
}

// "Access": front/back/data; at() is bounds-checked and throws std::out_of_range.
TEST(Vector, AtThrowsOutOfRange) {
    std::vector<int> v{1, 2, 3};
    EXPECT_EQ(v.front(), 1);
    EXPECT_EQ(v.back(), 3);
    EXPECT_EQ(v.data(), &v[0]);
    EXPECT_EQ(v.data() + 2, &v[2]);  // contiguous
    EXPECT_EQ(v.at(0), 1);
    EXPECT_THROW((void)v.at(3), std::out_of_range);
}

// "Erase-remove, and the C++20 simplification": std::erase returns the count removed.
TEST(Vector, EraseRemoveAndStdErase) {
    std::vector<int> v{7, 1, 7, 2, 3};
    v.erase(std::remove(v.begin(), v.end(), 7), v.end());
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));

    std::vector<int> w{7, 1, 7, 2, 3};
    EXPECT_EQ(std::erase(w, 7), 2u);
    EXPECT_EQ(std::erase_if(w, [](int x) { return x % 2 == 0; }), 1u);
    EXPECT_EQ(w, (std::vector<int>{1, 3}));
}

// "Iterating": auto& in a range-for mutates in place.
TEST(Vector, IterateMutateInPlace) {
    std::vector<int> v{1, 2, 3};
    for (auto& x : v) x *= 2;
    EXPECT_EQ(v, (std::vector<int>{2, 4, 6}));
}

// "Gotchas": swapping with an empty temporary actually releases the memory.
TEST(Vector, SwapIdiomReleasesMemory) {
    std::vector<int> v(1000);
    v.clear();
    EXPECT_GE(v.capacity(), 1000u);
    std::vector<int>().swap(v);
    EXPECT_EQ(v.capacity(), 0u);
}

// "Declaration": vector<bool>::operator[] returns a proxy, not bool&.
TEST(Vector, VectorBoolReturnsProxy) {
    static_assert(!std::is_same_v<std::vector<bool>::reference, bool&>);
    static_assert(std::is_same_v<std::vector<char>::reference, char&>);
}
