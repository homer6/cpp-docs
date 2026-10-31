// Checks claims made by docs/examples/containers/associative/multiset.md.
#include <functional>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

// "Declaration": default template arguments.
TEST(Multiset, DeclarationDefaults) {
    using S = std::multiset<int>;
    static_assert(std::is_same_v<S::key_compare, std::less<int>>);
    static_assert(std::is_same_v<S::allocator_type, std::allocator<int>>);
}

// "Mental model": sorted, duplicates kept and adjacent.
TEST(Multiset, SortedDuplicatesKept) {
    std::multiset<int> ms{3, 5, 1, 3, 3};
    EXPECT_EQ(std::vector<int>(ms.begin(), ms.end()), (std::vector<int>{1, 3, 3, 3, 5}));
}

// "How it differs from set": insert always succeeds and returns a plain iterator.
TEST(Multiset, InsertReturnsPlainIterator) {
    std::multiset<int> ms{1};
    static_assert(std::is_same_v<decltype(ms.insert(1)), std::multiset<int>::iterator>);
    static_assert(std::is_same_v<decltype(std::set<int>{}.insert(1)),
                                 std::pair<std::set<int>::iterator, bool>>);
    auto it = ms.insert(1);
    EXPECT_EQ(*it, 1);
    EXPECT_EQ(ms.count(1), 2u);
}

// "Examples": count, contains, erase one by iterator, erase(key) returns the count, equal_range.
TEST(Multiset, Example) {
    std::multiset<int> ms{1, 3, 3, 3, 5};
    ms.insert(3);
    EXPECT_EQ(ms.count(3), 4u);
    EXPECT_TRUE(ms.contains(3));

    auto it = ms.find(3);
    ASSERT_NE(it, ms.end());
    ms.erase(it);
    EXPECT_EQ(ms.count(3), 3u);  // a single node removed

    EXPECT_EQ(ms.erase(3), 3u);  // all remaining 3s, returns the count
    EXPECT_FALSE(ms.contains(3));

    auto [lo, hi] = ms.equal_range(3);
    EXPECT_EQ(lo, hi);
    EXPECT_EQ(std::vector<int>(ms.begin(), ms.end()), (std::vector<int>{1, 5}));
}

// "Iterator & reference invalidation": equivalent elements keep insertion order (C++11).
struct ByFirst {
    bool operator()(const std::pair<int, int>& a, const std::pair<int, int>& b) const {
        return a.first < b.first;
    }
};

TEST(Multiset, EquivalentElementsKeepInsertionOrder) {
    std::multiset<std::pair<int, int>, ByFirst> ms;
    ms.insert({2, 0});
    const auto* p = &*ms.begin();
    for (int i = 1; i <= 5; ++i) ms.insert({1, i});
    std::vector<int> order;
    auto [lo, hi] = ms.equal_range({1, 0});
    for (auto it = lo; it != hi; ++it) order.push_back(it->second);
    EXPECT_EQ(order, (std::vector<int>{1, 2, 3, 4, 5}));
    EXPECT_EQ(p, &*ms.find({2, 0}));  // insertion didn't move the existing node
}
