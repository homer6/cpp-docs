// Checks claims made by docs/examples/containers/unordered/unordered_multimap.md.
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using UMM = std::unordered_multimap<std::string, int>;

// "Declaration": default template arguments.
TEST(UnorderedMultimap, DeclarationDefaults) {
    static_assert(std::is_same_v<UMM::hasher, std::hash<std::string>>);
    static_assert(std::is_same_v<UMM::key_equal, std::equal_to<std::string>>);
    static_assert(std::is_same_v<UMM::allocator_type,
                                 std::allocator<std::pair<const std::string, int>>>);
}

// "How it differs from unordered_map" / "Gotchas": no [], at, try_emplace, insert_or_assign.
template <class M>
concept HasSubscript = requires(M m, typename M::key_type k) { m[k]; };
template <class M>
concept HasAt = requires(M m, typename M::key_type k) { m.at(k); };
template <class M>
concept HasTryEmplace = requires(M m, typename M::key_type k) { m.try_emplace(k); };
template <class M>
concept HasInsertOrAssign =
    requires(M m, typename M::key_type k, typename M::mapped_type v) { m.insert_or_assign(k, v); };

TEST(UnorderedMultimap, NoUniqueKeyConveniences) {
    static_assert(!HasSubscript<UMM> && !HasAt<UMM> && !HasTryEmplace<UMM> &&
                  !HasInsertOrAssign<UMM>);
    using M = std::unordered_map<std::string, int>;
    static_assert(HasSubscript<M> && HasAt<M> && HasTryEmplace<M> && HasInsertOrAssign<M>);
}

// "How it differs from unordered_map": insert/emplace always succeed, returning a plain iterator.
TEST(UnorderedMultimap, InsertReturnsPlainIterator) {
    UMM m;
    static_assert(std::is_same_v<decltype(m.emplace("a", 1)), UMM::iterator>);
    static_assert(std::is_same_v<decltype(m.insert({"a", 1})), UMM::iterator>);
    m.emplace("a", 1);
    m.insert({"a", 1});
    EXPECT_EQ(m.size(), 2u);
}

// "Examples" + "Mental model": equal keys form a contiguous run returned by equal_range.
TEST(UnorderedMultimap, Example) {
    UMM m;
    m.emplace("a", 1);
    m.emplace("a", 2);
    m.emplace("b", 3);

    EXPECT_EQ(m.count("a"), 2u);

    auto [lo, hi] = m.equal_range("a");
    EXPECT_EQ(std::distance(lo, hi), 2);
    std::vector<int> vals;
    for (auto it = lo; it != hi; ++it) vals.push_back(it->second);
    std::ranges::sort(vals);  // order among equal keys is unspecified
    EXPECT_EQ(vals, (std::vector<int>{1, 2}));

    auto it = m.find("a");
    ASSERT_NE(it, m.end());
    m.erase(it);
    EXPECT_EQ(m.count("a"), 1u);
    EXPECT_EQ(m.erase("a"), 1u);
    EXPECT_EQ(m.count("a"), 0u);
    EXPECT_EQ(m.size(), 1u);
}

// "Mental model": entries with equal keys are adjacent in iteration order.
TEST(UnorderedMultimap, EqualKeysGrouped) {
    std::unordered_multimap<int, int> m;
    for (int round = 0; round < 3; ++round)
        for (int k = 0; k < 50; ++k) m.emplace(k, round);
    std::vector<int> keys;
    for (const auto& [k, v] : m) keys.push_back(k);
    // Each key appears as one run: the number of key changes is (distinct keys - 1).
    int changes = 0;
    for (std::size_t i = 1; i < keys.size(); ++i) changes += keys[i] != keys[i - 1];
    EXPECT_EQ(changes, 49);
}

// "Iterator & reference invalidation": references stable across rehash.
TEST(UnorderedMultimap, ReferencesSurviveRehash) {
    std::unordered_multimap<int, int> m;
    int& ref = m.emplace(0, 42)->second;
    const auto buckets = m.bucket_count();
    for (int i = 1; i < 10'000; ++i) m.emplace(i, i);
    ASSERT_NE(m.bucket_count(), buckets);
    EXPECT_EQ(&ref, &m.find(0)->second);
}
