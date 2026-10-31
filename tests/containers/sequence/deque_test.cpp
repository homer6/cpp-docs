// Checks claims made by docs/examples/containers/sequence/deque.md.
#include <cstddef>
#include <deque>
#include <queue>
#include <stack>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

template <class C>
concept HasData = requires(C c) { c.data(); };

// "Mental model": no data() on deque.
TEST(Deque, NoData) {
    static_assert(!HasData<std::deque<int>>);
}

// "Examples": the values after each push/pop.
TEST(Deque, BothEnds) {
    std::deque<int> d{1, 2, 3};
    d.push_front(0);
    EXPECT_EQ(d, (std::deque<int>{0, 1, 2, 3}));
    d.push_back(4);
    EXPECT_EQ(d, (std::deque<int>{0, 1, 2, 3, 4}));
    d.pop_front();
    EXPECT_EQ(d, (std::deque<int>{1, 2, 3, 4}));
    EXPECT_EQ(d[2], 3);
    EXPECT_EQ(d.at(2), 3);
    EXPECT_THROW((void)d.at(4), std::out_of_range);
    EXPECT_EQ(d.front(), 1);
    EXPECT_EQ(d.back(), 4);
}

// "Using it as a sliding window": keeps the last `cap` values.
TEST(Deque, SlidingWindow) {
    std::deque<int> window;
    auto push = [&](int x, std::size_t cap) {
        window.push_back(x);
        if (window.size() > cap) window.pop_front();
    };
    for (int i = 1; i <= 5; ++i) push(i, 3);
    EXPECT_EQ(window, (std::deque<int>{3, 4, 5}));
}

// "Iterator & reference invalidation": references to existing elements survive
// push_front/push_back; pop_front leaves references to the other elements valid.
TEST(Deque, ReferencesSurviveEndInsertion) {
    std::deque<int> d{1, 2, 3};
    int& mid = d[1];
    for (int i = 0; i < 10'000; ++i) {
        d.push_back(i);
        d.push_front(i);
    }
    EXPECT_EQ(&mid, &d[10'001]);
    EXPECT_EQ(mid, 2);
    d.pop_front();
    d.pop_back();
    EXPECT_EQ(mid, 2);
    EXPECT_EQ(&mid, &d[10'000]);
}

// "When to use it": std::queue and std::stack default to deque.
TEST(Deque, AdaptorsDefaultToDeque) {
    static_assert(std::is_same_v<std::queue<int>::container_type, std::deque<int>>);
    static_assert(std::is_same_v<std::stack<int>::container_type, std::deque<int>>);
}

// "Gotchas": shrink_to_fit exists (a non-binding request).
TEST(Deque, ShrinkToFit) {
    std::deque<int> d(1000);
    d.clear();
    d.shrink_to_fit();
    EXPECT_TRUE(d.empty());
}
