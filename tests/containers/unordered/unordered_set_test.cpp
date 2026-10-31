// Checks claims made by docs/examples/containers/unordered/unordered_set.md.
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

// "Declaration": default template arguments.
TEST(UnorderedSet, DeclarationDefaults) {
    using S = std::unordered_set<std::string>;
    static_assert(std::is_same_v<S::key_type, S::value_type>);
    static_assert(std::is_same_v<S::hasher, std::hash<std::string>>);
    static_assert(std::is_same_v<S::key_equal, std::equal_to<std::string>>);
    static_assert(std::is_same_v<S::allocator_type, std::allocator<std::string>>);
}

// "Examples": insert of an existing key reports inserted == false; contains/count/erase.
TEST(UnorderedSet, Basics) {
    std::unordered_set<std::string> seen{"a", "b", "c"};
    auto [it, inserted] = seen.insert("b");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(*it, "b");
    EXPECT_TRUE(seen.contains("a"));
    EXPECT_EQ(seen.count("z"), 0u);
    EXPECT_EQ(seen.count("a"), 1u);
    EXPECT_EQ(seen.erase("b"), 1u);
    EXPECT_EQ(seen, (std::unordered_set<std::string>{"a", "c"}));
}

// "Dedup a vector in O(n) average".
TEST(UnorderedSet, DedupVector) {
    std::vector<int> v{1, 2, 2, 3, 3, 3};
    std::unordered_set<int> uniq(v.begin(), v.end());
    EXPECT_EQ(uniq, (std::unordered_set<int>{1, 2, 3}));
}

// "Reserve for known size" / "Gotchas": no rehash while filling up to the reserved size,
// so iterators stay valid.
TEST(UnorderedSet, ReserveAvoidsRehash) {
    std::unordered_set<int> s;
    s.reserve(1'000'000);
    const auto buckets = s.bucket_count();
    s.insert(-1);
    auto it = s.find(-1);
    for (int i = 0; i < 999'999; ++i) s.insert(i);
    EXPECT_EQ(s.bucket_count(), buckets);
    EXPECT_EQ(it, s.find(-1));
}

// "Custom element type".
struct Id {
    int v;
    bool operator==(const Id&) const = default;
};
template <>
struct std::hash<Id> {
    std::size_t operator()(const Id& i) const noexcept { return std::hash<int>{}(i.v); }
};

TEST(UnorderedSet, CustomElementType) {
    std::unordered_set<Id> ids{{1}, {2}, {1}};
    EXPECT_EQ(ids.size(), 2u);
    EXPECT_TRUE(ids.contains(Id{2}));
}

// "C++20 uniform erase".
TEST(UnorderedSet, EraseIf) {
    std::unordered_set<std::string> seen{"a", "", "c"};
    EXPECT_EQ(std::erase_if(seen, [](const std::string& s) { return s.empty(); }), 1u);
    EXPECT_EQ(seen, (std::unordered_set<std::string>{"a", "c"}));
}

// "Gotchas": equivalence is KeyEqual (==), not ordering.
struct AllEqual {
    bool operator()(int, int) const { return true; }
};
struct ZeroHash {
    std::size_t operator()(int) const { return 0; }
};

TEST(UnorderedSet, EquivalenceIsKeyEqual) {
    std::unordered_set<int, ZeroHash, AllEqual> s{1, 2, 3};
    EXPECT_EQ(s.size(), 1u);  // KeyEqual says they're all the same
}

// "Iterator & reference invalidation": references survive rehash.
TEST(UnorderedSet, ReferencesSurviveRehash) {
    std::unordered_set<int> s{0};
    const int* p = &*s.find(0);
    const auto buckets = s.bucket_count();
    for (int i = 1; i < 10'000; ++i) s.insert(i);
    ASSERT_NE(s.bucket_count(), buckets);
    EXPECT_EQ(p, &*s.find(0));
}
