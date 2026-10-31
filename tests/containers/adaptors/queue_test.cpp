// Checks claims made by docs/examples/containers/adaptors/queue.md.
#include <deque>
#include <list>
#include <queue>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {
template <class T>
concept HasBegin = requires(T& t) { t.begin(); };
template <class T>
concept HasPopFront = requires(T& t) { t.pop_front(); };
}  // namespace

// "Declaration": the default backing container is std::deque<T>.
TEST(Queue, DefaultContainerIsDeque) {
    static_assert(std::is_same_v<std::queue<int>::container_type, std::deque<int>>);
}

// "Declaration": list qualifies; vector does not, because it has no pop_front.
TEST(Queue, ListQualifiesVectorLacksPopFront) {
    std::queue<int, std::list<int>> lq;
    lq.push(1);
    lq.push(2);
    lq.pop();
    EXPECT_EQ(lq.front(), 2);
    static_assert(!HasPopFront<std::vector<int>>);
    static_assert(HasPopFront<std::deque<int>>);
}

// "Interface": pop() returns void; there are no iterators.
TEST(Queue, PopReturnsVoidNoIterators) {
    static_assert(std::is_void_v<decltype(std::declval<std::queue<int>&>().pop())>);
    static_assert(!HasBegin<std::queue<int>>);
}

// "Examples": FIFO — front() is the oldest, back() the newest, pop() removes the front.
TEST(Queue, ExampleFifo) {
    std::queue<int> q;
    q.push(1);
    q.push(2);
    q.emplace(3);

    EXPECT_EQ(q.front(), 1);
    EXPECT_EQ(q.back(), 3);
    q.pop();
    EXPECT_EQ(q.front(), 2);
    EXPECT_EQ(q.size(), 2u);

    std::vector<int> drained;
    while (!q.empty()) {
        drained.push_back(q.front());
        q.pop();
    }
    EXPECT_EQ(drained, (std::vector<int>{2, 3}));
}
