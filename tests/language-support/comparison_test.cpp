// Checks claims made by docs/examples/language-support/comparison.md.
#include <algorithm>
#include <compare>
#include <limits>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

struct Point {
    int x, y;
    auto operator<=>(const Point&) const = default;
    bool operator==(const Point&) const = default;
};

// "Default it — the common case": lexicographic on x then y; all six operators; sortable.
TEST(Comparison, DefaultedIsLexicographic) {
    EXPECT_TRUE((Point{1, 2} < Point{1, 3}));
    EXPECT_TRUE((Point{0, 9} < Point{1, 0}));
    EXPECT_TRUE((Point{1, 2} != Point{2, 1}));
    EXPECT_TRUE((Point{2, 1} >= Point{1, 5}));
    std::vector<Point> pts{{2, 1}, {1, 3}, {1, 2}};
    std::sort(pts.begin(), pts.end());
    EXPECT_EQ(pts, (std::vector<Point>{{1, 2}, {1, 3}, {2, 1}}));
}

struct Version {
    int major, minor, patch;
    std::strong_ordering operator<=>(const Version& o) const {
        if (auto c = major <=> o.major; c != 0) return c;
        if (auto c = minor <=> o.minor; c != 0) return c;
        return patch <=> o.patch;
    }
    bool operator==(const Version&) const = default;
};

// "Custom ordering": field-by-field comparison.
TEST(Comparison, CustomOrdering) {
    EXPECT_TRUE((Version{1, 2, 3} < Version{1, 10, 0}));
    EXPECT_EQ((Version{2, 0, 0} <=> Version{1, 9, 9}), std::strong_ordering::greater);
    EXPECT_TRUE((Version{1, 2, 3} == Version{1, 2, 3}));
}

// "Using the result": int <=> int is strong_ordering; floats give partial_ordering, NaN unordered.
TEST(Comparison, OrderingCategories) {
    int a = 1, b = 2;
    static_assert(std::is_same_v<decltype(a <=> b), std::strong_ordering>);
    EXPECT_TRUE((a <=> b) < 0);

    double nan = std::numeric_limits<double>::quiet_NaN();
    static_assert(std::is_same_v<decltype(1.0 <=> 2.0), std::partial_ordering>);
    EXPECT_EQ((nan <=> 1.0), std::partial_ordering::unordered);
    EXPECT_FALSE(nan < 1.0);
    EXPECT_FALSE(nan >= 1.0);  // !(a < b) does not imply a >= b
}

// "Gotchas": a user-written <=> does not give ==; a defaulted <=> also declares a defaulted ==.
struct OnlyCustomSpaceship {
    int v;
    std::strong_ordering operator<=>(const OnlyCustomSpaceship& o) const { return v <=> o.v; }
};
struct OnlyDefaultedSpaceship {
    int v;
    auto operator<=>(const OnlyDefaultedSpaceship&) const = default;
};

TEST(Comparison, EqualityIsNotSynthesizedFromCustomSpaceship) {
    static_assert(std::totally_ordered<OnlyDefaultedSpaceship>);
    static_assert(!std::equality_comparable<OnlyCustomSpaceship>);
    EXPECT_TRUE((OnlyCustomSpaceship{1} < OnlyCustomSpaceship{2}));
    EXPECT_TRUE((OnlyDefaultedSpaceship{1} == OnlyDefaultedSpaceship{1}));
}

// "The three ordering types": weak_ordering for equivalent-but-not-identical
// values, e.g. a case-insensitive comparison.
struct CiChar {
    char c;
    std::weak_ordering operator<=>(const CiChar& o) const {
        auto lower = [](char ch) { return ch >= 'A' && ch <= 'Z' ? char(ch - 'A' + 'a') : ch; };
        return lower(c) <=> lower(o.c);
    }
};
TEST(Comparison, WeakOrderingEquivalence) {
    EXPECT_EQ((CiChar{'a'} <=> CiChar{'A'}), std::weak_ordering::equivalent);
    EXPECT_EQ((CiChar{'b'} <=> CiChar{'A'}), std::weak_ordering::greater);
}

// "Gotchas": a defaulted <=> compares bases first, then members in declaration order.
struct Base {
    int b;
    auto operator<=>(const Base&) const = default;
};
struct Derived : Base {
    int d;
    auto operator<=>(const Derived&) const = default;
};
TEST(Comparison, BasesBeforeMembers) {
    EXPECT_TRUE((Derived{{1}, 9} < Derived{{2}, 0}));  // base decides first
    EXPECT_TRUE((Derived{{1}, 0} < Derived{{1}, 1}));
}
