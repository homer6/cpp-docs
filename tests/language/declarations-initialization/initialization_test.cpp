// Checks claims made by docs/examples/language/declarations-initialization/initialization.md.
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {
struct Config { int width = 80; int height = 25; bool wrap = false; };

struct Explicit { explicit Explicit(int) {} };

template <class T>
concept BraceFromDouble = requires(double d) { T{d}; };
}  // namespace

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
