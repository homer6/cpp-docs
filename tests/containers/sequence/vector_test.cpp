// Checks claims made by docs/examples/containers/sequence/vector.md.
#include <algorithm>
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

// "Access": at() is bounds-checked and throws std::out_of_range.
TEST(Vector, AtThrowsOutOfRange) {
    std::vector<int> v{1, 2, 3};
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

// "Declaration": vector<bool>::operator[] returns a proxy, not bool&.
TEST(Vector, VectorBoolReturnsProxy) {
    static_assert(!std::is_same_v<std::vector<bool>::reference, bool&>);
    static_assert(std::is_same_v<std::vector<char>::reference, char&>);
}
