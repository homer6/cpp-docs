// Checks claims made by docs/examples/utilities/pair.md.
#include <compare>
#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

// "Mental model": map iteration yields pair<const Key, T>.
TEST(Pair, MapValueTypeIsPairOfConstKey) {
    static_assert(std::is_same_v<std::map<std::string, int>::value_type,
                                 std::pair<const std::string, int>>);
}

// "Examples": first/second, make_pair and CTAD both deduce pair<int, double>, structured bindings.
TEST(Pair, Basics) {
    std::pair<int, std::string> p{1, "one"};
    EXPECT_EQ(p.first, 1);
    EXPECT_EQ(p.second, "one");

    auto q = std::make_pair(2, 3.5);
    std::pair r{2, 3.5};
    static_assert(std::is_same_v<decltype(q), std::pair<int, double>>);
    static_assert(std::is_same_v<decltype(r), std::pair<int, double>>);

    auto [n, name] = p;
    EXPECT_EQ(n, 1);
    EXPECT_EQ(name, "one");
}

// "As function return / map element": insert returns pair<iterator, bool>; iteration yields pairs.
TEST(Pair, MapInsertReturnsPair) {
    std::map<std::string, int> m;
    auto [it, inserted] = m.insert({"k", 1});
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->first, "k");
    auto [it2, inserted2] = m.insert({"k", 2});
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it2->second, 1);
    for (const auto& [key, value] : m) {
        EXPECT_EQ(key, "k");
        EXPECT_EQ(value, 1);
    }
}

// "Comparison": lexicographic (first, then second), with operator<=>.
TEST(Pair, LexicographicComparison) {
    EXPECT_TRUE((std::pair{1, 2} < std::pair{1, 3}));
    EXPECT_TRUE((std::pair{1, 9} < std::pair{2, 0}));
    EXPECT_TRUE((std::pair{1, 2} <=> std::pair{1, 3}) < 0);
    static_assert(std::three_way_comparable<std::pair<int, int>>);
}

// "Gotchas": make_pair unwraps reference_wrapper to a reference; CTAD keeps the wrapper.
TEST(Pair, MakePairUnwrapsReferenceWrapper) {
    int x = 1;
    auto a = std::make_pair(std::ref(x), 0);
    std::pair b{std::ref(x), 0};
    static_assert(std::is_same_v<decltype(a), std::pair<int&, int>>);
    static_assert(std::is_same_v<decltype(b), std::pair<std::reference_wrapper<int>, int>>);
    a.first = 5;
    EXPECT_EQ(x, 5);
}

// "Gotchas": the map key is const, so .first can't be reassigned through an iterator.
TEST(Pair, MapKeyIsConst) {
    std::map<std::string, int> m{{"k", 1}};
    static_assert(std::is_const_v<std::remove_reference_t<decltype(m.begin()->first)>>);
    static_assert(!std::is_assignable_v<decltype((m.begin()->first)), std::string>);
}
