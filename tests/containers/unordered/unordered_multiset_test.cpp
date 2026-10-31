// Checks claims made by docs/examples/containers/unordered/unordered_multiset.md.
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <unordered_set>

#include <gtest/gtest.h>

// "Declaration": default template arguments.
TEST(UnorderedMultiset, DeclarationDefaults) {
    using S = std::unordered_multiset<int>;
    static_assert(std::is_same_v<S::hasher, std::hash<int>>);
    static_assert(std::is_same_v<S::key_equal, std::equal_to<int>>);
    static_assert(std::is_same_v<S::allocator_type, std::allocator<int>>);
}

// "How it differs from unordered_set": insert always succeeds, returns a plain iterator.
TEST(UnorderedMultiset, InsertReturnsPlainIterator) {
    std::unordered_multiset<int> s{1};
    static_assert(std::is_same_v<decltype(s.insert(1)), std::unordered_multiset<int>::iterator>);
    EXPECT_EQ(*s.insert(1), 1);
    EXPECT_EQ(s.count(1), 2u);
}

// "Examples": count, erase one by iterator, erase(key) returns the count; equal_range.
TEST(UnorderedMultiset, Example) {
    std::unordered_multiset<int> bag{1, 1, 2, 3, 3, 3};
    bag.insert(3);
    EXPECT_EQ(bag.count(3), 4u);

    auto [lo, hi] = bag.equal_range(3);
    EXPECT_EQ(std::distance(lo, hi), 4);
    for (auto it = lo; it != hi; ++it) EXPECT_EQ(*it, 3);

    auto it = bag.find(3);
    ASSERT_NE(it, bag.end());
    bag.erase(it);
    EXPECT_EQ(bag.count(3), 3u);

    EXPECT_EQ(bag.erase(3), 3u);
    EXPECT_EQ(bag, (std::unordered_multiset<int>{1, 1, 2}));
}

// "Iterator & reference invalidation": references survive rehash.
TEST(UnorderedMultiset, ReferencesSurviveRehash) {
    std::unordered_multiset<int> s;
    const int* p = &*s.insert(0);
    const auto buckets = s.bucket_count();
    for (int i = 1; i < 10'000; ++i) s.insert(i);
    ASSERT_NE(s.bucket_count(), buckets);
    EXPECT_EQ(p, &*s.find(0));
}
