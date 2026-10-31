// Checks claims made by docs/examples/containers/adaptors/priority_queue.md.
#include <algorithm>
#include <deque>
#include <functional>
#include <queue>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {
template <class T>
concept HasBegin = requires(T& t) { t.begin(); };
}  // namespace

// "Declaration": defaults are std::vector<T> and std::less.
TEST(PriorityQueue, Defaults) {
    using PQ = std::priority_queue<int>;
    static_assert(std::is_same_v<PQ::container_type, std::vector<int>>);
    static_assert(std::is_same_v<PQ::value_compare, std::less<int>>);
}

// "Declaration": std::deque also qualifies as the container.
TEST(PriorityQueue, DequeQualifies) {
    std::priority_queue<int, std::deque<int>> pq;
    pq.push(1);
    pq.push(5);
    pq.push(3);
    EXPECT_EQ(pq.top(), 5);
}

// "Interface" / "Gotchas": top() is a const reference; pop() returns void.
TEST(PriorityQueue, TopIsConstPopIsVoid) {
    using PQ = std::priority_queue<int>;
    static_assert(std::is_same_v<decltype(std::declval<PQ&>().top()), const int&>);
    static_assert(std::is_void_v<decltype(std::declval<PQ&>().pop())>);
    static_assert(!HasBegin<PQ>);
}

// "Default max-heap": top() is the largest.
TEST(PriorityQueue, DefaultMaxHeap) {
    std::priority_queue<int> pq;
    pq.push(3);
    pq.push(9);
    pq.push(5);
    EXPECT_EQ(pq.top(), 9);
    pq.pop();
    EXPECT_EQ(pq.top(), 5);
}

// "Min-heap with std::greater": top() is the smallest.
TEST(PriorityQueue, MinHeapWithGreater) {
    std::priority_queue<int, std::vector<int>, std::greater<int>> minpq;
    minpq.push(3);
    minpq.push(9);
    minpq.push(5);
    EXPECT_EQ(minpq.top(), 3);
}

// "Custom priority (e.g. by a field)": highest priority field first.
TEST(PriorityQueue, CustomComparator) {
    struct Task {
        int priority;
        std::string name;
    };
    auto cmp = [](const Task& a, const Task& b) { return a.priority < b.priority; };
    std::priority_queue<Task, std::vector<Task>, decltype(cmp)> jobs(cmp);
    jobs.push({5, "build"});
    jobs.push({9, "deploy"});
    EXPECT_EQ(jobs.top().priority, 9);
    EXPECT_EQ(jobs.top().name, "deploy");
}

// "Build from existing data in O(n)" / "Not sorted, just heap-ordered":
// the range constructor heapifies; repeated pop() yields descending order.
TEST(PriorityQueue, BuildFromRangeAndPopOrder) {
    std::vector<int> data{4, 1, 7, 3};
    std::priority_queue<int> pq(data.begin(), data.end());
    EXPECT_EQ(pq.size(), 4u);
    EXPECT_EQ(pq.top(), 7);

    std::vector<int> popped;
    while (!pq.empty()) {
        popped.push_back(pq.top());
        pq.pop();
    }
    EXPECT_EQ(popped, (std::vector<int>{7, 4, 3, 1}));
}

// "Mental model": the underlying container is a heap, which need not be sorted.
TEST(PriorityQueue, UnderlyingIsHeapNotSorted) {
    struct Peek : std::priority_queue<int> {
        const std::vector<int>& container() const { return c; }
    };
    Peek pq;
    for (int x : {3, 9, 7, 5, 8}) pq.push(x);
    const auto& c = pq.container();
    EXPECT_TRUE(std::is_heap(c.begin(), c.end()));
    EXPECT_EQ(c.front(), 9);
    EXPECT_FALSE(std::is_sorted(c.begin(), c.end(), std::greater<int>{}));
}
