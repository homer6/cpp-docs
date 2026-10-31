// Checks claims made by docs/examples/containers/README.md.
#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <version>

#if __has_include(<flat_map>)
#include <flat_map>
#endif
#if __has_include(<flat_set>)
#include <flat_set>
#endif
#if __has_include(<mdspan>)
#include <mdspan>
#endif
#if __has_include(<inplace_vector>)
#include <inplace_vector>
#endif
#if __has_include(<hive>)
#include <hive>
#endif

#include "support/feature.hpp"

#include <gtest/gtest.h>

// "Sequence containers": array is fixed-size with inline storage; the rest keep the order you give.
TEST(ContainersReadme, SequenceContainers) {
    static_assert(std::tuple_size_v<std::array<int, 4>> == 4);
    static_assert(sizeof(std::array<int, 4>) == 4 * sizeof(int));  // no heap pointer
    const std::vector<int> order{3, 1, 2};
    EXPECT_EQ(std::deque<int>(order.begin(), order.end()), (std::deque<int>{3, 1, 2}));
    EXPECT_EQ(std::list<int>(order.begin(), order.end()), (std::list<int>{3, 1, 2}));
    EXPECT_EQ(std::forward_list<int>(order.begin(), order.end()),
              (std::forward_list<int>{3, 1, 2}));
}

// "Sequence containers": inplace_vector (C++26) is a fixed-capacity vector in <inplace_vector>.
TEST(ContainersReadme, InplaceVector) {
    CPPDOCS_REQUIRE(__cpp_lib_inplace_vector);
#ifdef __cpp_lib_inplace_vector
    std::inplace_vector<int, 4> v{1, 2};
    EXPECT_EQ(v.capacity(), 4u);
#endif
}

// "Sequence containers": hive (C++26) lives in <hive>.
TEST(ContainersReadme, Hive) {
    CPPDOCS_REQUIRE(__cpp_lib_hive);
#ifdef __cpp_lib_hive
    std::hive<int> h{1, 2, 3};
    EXPECT_EQ(h.size(), 3u);
#endif
}

// "Associative containers (ordered)": sorted by key; set/map unique, multi* keep duplicates.
TEST(ContainersReadme, OrderedAssociative) {
    std::set<int> s{3, 1, 3, 2};
    EXPECT_EQ(std::vector<int>(s.begin(), s.end()), (std::vector<int>{1, 2, 3}));
    std::multiset<int> ms{3, 1, 3, 2};
    EXPECT_EQ(std::vector<int>(ms.begin(), ms.end()), (std::vector<int>{1, 2, 3, 3}));
    std::map<int, char> m{{2, 'b'}, {1, 'a'}, {2, 'x'}};
    EXPECT_EQ(m.size(), 2u);
    EXPECT_EQ(m.begin()->first, 1);
    std::multimap<int, char> mm{{2, 'b'}, {1, 'a'}, {2, 'x'}};
    EXPECT_EQ(mm.size(), 3u);
}

// "Unordered associative containers (hash-based)": unique vs duplicate keys.
TEST(ContainersReadme, UnorderedAssociative) {
    EXPECT_EQ((std::unordered_set<int>{1, 1, 2}.size()), 2u);
    EXPECT_EQ((std::unordered_multiset<int>{1, 1, 2}.size()), 3u);
    EXPECT_EQ((std::unordered_map<int, int>{{1, 1}, {1, 2}}.size()), 1u);
    EXPECT_EQ((std::unordered_multimap<int, int>{{1, 1}, {1, 2}}.size()), 2u);
}

// "Container adaptors": stack is LIFO, queue FIFO, priority_queue largest-first by default.
TEST(ContainersReadme, ClassicAdaptors) {
    std::stack<int> s;
    std::queue<int> q;
    std::priority_queue<int> pq;
    for (int x : {2, 3, 1}) {
        s.push(x);
        q.push(x);
        pq.push(x);
    }
    EXPECT_EQ(s.top(), 1);
    EXPECT_EQ(q.front(), 2);
    EXPECT_EQ(pq.top(), 3);
}

// "Container adaptors": flat_set is a sorted set over one contiguous array (<flat_set>, C++23).
TEST(ContainersReadme, FlatSet) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_set);
#ifdef __cpp_lib_flat_set
    std::flat_set<int> s{3, 1, 2};
    EXPECT_EQ(std::vector<int>(s.begin(), s.end()), (std::vector<int>{1, 2, 3}));
    static_assert(std::is_same_v<std::flat_set<int>::container_type, std::vector<int>>);
    static_assert(std::is_same_v<std::flat_multiset<int>::container_type, std::vector<int>>);
#endif
}

// "Container adaptors": flat_map is a sorted map over two arrays (<flat_map>, C++23).
TEST(ContainersReadme, FlatMap) {
    CPPDOCS_REQUIRE(__cpp_lib_flat_map);
#ifdef __cpp_lib_flat_map
    std::flat_map<int, char> m{{2, 'b'}, {1, 'a'}};
    EXPECT_EQ(m.keys(), (std::vector<int>{1, 2}));
    EXPECT_EQ(m.values(), (std::vector<char>{'a', 'b'}));
    std::flat_multimap<int, char> mm{{1, 'a'}, {1, 'b'}};
    EXPECT_EQ(mm.size(), 2u);
#endif
}

// "Views (related — not containers)": span is a non-owning contiguous 1-D view.
TEST(ContainersReadme, Span) {
    std::vector<int> v{1, 2, 3};
    std::span<int> s = v;
    EXPECT_EQ(s.data(), v.data());  // views v's storage, no copy
    EXPECT_EQ(s.size(), 3u);
}

// "Views (related — not containers)": mdspan is a multidimensional view (<mdspan>, C++23).
TEST(ContainersReadme, Mdspan) {
    CPPDOCS_REQUIRE(__cpp_lib_mdspan);
#ifdef __cpp_lib_mdspan
    int buf[6]{0, 1, 2, 3, 4, 5};
    std::mdspan m{buf, 2, 3};
    EXPECT_EQ(m.rank(), 2u);
    EXPECT_EQ((m[1, 0]), 3);
    EXPECT_EQ((&m[0, 0]), buf);
#endif
}

// "Which container should I use?": list keeps element addresses stable across inserts.
TEST(ContainersReadme, CheatSheetListStability) {
    std::list<int> l{1, 2, 3};
    const int* second = &*std::next(l.begin());
    for (int i = 0; i < 100; ++i) l.push_front(i);
    EXPECT_EQ(*second, 2);
}
