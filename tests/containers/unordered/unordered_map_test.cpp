// Checks claims made by docs/examples/containers/unordered/unordered_map.md.
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <gtest/gtest.h>

// "Declaration": default template arguments.
TEST(UnorderedMap, DeclarationDefaults) {
    using M = std::unordered_map<std::string, int>;
    static_assert(std::is_same_v<M::value_type, std::pair<const std::string, int>>);
    static_assert(std::is_same_v<M::hasher, std::hash<std::string>>);
    static_assert(std::is_same_v<M::key_equal, std::equal_to<std::string>>);
    static_assert(std::is_same_v<M::allocator_type,
                                 std::allocator<std::pair<const std::string, int>>>);
}

// "Basics — same insertion API as map".
TEST(UnorderedMap, Basics) {
    std::unordered_map<std::string, int> m;
    m["alice"] = 30;
    EXPECT_TRUE(m.try_emplace("bob", 25).second);
    EXPECT_FALSE(m.try_emplace("bob", 99).second);  // skip if present
    EXPECT_EQ(m.at("bob"), 25);
    EXPECT_FALSE(m.insert_or_assign("alice", 31).second);  // overwrite
    EXPECT_EQ(m.at("alice"), 31);

    EXPECT_TRUE(m.contains("bob"));
    EXPECT_THROW((void)m.at("zed"), std::out_of_range);

    int sum = 0;
    for (const auto& [k, v] : m) sum += v;
    EXPECT_EQ(sum, 56);
}

// "Gotchas": operator[] inserts a default value for missing keys; find/contains don't.
TEST(UnorderedMap, SubscriptInserts) {
    std::unordered_map<std::string, int> m;
    EXPECT_FALSE(m.contains("x"));
    EXPECT_EQ(m.find("x"), m.end());
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m["x"], 0);
    EXPECT_EQ(m.size(), 1u);
}

// "Reserve to avoid rehashing": with max_load_factor set first, reserve(n) sizes the table
// so filling to n elements never rehashes.
TEST(UnorderedMap, ReserveAvoidsRehash) {
    std::unordered_map<int, int> m;
    m.max_load_factor(0.5f);
    m.reserve(10'000);
    EXPECT_EQ(m.max_load_factor(), 0.5f);
    EXPECT_GE(m.bucket_count() * m.max_load_factor(), 10'000.0f);
    const auto buckets = m.bucket_count();
    for (int i = 0; i < 10'000; ++i) m.emplace(i, i);
    EXPECT_EQ(m.bucket_count(), buckets);  // no rehash while filling
    EXPECT_LE(m.load_factor(), 0.5f);
}

// "Custom key type needs a hash".
struct Point {
    int x, y;
    bool operator==(const Point&) const = default;
};

template <>
struct std::hash<Point> {
    std::size_t operator()(const Point& p) const noexcept {
        return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
    }
};

TEST(UnorderedMap, CustomKeyHash) {
    std::unordered_map<Point, std::string> grid;
    grid[{1, 2}] = "a";
    grid[{2, 1}] = "b";
    EXPECT_EQ(grid.size(), 2u);
    EXPECT_EQ(grid.at({1, 2}), "a");
    EXPECT_EQ(grid.at({2, 1}), "b");
}

// "C++20 uniform erase".
TEST(UnorderedMap, EraseIf) {
    std::unordered_map<std::string, int> m{{"alice", 30}, {"bob", 0}};
    EXPECT_EQ(std::erase_if(m, [](const auto& kv) { return kv.second == 0; }), 1u);
    EXPECT_EQ(m, (std::unordered_map<std::string, int>{{"alice", 30}}));
}

// "Iterator & reference invalidation": references survive rehash.
TEST(UnorderedMap, ReferencesSurviveRehash) {
    std::unordered_map<int, int> m{{0, 42}};
    int& ref = m.at(0);
    const auto buckets = m.bucket_count();
    for (int i = 1; i < 10'000; ++i) m.emplace(i, i);
    ASSERT_NE(m.bucket_count(), buckets);  // a rehash happened
    EXPECT_EQ(&ref, &m.at(0));
    EXPECT_EQ(ref, 42);
}

// "Gotchas": a bad hash still works (all keys in one bucket) — just slowly.
struct ConstantHash {
    std::size_t operator()(int) const noexcept { return 0; }
};

TEST(UnorderedMap, BadHashCollides) {
    std::unordered_map<int, int, ConstantHash> m;
    for (int i = 0; i < 100; ++i) m.emplace(i, i);
    EXPECT_EQ(m.bucket_size(m.bucket(0)), 100u);
    EXPECT_EQ(m.at(57), 57);
}
