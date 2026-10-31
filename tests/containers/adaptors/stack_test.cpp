// Checks claims made by docs/examples/containers/adaptors/stack.md.
#include <deque>
#include <list>
#include <stack>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {
template <class T>
concept HasBegin = requires(T& t) { t.begin(); };
template <class T>
concept HasIteratorType = requires { typename T::iterator; };
}  // namespace

// "Declaration": the default backing container is std::deque<T>.
TEST(Stack, DefaultContainerIsDeque) {
    static_assert(std::is_same_v<std::stack<int>::container_type, std::deque<int>>);
    static_assert(std::is_same_v<std::stack<int>, std::stack<int, std::deque<int>>>);
}

// "Declaration": deque, vector and list all qualify as the backing store.
TEST(Stack, VectorAndListQualify) {
    std::stack<int, std::vector<int>> vs;
    std::stack<int, std::list<int>> ls;
    for (int i = 1; i <= 3; ++i) {
        vs.push(i);
        ls.push(i);
    }
    EXPECT_EQ(vs.top(), 3);
    EXPECT_EQ(ls.top(), 3);
}

// "Interface": pop() returns void; there are no iterators.
TEST(Stack, PopReturnsVoidNoIterators) {
    static_assert(std::is_void_v<decltype(std::declval<std::stack<int>&>().pop())>);
    static_assert(!HasBegin<std::stack<int>>);
    static_assert(!HasIteratorType<std::stack<int>>);
}

// "Interface": top() is read/write.
TEST(Stack, TopIsWritable) {
    static_assert(std::is_same_v<decltype(std::declval<std::stack<int>&>().top()), int&>);
    std::stack<int> s;
    s.push(1);
    s.top() = 42;
    EXPECT_EQ(s.top(), 42);
}

// "Examples": LIFO order; top() does not remove; pop() removes the top.
TEST(Stack, ExampleLifo) {
    std::stack<int> s;
    s.push(1);
    s.push(2);
    s.emplace(3);

    EXPECT_EQ(s.top(), 3);
    EXPECT_EQ(s.size(), 3u);  // top() didn't remove
    s.pop();
    EXPECT_EQ(s.size(), 2u);

    std::vector<int> drained;
    while (!s.empty()) {
        drained.push_back(s.top());
        s.pop();
    }
    EXPECT_EQ(drained, (std::vector<int>{2, 1}));
}
