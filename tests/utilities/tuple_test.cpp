// Checks claims made by docs/examples/utilities/tuple.md.
#include <string>
#include <tuple>
#include <type_traits>

#include <gtest/gtest.h>

namespace {
std::tuple<int, int> divmod(int a, int b) { return {a / b, a % b}; }

struct P { int x, y; };
bool less(const P& a, const P& b) { return std::tie(a.x, a.y) < std::tie(b.x, b.y); }
}  // namespace

// "Make, access, unpack": get by index / by type, tuple_size_v, CTAD, structured bindings.
TEST(Tuple, MakeAccessUnpack) {
    std::tuple<int, double, std::string> t{1, 2.5, "hi"};
    std::tuple u{1, 2.5, "hi"};
    static_assert(std::is_same_v<decltype(u), std::tuple<int, double, const char*>>);

    EXPECT_EQ(std::get<0>(t), 1);
    EXPECT_EQ(std::get<std::string>(t), "hi");  // get<T> on a repeated T is a hard error, not testable at run time
    static_assert(std::tuple_size_v<decltype(t)> == 3);

    auto [i, d, s] = t;
    EXPECT_EQ(i, 1);
    EXPECT_EQ(d, 2.5);
    EXPECT_EQ(s, "hi");
}

// "Multiple return values": divmod(17, 5) gives q == 3, r == 2.
TEST(Tuple, MultipleReturnValues) {
    auto [q, r] = divmod(17, 5);
    EXPECT_EQ(q, 3);
    EXPECT_EQ(r, 2);
}

// "tie and ignore": tie assigns into existing variables; ignore discards; tie compares lexicographically.
TEST(Tuple, TieAndIgnore) {
    int q = 0, r = 0;
    std::tie(q, r) = divmod(17, 5);
    EXPECT_EQ(q, 3);
    EXPECT_EQ(r, 2);
    std::tie(std::ignore, r) = divmod(9, 4);
    EXPECT_EQ(q, 3);  // untouched
    EXPECT_EQ(r, 1);

    EXPECT_TRUE(less({1, 9}, {2, 0}));   // first field decides
    EXPECT_TRUE(less({1, 1}, {1, 2}));   // tie broken by the second
    EXPECT_FALSE(less({1, 2}, {1, 2}));
}

// "Combine and forward": tuple_cat yields tuple<int,int,char>; apply calls with unpacked args -> 7.
TEST(Tuple, CombineAndForward) {
    auto cat = std::tuple_cat(std::tuple{1, 2}, std::tuple{'a'});
    static_assert(std::is_same_v<decltype(cat), std::tuple<int, int, char>>);
    EXPECT_EQ(cat, std::make_tuple(1, 2, 'a'));
    EXPECT_EQ(std::apply([](int a, int b) { return a + b; }, std::tuple{3, 4}), 7);
}

// "Gotchas": make_tuple decays (strips references); tie keeps lvalue refs; forward_as_tuple forwards.
TEST(Tuple, MakeTupleDecaysTieAndForwardAsTupleKeepReferences) {
    int x = 1;
    int& rx = x;
    const char arr[] = "hi";
    static_assert(std::is_same_v<decltype(std::make_tuple(rx, arr)), std::tuple<int, const char*>>);
    static_assert(std::is_same_v<decltype(std::tie(x)), std::tuple<int&>>);
    static_assert(std::is_same_v<decltype(std::forward_as_tuple(x, 2)), std::tuple<int&, int&&>>);

    std::get<0>(std::tie(x)) = 5;
    EXPECT_EQ(x, 5);
}
