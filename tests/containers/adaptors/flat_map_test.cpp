// Checks claims made by docs/examples/containers/adaptors/flat_map.md.
#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <version>

#if __has_include(<flat_map>)
#include <flat_map>
#endif

#include "support/feature.hpp"

#include <gtest/gtest.h>

namespace {
template <class M>
concept HasSubscript = requires(M& m, const typename M::key_type& k) { m[k]; };
template <class M>
concept HasAt = requires(M& m, const typename M::key_type& k) { m.at(k); };
}  // namespace

// "Declaration": default parameters, and value_type is pair<key, mapped>.
TEST(FlatMap, DeclarationDefaults) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_map);
#ifdef __cpp_lib_flat_map
    using M = std::flat_map<std::string, int>;
    static_assert(std::is_same_v<
                  M, std::flat_map<std::string, int, std::less<std::string>,
                                   std::vector<std::string>, std::vector<int>>>);
    static_assert(std::is_same_v<M::key_container_type, std::vector<std::string>>);
    static_assert(std::is_same_v<M::mapped_container_type, std::vector<int>>);
    static_assert(std::is_same_v<M::value_type, std::pair<std::string, int>>);
    static_assert(std::is_same_v<std::flat_multimap<std::string, int>::key_container_type,
                                 std::vector<std::string>>);
#endif
}

// "Declaration" / "Gotchas": *it is a pair of references (a proxy), not a reference to a stored pair.
TEST(FlatMap, DereferenceYieldsProxy) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_map);
#ifdef __cpp_lib_flat_map
    using M = std::flat_map<std::string, int>;
    static_assert(std::is_same_v<M::reference, std::pair<const std::string&, int&>>);
    static_assert(!std::is_reference_v<decltype(*std::declval<M&>().begin())>);

    M m{{"a", 1}};
    auto [k, v] = *m.begin();
    v = 10;  // writes through into the values array
    EXPECT_EQ(&k, &m.keys()[0]);
    EXPECT_EQ(m.values()[0], 10);
#endif
}

// "Examples": operator[] inserts keeping keys sorted; at/contains; ascending iteration.
TEST(FlatMap, ExampleBasics) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_map);
#ifdef __cpp_lib_flat_map
    std::flat_map<std::string, int> m{{"banana", 7}, {"apple", 3}};
    m["cherry"] = 2;
    EXPECT_EQ(m.at("apple"), 3);
    EXPECT_TRUE(m.contains("banana"));
    EXPECT_THROW((void)m.at("durian"), std::out_of_range);

    std::vector<std::string> ks;
    std::vector<int> vs;
    for (const auto& [k, v] : m) {
        ks.push_back(k);
        vs.push_back(v);
    }
    EXPECT_EQ(ks, (std::vector<std::string>{"apple", "banana", "cherry"}));
    EXPECT_EQ(vs, (std::vector<int>{3, 7, 2}));
#endif
}

// "Mental model" / "Access the underlying arrays directly": keys() and values() are
// the two parallel sorted containers, returned by const reference.
TEST(FlatMap, KeysAndValuesAreParallelArrays) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_map);
#ifdef __cpp_lib_flat_map
    std::flat_map<std::string, int> m{{"banana", 7}, {"apple", 3}};
    static_assert(std::is_same_v<decltype(m.keys()), const std::vector<std::string>&>);
    static_assert(std::is_same_v<decltype(m.values()), const std::vector<int>&>);
    EXPECT_EQ(m.keys(), (std::vector<std::string>{"apple", "banana"}));
    EXPECT_EQ(m.values(), (std::vector<int>{3, 7}));
#endif
}

// "Build fast from sorted input": the sorted_unique constructor adopts the two containers.
TEST(FlatMap, SortedUniqueConstructor) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_map);
#ifdef __cpp_lib_flat_map
    std::vector<std::string> keys{"apple", "banana", "cherry"};
    std::vector<int> vals{3, 7, 2};
    std::flat_map<std::string, int> fm(std::sorted_unique, std::move(keys), std::move(vals));
    EXPECT_EQ(fm.size(), 3u);
    EXPECT_EQ(fm.at("banana"), 7);
    EXPECT_EQ(fm.keys(), (std::vector<std::string>{"apple", "banana", "cherry"}));
#endif
}

// "flat_multimap allows repeats; no operator[]" / "Gotchas": both kept, no operator[] or at.
TEST(FlatMap, MultimapKeepsRepeatsNoSubscript) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_map);
#ifdef __cpp_lib_flat_map
    std::flat_multimap<std::string, int> mm;
    mm.emplace("x", 1);
    mm.emplace("x", 2);
    auto [lo, hi] = mm.equal_range("x");
    EXPECT_EQ(std::distance(lo, hi), 2);
    EXPECT_EQ(mm.values(), (std::vector<int>{1, 2}));

    using MM = std::flat_multimap<std::string, int>;
    static_assert(!HasSubscript<MM>);
    static_assert(!HasAt<MM>);
    static_assert(HasSubscript<std::flat_map<std::string, int>>);
    static_assert(HasAt<std::flat_map<std::string, int>>);
#endif
}

// "Gotchas": operator[] inserts a default value for a missing key.
TEST(FlatMap, SubscriptInsertsDefault) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_map);
#ifdef __cpp_lib_flat_map
    std::flat_map<std::string, int> m{{"a", 1}};
    EXPECT_EQ(m["missing"], 0);
    EXPECT_EQ(m.size(), 2u);
    EXPECT_TRUE(m.contains("missing"));
#endif
}

// "Iterator & reference invalidation": an insert shifts elements in both arrays.
TEST(FlatMap, InsertShiftsBothArrays) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_map);
#ifdef __cpp_lib_flat_map
    std::flat_map<std::string, int> m{{"banana", 7}, {"cherry", 2}};
    EXPECT_EQ(m.keys()[0], "banana");
    m.emplace("apple", 3);  // lands at the front: everything after it moves one slot
    EXPECT_EQ(m.keys()[0], "apple");
    EXPECT_EQ(m.values()[0], 3);
    EXPECT_EQ(m.keys()[1], "banana");
    EXPECT_EQ(m.values()[1], 7);
#endif
}
