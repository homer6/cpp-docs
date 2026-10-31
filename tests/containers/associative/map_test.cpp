// Checks claims made by docs/examples/containers/associative/map.md.
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

// "Declaration": default template arguments and value_type.
TEST(Map, DeclarationDefaults) {
    using M = std::map<std::string, int>;
    static_assert(std::is_same_v<M::value_type, std::pair<const std::string, int>>);
    static_assert(std::is_same_v<M::key_compare, std::less<std::string>>);
    static_assert(std::is_same_v<M::allocator_type,
                                 std::allocator<std::pair<const std::string, int>>>);
}

// "Mental model": the key is const through an iterator; the value is mutable.
TEST(Map, KeyIsConstValueIsNot) {
    using It = std::map<std::string, int>::iterator;
    static_assert(std::is_const_v<std::remove_reference_t<decltype(std::declval<It>()->first)>>);
    static_assert(!std::is_const_v<std::remove_reference_t<decltype(std::declval<It>()->second)>>);
}

// "The many ways to insert (and why they differ)": what each call does to the value.
TEST(Map, ManyWaysToInsert) {
    std::map<std::string, int> m;
    m["alice"] = 30;
    EXPECT_EQ(m.at("alice"), 30);

    EXPECT_TRUE(m.insert({"bob", 25}).second);
    EXPECT_FALSE(m.insert({"bob", 99}).second);  // no-op if "bob" exists
    EXPECT_EQ(m.at("bob"), 25);

    EXPECT_TRUE(m.emplace("carol", 40).second);
    EXPECT_FALSE(m.emplace("carol", 99).second);  // no-op if exists
    EXPECT_EQ(m.at("carol"), 40);

    auto [it, inserted] = m.try_emplace("alice", 99);  // does NOT touch the value
    EXPECT_FALSE(inserted);
    EXPECT_EQ(it->second, 30);

    auto [it2, inserted2] = m.insert_or_assign("alice", 31);  // overwrite if present
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it2->second, 31);
    EXPECT_TRUE(m.insert_or_assign("dave", 1).second);  // insert if absent
    EXPECT_EQ(m.size(), 4u);
}

// "Lookup": at throws, operator[] inserts a default, contains/find don't insert.
TEST(Map, Lookup) {
    std::map<std::string, int> m{{"alice", 30}, {"bob", 25}};
    EXPECT_EQ(m.at("alice"), 30);
    EXPECT_THROW((void)m.at("zed"), std::out_of_range);
    EXPECT_EQ(m.size(), 2u);

    EXPECT_TRUE(m.contains("alice"));
    EXPECT_FALSE(m.contains("zed"));
    EXPECT_EQ(m.find("zed"), m.end());
    EXPECT_EQ(m.size(), 2u);  // no insertion

    auto it = m.find("bob");
    ASSERT_NE(it, m.end());
    EXPECT_EQ(it->first, "bob");
    EXPECT_EQ(it->second, 25);

    EXPECT_EQ(m["zed"], 0);  // inserts a value-initialized int
    EXPECT_EQ(m.size(), 3u);
}

// "Gotchas": operator[] needs a default-constructible T and isn't available on a const map.
template <class M>
concept HasSubscript = requires(M m) { m[typename M::key_type{}]; };

TEST(Map, SubscriptRequirements) {
    static_assert(HasSubscript<std::map<int, int>>);
    static_assert(!HasSubscript<const std::map<int, int>>);
    // The default-constructible requirement isn't SFINAE-visible (operator[] is
    // unconstrained; it fails on instantiation), so it can't be asserted here.
}

// "Iterating (sorted by key) with structured bindings": ascending key order, values mutable.
TEST(Map, IterationSortedAndMutable) {
    std::map<std::string, int> m{{"bob", 25}, {"alice", 30}};
    std::vector<std::string> keys;
    for (const auto& [key, value] : m) keys.push_back(key);
    EXPECT_EQ(keys, (std::vector<std::string>{"alice", "bob"}));

    for (auto& [key, value] : m) value += 1;
    EXPECT_EQ(m.at("alice"), 31);
    EXPECT_EQ(m.at("bob"), 26);
}

// "Counting words — the idiomatic snippet": operator[] default-inits to 0.
TEST(Map, CountingWords) {
    std::vector<std::string> words{"a", "b", "a"};
    std::map<std::string, int> freq;
    for (const auto& w : words) ++freq[w];
    EXPECT_EQ(freq, (std::map<std::string, int>{{"a", 2}, {"b", 1}}));
}

// "C++20 uniform erase": erase_if removes matching entries.
TEST(Map, EraseIf) {
    std::map<std::string, int> m{{"alice", 30}, {"bob", 0}};
    EXPECT_EQ(std::erase_if(m, [](const auto& kv) { return kv.second == 0; }), 1u);
    EXPECT_EQ(m, (std::map<std::string, int>{{"alice", 30}}));
}

// "Iterator & reference invalidation": inserts never invalidate; erase only the erased one.
TEST(Map, NodeStability) {
    std::map<int, int> m{{1, 10}, {2, 20}};
    int& ref = m.at(1);
    auto it = m.find(2);
    for (int i = 3; i < 1000; ++i) m.emplace(i, i);
    m.erase(500);
    EXPECT_EQ(&ref, &m.at(1));
    EXPECT_EQ(it, m.find(2));
    EXPECT_EQ(it->second, 20);
}

// "Gotchas": change a key via extract (C++17).
TEST(Map, ExtractChangesKey) {
    std::map<std::string, int> m{{"alice", 30}};
    auto node = m.extract("alice");
    node.key() = "alicia";
    m.insert(std::move(node));
    EXPECT_EQ(m, (std::map<std::string, int>{{"alicia", 30}}));
}
