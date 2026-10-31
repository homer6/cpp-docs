// Checks claims made by docs/examples/containers/associative/multimap.md.
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using MM = std::multimap<std::string, std::string>;

// "Declaration": default template arguments.
TEST(Multimap, DeclarationDefaults) {
    static_assert(std::is_same_v<MM::value_type, std::pair<const std::string, std::string>>);
    static_assert(std::is_same_v<MM::key_compare, std::less<std::string>>);
    static_assert(std::is_same_v<MM::allocator_type,
                                 std::allocator<std::pair<const std::string, std::string>>>);
}

// "How it differs from map" / "Gotchas": no operator[], at, try_emplace, insert_or_assign.
template <class M>
concept HasSubscript = requires(M m, typename M::key_type k) { m[k]; };
template <class M>
concept HasAt = requires(M m, typename M::key_type k) { m.at(k); };
template <class M>
concept HasTryEmplace = requires(M m, typename M::key_type k) { m.try_emplace(k); };
template <class M>
concept HasInsertOrAssign =
    requires(M m, typename M::key_type k, typename M::mapped_type v) { m.insert_or_assign(k, v); };

TEST(Multimap, NoUniqueKeyConveniences) {
    static_assert(!HasSubscript<MM> && !HasAt<MM> && !HasTryEmplace<MM> && !HasInsertOrAssign<MM>);
    using M = std::map<std::string, std::string>;
    static_assert(HasSubscript<M> && HasAt<M> && HasTryEmplace<M> && HasInsertOrAssign<M>);
}

// "How it differs from map": insert/emplace always succeed and return a plain iterator.
TEST(Multimap, InsertReturnsPlainIterator) {
    MM m;
    static_assert(std::is_same_v<decltype(m.emplace("k", "v")), MM::iterator>);
    static_assert(std::is_same_v<decltype(m.insert({"k", "v"})), MM::iterator>);
    m.emplace("k", "v");
    m.insert({"k", "v"});
    EXPECT_EQ(m.size(), 2u);
}

// "Examples": count, equal_range, erase one iterator vs erase(key).
TEST(Multimap, Example) {
    MM m;
    m.emplace("fruit", "apple");
    m.emplace("fruit", "cherry");
    m.emplace("veg", "carrot");

    EXPECT_EQ(m.count("fruit"), 2u);

    std::vector<std::string> values;
    auto [lo, hi] = m.equal_range("fruit");
    for (auto it = lo; it != hi; ++it) {
        EXPECT_EQ(it->first, "fruit");
        values.push_back(it->second);
    }
    EXPECT_EQ(values, (std::vector<std::string>{"apple", "cherry"}));

    auto it = m.find("fruit");
    ASSERT_NE(it, m.end());
    m.erase(it);
    EXPECT_EQ(m.count("fruit"), 1u);

    EXPECT_EQ(m.erase("fruit"), 1u);  // removes all remaining, returns the count
    EXPECT_EQ(m.count("fruit"), 0u);
    EXPECT_EQ(m.size(), 1u);
}

// "How it differs from map": erase(key) removes all entries and returns the count.
TEST(Multimap, EraseKeyRemovesWholeGroup) {
    std::multimap<int, int> m{{1, 1}, {1, 2}, {1, 3}, {2, 4}};
    EXPECT_EQ(m.erase(1), 3u);
    EXPECT_EQ(m, (std::multimap<int, int>{{2, 4}}));
}

// "Iterator & reference invalidation": equivalent keys keep insertion order; nodes are stable.
TEST(Multimap, InsertionOrderAndStability) {
    std::multimap<int, int> m;
    m.emplace(2, 0);
    int& ref = m.find(2)->second;
    auto keep = m.begin();
    for (int i = 1; i <= 5; ++i) m.emplace(1, i);
    for (int i = 0; i < 100; ++i) m.emplace(3, i);
    auto [lo, hi] = m.equal_range(1);
    std::vector<int> order;
    for (auto it = lo; it != hi; ++it) order.push_back(it->second);
    EXPECT_EQ(order, (std::vector<int>{1, 2, 3, 4, 5}));
    EXPECT_EQ(&ref, &m.find(2)->second);
    EXPECT_EQ(keep, m.find(2));
}
