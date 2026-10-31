// Checks claims made by docs/examples/containers/adaptors/flat_set.md.
#include <deque>
#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>
#include <version>

#if __has_include(<flat_set>)
#include <flat_set>
#endif

#include "support/feature.hpp"

#include <gtest/gtest.h>

namespace {
template <class T>
concept HasAllocatorType = requires { typename T::allocator_type; };
}  // namespace

// "Declaration": default parameters; no allocator on the adaptor itself.
TEST(FlatSet, DeclarationDefaults) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_set);
#ifdef __cpp_lib_flat_set
    using S = std::flat_set<int>;
    static_assert(std::is_same_v<S, std::flat_set<int, std::less<int>, std::vector<int>>>);
    static_assert(std::is_same_v<S::container_type, std::vector<int>>);
    static_assert(std::is_same_v<std::flat_multiset<int>::container_type, std::vector<int>>);
    static_assert(!HasAllocatorType<S>);
#endif
}

// "Declaration": the backing container can be a std::deque.
TEST(FlatSet, DequeBacked) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_set);
#ifdef __cpp_lib_flat_set
    std::flat_set<int, std::less<int>, std::deque<int>> s{5, 1, 3, 2};
    EXPECT_EQ(std::vector<int>(s.begin(), s.end()), (std::vector<int>{1, 2, 3, 5}));
#endif
}

// "Mental model" / "Basics": kept sorted through insert and erase; contains().
TEST(FlatSet, ExampleBasics) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_set);
#ifdef __cpp_lib_flat_set
    std::flat_set<int> s{5, 1, 3, 2};
    EXPECT_EQ(std::vector<int>(s.begin(), s.end()), (std::vector<int>{1, 2, 3, 5}));
    s.insert(4);
    EXPECT_EQ(std::vector<int>(s.begin(), s.end()), (std::vector<int>{1, 2, 3, 4, 5}));
    EXPECT_TRUE(s.contains(3));
    s.erase(2);
    EXPECT_EQ(std::vector<int>(s.begin(), s.end()), (std::vector<int>{1, 3, 4, 5}));

    // "a single sorted array": the elements are contiguous.
    EXPECT_EQ(&*std::next(s.begin()), &*s.begin() + 1);
#endif
}

// "Skip the sort when input is already ordered": sorted_unique for flat_set,
// sorted_equivalent for flat_multiset.
TEST(FlatSet, SortedTags) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_set);
#ifdef __cpp_lib_flat_set
    std::vector<int> sorted{1, 2, 3, 5};
    std::flat_set<int> s2(std::sorted_unique, sorted.begin(), sorted.end());
    EXPECT_EQ(std::vector<int>(s2.begin(), s2.end()), sorted);

    std::vector<int> runs{1, 2, 2, 3};
    std::flat_multiset<int> ms(std::sorted_equivalent, runs.begin(), runs.end());
    EXPECT_EQ(ms.count(2), 2u);
#endif
}

// "flat_multiset keeps duplicates": [1,2,3,3,3], count(3) == 3, equal_range is the run.
TEST(FlatSet, MultisetKeepsDuplicates) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_set);
#ifdef __cpp_lib_flat_set
    std::flat_multiset<int> ms{3, 1, 3, 2, 3};
    EXPECT_EQ(std::vector<int>(ms.begin(), ms.end()), (std::vector<int>{1, 2, 3, 3, 3}));
    EXPECT_EQ(ms.count(3), 3u);
    auto [lo, hi] = ms.equal_range(3);
    EXPECT_EQ(std::distance(lo, hi), 3);
    EXPECT_EQ(std::distance(ms.begin(), lo), 2);
#endif
}

// "Iterator & reference invalidation": an insert shifts the tail like a vector.
TEST(FlatSet, InsertShiftsTail) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_set);
#ifdef __cpp_lib_flat_set
    std::flat_set<int> s{2, 3};
    s.insert(1);  // lands at the front: 2 and 3 each move one slot
    EXPECT_EQ(*s.begin(), 1);
    EXPECT_EQ(*std::next(s.begin()), 2);
#endif
}
