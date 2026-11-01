// Checks claims made by docs/examples/language/declarations-initialization/initialization.md.
#include <initializer_list>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {
struct Config { int width = 80; int height = 25; bool wrap = false; };

struct Explicit { explicit Explicit(int) {} };

struct Defaulted { int n = 7; };  // default-initialization runs the default constructor

struct Picky {  // list-init prefers the initializer_list constructor
    std::string which;
    Picky(int, int) : which("(int, int)") {}
    Picky(std::initializer_list<int>) : which("initializer_list") {}
};

template <class T>
concept BraceFromDouble = requires(double d) { T{d}; };
}  // namespace

// "The forms": every spelling of an int initializer gives the same value; the rest as commented.
TEST(Initialization, TheForms) {
    int a = 5;
    int b(5);
    int c{5};
    int d = {5};
    int e{};
    EXPECT_EQ(a + b + c + d, 20);
    EXPECT_EQ(e, 0);  // value initialization → 0

    std::string s{"hi"};
    std::vector<int> v{1, 2, 3};
    auto w = std::vector<int>(5, 0);
    EXPECT_EQ(s, "hi");
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(w, (std::vector<int>(5, 0)));
    EXPECT_EQ(w.size(), 5u);
}

// "What the categories mean": default-init of a class runs its constructor; {} prefers initializer_list.
TEST(Initialization, CategoryMeanings) {
    Defaulted x;
    EXPECT_EQ(x.n, 7);
    std::string str;  // default-constructed, not garbage
    EXPECT_TRUE(str.empty());

    EXPECT_EQ(Picky(1, 2).which, "(int, int)");
    EXPECT_EQ((Picky{1, 2}).which, "initializer_list");

    int arr[3]{1};  // aggregate init of an array: the rest are zero
    EXPECT_EQ(arr[1] + arr[2], 0);
    static_assert(std::is_aggregate_v<Config>);
}

// "The () vs {} trap": (5, 0) is five zeros; {5, 0} is two elements.
TEST(Initialization, ParenVersusBraceForVector) {
    std::vector<int> v1(5, 0);
    std::vector<int> v2{5, 0};
    EXPECT_EQ(v1, (std::vector<int>{0, 0, 0, 0, 0}));
    EXPECT_EQ(v2.size(), 2u);
    EXPECT_EQ(v2[0], 5);
}

// "Aggregates & designated initializers": unnamed members keep their default member initializers.
TEST(Initialization, DesignatedInitializersKeepDefaults) {
    Config a{100, 40};
    EXPECT_EQ(a.width, 100);
    EXPECT_EQ(a.height, 40);
    EXPECT_FALSE(a.wrap);

    Config b{.width = 100, .wrap = true};
    EXPECT_EQ(b.width, 100);
    EXPECT_EQ(b.height, 25);  // "height stays 25"
    EXPECT_TRUE(b.wrap);
}

// "What the categories mean": value-initialization zeroes scalars.
TEST(Initialization, ValueInitializationZeroes) {
    int e{};
    double d{};
    int* p{};
    EXPECT_EQ(e, 0);
    EXPECT_EQ(d, 0.0);
    EXPECT_EQ(p, nullptr);
    EXPECT_EQ(int(), 0);
}

// "Gotchas": {} bans narrowing; copy-list-init can't call an explicit constructor.
TEST(Initialization, BracesBanNarrowingAndCopyListSkipsExplicit) {
    static_assert(!BraceFromDouble<int>);       // int{3.9} is ill-formed
    static_assert(BraceFromDouble<double>);
    static_assert(std::is_constructible_v<Explicit, int>);   // Explicit e{1}; is fine
    static_assert(!std::is_convertible_v<int, Explicit>);    // Explicit e = {1}; is not
}

namespace paren_agg {
struct Point { int x; int y; };
struct In { int a, b; };
struct Out { In i; int c; };
struct Ref { const int& r; };
constexpr bool braces_extend() { Ref a{42}; return a.r == 42; }
template <class T, class... A>
concept ParenInit = requires(A... a) { T(a...); };
}  // namespace paren_agg

// "Aggregates & designated initializers": parenthesized aggregate init (C++20).
TEST(Initialization, ParenthesizedAggregateInit) {
    using paren_agg::Point;
    Point p(1, 2);
    EXPECT_EQ(p.x, 1);
    EXPECT_EQ(p.y, 2);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"  // the omission is the point
    Point q(5);
#pragma GCC diagnostic pop
    EXPECT_EQ(q.y, 0);
    int arr[](1, 2, 3);
    static_assert(sizeof arr / sizeof arr[0] == 3);
    std::vector<Point> pts;
    pts.emplace_back(3, 4);
    EXPECT_EQ(pts[0].y, 4);
    double d = 2.9;
    Point n(d, 1);  // parentheses allow narrowing
    EXPECT_EQ(n.x, 2);
    static_assert(!paren_agg::ParenInit<paren_agg::Out, int, int, int>);  // no brace elision
    static_assert(paren_agg::braces_extend());  // braces extend the temporary; parens don't
}
