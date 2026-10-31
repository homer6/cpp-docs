// Checks claims made by docs/examples/containers/associative/set.md.
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

// "Declaration": default template arguments; the value is the key.
TEST(Set, DeclarationDefaults) {
    using S = std::set<int>;
    static_assert(std::is_same_v<S::key_type, S::value_type>);
    static_assert(std::is_same_v<S::key_compare, std::less<int>>);
    static_assert(std::is_same_v<S::allocator_type, std::allocator<int>>);
}

// "Basics": duplicates dropped, sorted, insert/erase/contains/count/find.
TEST(Set, Basics) {
    std::set<int> s{3, 1, 4, 1, 5};
    EXPECT_EQ(s, (std::set<int>{1, 3, 4, 5}));
    EXPECT_EQ(std::vector<int>(s.begin(), s.end()), (std::vector<int>{1, 3, 4, 5}));
    s.insert(2);
    EXPECT_EQ(std::vector<int>(s.begin(), s.end()), (std::vector<int>{1, 2, 3, 4, 5}));
    s.erase(3);
    EXPECT_EQ(std::vector<int>(s.begin(), s.end()), (std::vector<int>{1, 2, 4, 5}));
    EXPECT_TRUE(s.contains(4));
    EXPECT_EQ(s.count(4), 1u);
    EXPECT_EQ(s.count(3), 0u);  // count is 0/1 for set
    EXPECT_NE(s.find(4), s.end());
}

// "insert returns whether it happened": inserted is false for an existing key; it points at it.
TEST(Set, InsertReturnsPair) {
    std::set<int> s{1, 2, 3};
    auto [it, inserted] = s.insert(2);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(*it, 2);
    EXPECT_TRUE(s.insert(4).second);
}

// "Ordered queries": lower_bound / upper_bound.
TEST(Set, OrderedQueries) {
    std::set<int> t{10, 20, 30, 40};
    auto lo = t.lower_bound(20);
    auto hi = t.upper_bound(30);
    EXPECT_EQ(*lo, 20);
    EXPECT_EQ(*hi, 40);
    EXPECT_EQ(std::vector<int>(lo, hi), (std::vector<int>{20, 30}));
}

// "Custom ordering & transparent comparators": greater<> sorts descending; less<> allows
// heterogeneous lookup by string_view.
template <class S>
concept FindsByStringView = requires(S s, std::string_view sv) { s.find(sv); };

TEST(Set, CustomOrderingAndTransparentLookup) {
    std::set<std::string, std::greater<>> desc{"a", "c", "b"};
    EXPECT_EQ(std::vector<std::string>(desc.begin(), desc.end()),
              (std::vector<std::string>{"c", "b", "a"}));

    std::set<std::string, std::less<>> names{"alice", "bob"};
    auto it = names.find(std::string_view{"bob"});
    ASSERT_NE(it, names.end());
    EXPECT_EQ(*it, "bob");

    static_assert(requires { typename std::less<>::is_transparent; });
    static_assert(FindsByStringView<std::set<std::string, std::less<>>>);
    static_assert(!FindsByStringView<std::set<std::string>>);  // std::less<std::string>
}

// "Node extraction & merge (C++17)": merge leaves only duplicates in the source; extract
// lets you change a key.
TEST(Set, MergeAndExtract) {
    std::set<int> a{1, 2, 3}, b{3, 4, 5};
    a.merge(b);
    EXPECT_EQ(b, (std::set<int>{3}));
    EXPECT_EQ(a, (std::set<int>{1, 2, 3, 4, 5}));

    auto node = a.extract(2);
    node.value() = 20;
    a.insert(std::move(node));
    EXPECT_EQ(a, (std::set<int>{1, 3, 4, 5, 20}));
}

// "C++20 uniform erase": erase_if removes evens.
TEST(Set, EraseIf) {
    std::set<int> s{1, 2, 3, 4, 5};
    EXPECT_EQ(std::erase_if(s, [](int x) { return x % 2 == 0; }), 2u);
    EXPECT_EQ(s, (std::set<int>{1, 3, 5}));
}

// "Gotchas": *it is const — keys can't be modified through an iterator.
TEST(Set, IteratorDerefIsConst) {
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*std::declval<std::set<int>::iterator>())>>);
}

// "Mental model" / "Gotchas": uniqueness is by equivalence under the comparator, not ==.
struct CaseInsensitiveLess {
    bool operator()(const std::string& a, const std::string& b) const {
        auto lower = [](char c) { return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c; };
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                            [&](char x, char y) { return lower(x) < lower(y); });
    }
};

TEST(Set, UniquenessIsEquivalence) {
    std::set<std::string, CaseInsensitiveLess> s{"Bob"};
    EXPECT_FALSE(s.insert("bob").second);  // "bob" != "Bob", but equivalent
    EXPECT_EQ(s.size(), 1u);
    EXPECT_EQ(*s.begin(), "Bob");
}

// "Iterator & reference invalidation": insertion never invalidates; erase only the erased one.
TEST(Set, NodeStability) {
    std::set<int> s{1, 2};
    const int* p = &*s.find(1);
    auto it = s.find(2);
    for (int i = 3; i < 1000; ++i) s.insert(i);
    s.erase(500);
    EXPECT_EQ(p, &*s.find(1));
    EXPECT_EQ(it, s.find(2));
}
