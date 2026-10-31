// Checks claims made by docs/examples/containers/sequence/list.md.
#include <iterator>
#include <list>

#include <gtest/gtest.h>

template <class C>
concept HasSubscript = requires(C c) { c[0]; };
template <class C>
concept HasAt = requires(C c) { c.at(0); };

// "Examples": insert before an iterator, erase at it, without invalidating the rest.
TEST(List, InsertEraseAtIterator) {
    std::list<int> a{1, 2, 3};
    a.push_front(0);
    a.push_back(4);
    EXPECT_EQ(a, (std::list<int>{0, 1, 2, 3, 4}));

    auto it = std::next(a.begin(), 2);
    auto last = std::prev(a.end());
    const int* addr = &*last;
    a.insert(it, 99);
    EXPECT_EQ(a, (std::list<int>{0, 1, 99, 2, 3, 4}));
    EXPECT_EQ(*it, 2);  // insertion invalidates nothing
    a.erase(it);
    EXPECT_EQ(a, (std::list<int>{0, 1, 99, 3, 4}));
    EXPECT_EQ(&*last, addr);  // erasure only invalidates the erased element
    EXPECT_EQ(*last, 4);
}

// "Member operations that algorithms can't do as well": the commented results.
TEST(List, MemberOperations) {
    std::list<int> x{1, 2, 3}, y{4, 5, 6};
    x.splice(x.end(), y);
    EXPECT_EQ(x, (std::list<int>{1, 2, 3, 4, 5, 6}));
    EXPECT_TRUE(y.empty());
    x.sort();
    std::list<int> z{0, 7};
    x.merge(z);
    EXPECT_EQ(x, (std::list<int>{0, 1, 2, 3, 4, 5, 6, 7}));
    EXPECT_TRUE(z.empty());
    x.remove(2);
    EXPECT_EQ(x, (std::list<int>{0, 1, 3, 4, 5, 6, 7}));
    x.remove_if([](int n) { return n % 2 == 0; });
    EXPECT_EQ(x, (std::list<int>{1, 3, 5, 7}));
    std::list<int> d{1, 1, 2, 1};
    d.unique();
    EXPECT_EQ(d, (std::list<int>{1, 2, 1}));  // consecutive duplicates only
    x.reverse();
    EXPECT_EQ(x, (std::list<int>{7, 5, 3, 1}));
}

// "Iterator & reference invalidation": spliced elements keep their iterators, now in the other list.
TEST(List, SpliceKeepsIterators) {
    std::list<int> x{1, 2}, y{3, 4};
    auto it = y.begin();
    x.splice(x.end(), y);
    EXPECT_EQ(*it, 3);
    EXPECT_EQ(it, std::next(x.begin(), 2));
}

// "C++20 uniform erase"
TEST(List, UniformErase) {
    std::list<int> x{1, 2, 3, 4, 5, 6};
    EXPECT_EQ(std::erase(x, 3), 1u);
    EXPECT_EQ(std::erase_if(x, [](int n) { return n > 4; }), 2u);
    EXPECT_EQ(x, (std::list<int>{1, 2, 4}));
}

// "Gotchas": no operator[] or at(); iterators aren't random access (so no std::sort).
TEST(List, NoRandomAccess) {
    static_assert(!HasSubscript<std::list<int>>);
    static_assert(!HasAt<std::list<int>>);
    static_assert(std::bidirectional_iterator<std::list<int>::iterator>);
    static_assert(!std::random_access_iterator<std::list<int>::iterator>);
}
