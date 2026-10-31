// Checks claims made by docs/examples/containers/sequence/forward_list.md.
#include <forward_list>
#include <iterator>
#include <list>

#include <gtest/gtest.h>

template <class C>
concept HasSize = requires(C c) { c.size(); };
template <class C>
concept HasBack = requires(C c) { c.back(); };
template <class C>
concept HasPushBack = requires(C c) { c.push_back(1); };

// "Mental model": no stored size or tail, so the object is smaller than a list.
TEST(ForwardList, SmallerThanList) {
    static_assert(sizeof(std::forward_list<int>) < sizeof(std::list<int>));
    static_assert(std::forward_iterator<std::forward_list<int>::iterator>);
    static_assert(!std::bidirectional_iterator<std::forward_list<int>::iterator>);
}

// "The _after interface and before_begin": the commented results.
TEST(ForwardList, AfterInterface) {
    std::forward_list<int> fl{1, 2, 3};
    fl.push_front(0);
    EXPECT_EQ(fl, (std::forward_list<int>{0, 1, 2, 3}));
    auto one = std::next(fl.begin());
    fl.insert_after(fl.begin(), 99);
    EXPECT_EQ(fl, (std::forward_list<int>{0, 99, 1, 2, 3}));
    fl.insert_after(fl.before_begin(), -1);
    EXPECT_EQ(fl, (std::forward_list<int>{-1, 0, 99, 1, 2, 3}));
    auto it = fl.begin();
    fl.erase_after(it);  // removes the element after -1
    EXPECT_EQ(fl, (std::forward_list<int>{-1, 99, 1, 2, 3}));
    EXPECT_EQ(*one, 1);  // "Iterator & reference invalidation": still valid
}

// "No size(): count manually"
TEST(ForwardList, NoSize) {
    static_assert(!HasSize<std::forward_list<int>>);
    std::forward_list<int> fl{1, 2, 3};
    EXPECT_EQ(std::distance(fl.begin(), fl.end()), 3);
    EXPECT_FALSE(fl.empty());
}

// "Member operations (like list)": the commented results.
TEST(ForwardList, MemberOperations) {
    std::forward_list<int> fl{3, 99, 1, 1, 2};
    fl.sort();
    EXPECT_EQ(fl, (std::forward_list<int>{1, 1, 2, 3, 99}));
    fl.unique();
    EXPECT_EQ(fl, (std::forward_list<int>{1, 2, 3, 99}));
    std::forward_list<int> sorted{0, 4};
    fl.merge(sorted);
    EXPECT_EQ(fl, (std::forward_list<int>{0, 1, 2, 3, 4, 99}));
    EXPECT_TRUE(sorted.empty());
    fl.remove(99);
    EXPECT_EQ(fl, (std::forward_list<int>{0, 1, 2, 3, 4}));
    fl.reverse();
    EXPECT_EQ(fl, (std::forward_list<int>{4, 3, 2, 1, 0}));
    std::forward_list<int> other{7, 8};
    auto seven = other.begin();
    fl.splice_after(fl.before_begin(), other);
    EXPECT_EQ(fl, (std::forward_list<int>{7, 8, 4, 3, 2, 1, 0}));
    EXPECT_TRUE(other.empty());
    EXPECT_EQ(seven, fl.begin());  // splicing doesn't invalidate the moved nodes
}

// "C++20 uniform erase"
TEST(ForwardList, UniformErase) {
    std::forward_list<int> fl{-1, 2, 3, 2};
    EXPECT_EQ(std::erase(fl, 2), 2u);
    EXPECT_EQ(std::erase_if(fl, [](int x) { return x < 0; }), 1u);
    EXPECT_EQ(fl, (std::forward_list<int>{3}));
}

// "Gotchas": no back() / push_back().
TEST(ForwardList, NoBack) {
    static_assert(!HasBack<std::forward_list<int>>);
    static_assert(!HasPushBack<std::forward_list<int>>);
}
