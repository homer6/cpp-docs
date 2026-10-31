// Checks claims made by docs/examples/language-support/initializer-list.md.
#include <initializer_list>
#include <map>
#include <memory>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {
struct Bag {
    std::vector<int> data;
    Bag(std::initializer_list<int> il) : data{il} {}
};
}  // namespace

// "Accepting one": a braced list becomes an initializer_list with begin/end/size.
TEST(InitializerList, AcceptingOne) {
    auto take = [](std::initializer_list<int> il) {
        int s = 0;
        for (int x : il) s += x;
        return std::pair{s, il.size()};
    };
    auto [sum, n] = take({1, 2, 3, 4});
    EXPECT_EQ(sum, 10);
    EXPECT_EQ(n, 4u);
    Bag b{1, 2, 3};
    EXPECT_EQ(b.data, (std::vector<int>{1, 2, 3}));
}

// "Why containers feel built-in": vector, map and the accumulate lambda (6).
TEST(InitializerList, ContainersAndSum) {
    std::vector<int> v{1, 2, 3};
    std::map<int, int> m{{1, 10}, {2, 20}};
    auto sum = [](std::initializer_list<int> xs) { return std::accumulate(xs.begin(), xs.end(), 0); };
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(m.at(2), 20);
    EXPECT_EQ(sum({1, 2, 3}), 6);
}

// "Gotchas": v(3, 0) is {0,0,0}; v{3, 0} prefers the initializer_list constructor.
TEST(InitializerList, ParensVersusBraces) {
    std::vector<int> a(3, 0);
    std::vector<int> b{3, 0};
    EXPECT_EQ(a, (std::vector<int>{0, 0, 0}));
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 3);
}

// "Gotchas": elements are const (contiguous, read-only), so you can't move out
// (std::move on an element yields const T&&, which picks the copy constructor).
TEST(InitializerList, ElementsAreConst) {
    std::initializer_list<int> il{1, 2, 3};
    static_assert(std::is_same_v<decltype(*il.begin()), const int&>);
    EXPECT_EQ(il.begin() + 3, il.end());  // contiguous array
    static_assert(std::is_same_v<decltype(std::move(*il.begin())), const int&&>);
    static_assert(!std::is_constructible_v<std::unique_ptr<int>, const std::unique_ptr<int>&&>);
}

// "Gotchas": homogeneous — every element converts to one T.
TEST(InitializerList, Homogeneous) {
    auto il = {1, 2, 3};
    static_assert(std::is_same_v<decltype(il), std::initializer_list<int>>);
    SUCCEED();
}

// "Declaration": begin()/end() are const E*; free std::begin/end work; copying
// copies the view, not the elements.
TEST(InitializerList, DeclarationMembers) {
    std::initializer_list<int> il{1, 2, 3};
    static_assert(std::is_same_v<decltype(il.begin()), const int*>);
    static_assert(noexcept(il.size()));
    auto copy = il;
    EXPECT_EQ(copy.begin(), il.begin());  // same backing array
    EXPECT_EQ(std::end(il) - std::begin(il), 3);
    EXPECT_EQ(std::initializer_list<int>{}.size(), 0u);
}

// "What auto deduces": auto a = {…} is an initializer_list; auto b{1} is int (N3922).
TEST(InitializerList, AutoDeduction) {
    auto a = {1, 2, 3};
    auto b{1};
    static_assert(std::is_same_v<decltype(a), std::initializer_list<int>>);
    static_assert(std::is_same_v<decltype(b), int>);
}

// "Gotchas": narrowing is an error inside braces.
template <class T>
concept BracesFromDouble = requires(double d) { std::vector<T>{1, d}; };
TEST(InitializerList, NarrowingRejected) {
    static_assert(!BracesFromDouble<int>);
    static_assert(BracesFromDouble<double>);
}
