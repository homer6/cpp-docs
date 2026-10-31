// Checks claims made by docs/examples/iterators/categories.md.
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

namespace {
// A legacy iterator over int* that reports Tag and counts how it was moved.
template <class Tag>
struct CountingIt {
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = int*;
    using reference = int&;
    using iterator_category = Tag;
    int* p;
    int* steps;  // ++ / -- calls
    int* jumps;  // += calls
    int& operator*() const { return *p; }
    CountingIt& operator++() { ++p; ++*steps; return *this; }
    CountingIt& operator--() { --p; ++*steps; return *this; }
    CountingIt& operator+=(difference_type n) { p += n; ++*jumps; return *this; }
    bool operator==(const CountingIt& o) const { return p == o.p; }
};

template <class R>
concept RangesSortable = requires(R& r) { std::ranges::sort(r); };
}  // namespace

// "The hierarchy": the table's example containers have the stated category.
TEST(IteratorCategories, HierarchyTableExamples) {
    static_assert(std::forward_iterator<std::forward_list<int>::iterator>);
    static_assert(!std::bidirectional_iterator<std::forward_list<int>::iterator>);
    static_assert(std::forward_iterator<std::unordered_set<int>::iterator>);
    static_assert(std::forward_iterator<std::unordered_map<int, int>::iterator>);
    static_assert(std::bidirectional_iterator<std::set<int>::iterator>);
    static_assert(std::bidirectional_iterator<std::map<int, int>::iterator>);
    static_assert(!std::random_access_iterator<std::set<int>::iterator>);
    static_assert(std::random_access_iterator<std::deque<int>::iterator>);
    static_assert(std::contiguous_iterator<std::array<int, 3>::iterator>);
    static_assert(std::contiguous_iterator<std::string::iterator>);
    static_assert(std::contiguous_iterator<std::span<int>::iterator>);
    static_assert(std::output_iterator<std::back_insert_iterator<std::vector<int>>, int>);
    static_assert(!std::input_iterator<std::back_insert_iterator<std::vector<int>>>);
    SUCCEED();
}

// "The hierarchy": std::sort needs random access (the constrained ranges::sort rejects list); find needs input.
TEST(IteratorCategories, WhyItMatters) {
    static_assert(RangesSortable<std::vector<int>>);
    static_assert(!RangesSortable<std::list<int>>);
    std::list<int> l{3, 1, 2};
    l.sort();
    EXPECT_EQ(l, (std::list<int>{1, 2, 3}));
    std::forward_list<int> fl{5, 6};
    EXPECT_EQ(*std::find(fl.begin(), fl.end(), 6), 6);
}

// "C++20 iterator concepts": the page's static_asserts.
TEST(IteratorCategories, Cpp20Concepts) {
    static_assert(std::input_iterator<std::istream_iterator<int>>);
    static_assert(!std::forward_iterator<std::istream_iterator<int>>);  // single-pass
    static_assert(std::output_iterator<std::ostream_iterator<int>, int>);
    static_assert(std::forward_iterator<std::forward_list<int>::iterator>);
    static_assert(std::bidirectional_iterator<std::list<int>::iterator>);
    static_assert(std::random_access_iterator<std::deque<int>::iterator>);
    static_assert(std::contiguous_iterator<std::vector<int>::iterator>);
    static_assert(!std::random_access_iterator<std::list<int>::iterator>);
    static_assert(std::sentinel_for<std::default_sentinel_t, std::counted_iterator<int*>>);
    static_assert(std::sized_sentinel_for<int*, int*>);
    SUCCEED();
}

// "The legacy tag types": each tag derives from the weaker one; output_iterator_tag stands alone.
TEST(IteratorCategories, LegacyTagTypes) {
    static_assert(std::derived_from<std::forward_iterator_tag, std::input_iterator_tag>);
    static_assert(std::derived_from<std::bidirectional_iterator_tag, std::forward_iterator_tag>);
    static_assert(std::derived_from<std::random_access_iterator_tag, std::bidirectional_iterator_tag>);
    static_assert(std::derived_from<std::contiguous_iterator_tag, std::random_access_iterator_tag>);
    static_assert(!std::derived_from<std::output_iterator_tag, std::input_iterator_tag>);
    static_assert(std::same_as<std::iterator_traits<std::list<int>::iterator>::iterator_category,
                               std::bidirectional_iterator_tag>);
    SUCCEED();
}

// "The legacy tag types": std::advance jumps once for random access, steps n times otherwise.
TEST(IteratorCategories, AdvanceDispatchesOnTag) {
    int a[10]{};
    int steps = 0, jumps = 0;
    CountingIt<std::random_access_iterator_tag> ra{a, &steps, &jumps};
    std::advance(ra, 7);
    EXPECT_EQ(ra.p, a + 7);
    EXPECT_EQ(jumps, 1);
    EXPECT_EQ(steps, 0);

    steps = jumps = 0;
    CountingIt<std::forward_iterator_tag> fw{a, &steps, &jumps};
    std::advance(fw, 7);
    EXPECT_EQ(fw.p, a + 7);
    EXPECT_EQ(steps, 7);
    EXPECT_EQ(jumps, 0);
}

// "Gotchas": contiguous guarantees &*(it + n) == &*it + n; deque is random access but not contiguous.
TEST(IteratorCategories, ContiguousVersusRandomAccess) {
    static_assert(!std::contiguous_iterator<std::deque<int>::iterator>);
    std::vector<int> v{1, 2, 3, 4};
    auto it = v.begin();
    EXPECT_EQ(&*(it + 3), &*it + 3);
}
