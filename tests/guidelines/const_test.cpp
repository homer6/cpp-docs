// Checks claims made by docs/guidelines/core-guidelines/examples/const.md.
#include <array>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace {
struct Circle {
    double r;
    double area() const { return 3.0 * r * r; }
    void grow() { r += 1; }
};

std::size_t length(const std::string& s) { return s.size(); }

constexpr int square(int x) { return x * x; }

template <class T>
concept can_grow = requires(T& t) { t.grow(); };
}  // namespace

// Con.1: a const object can't be assigned to, so an accidental write is a compile error.
TEST(CoreGuidelinesCon, Con1_ConstObjectsRejectWrites) {
    const int result = 6 * 7;
    static_assert(std::is_const_v<decltype(result)>);
    static_assert(!std::is_assignable_v<const int&, int>);
    EXPECT_EQ(result, 42);
}

// Con.2: a const member function can be called on a const object; a non-const one can't.
TEST(CoreGuidelinesCon, Con2_ConstMemberFunctionsWorkOnConstObjects) {
    const Circle c{2.0};
    EXPECT_DOUBLE_EQ(c.area(), 12.0);
    static_assert(std::is_invocable_v<decltype(&Circle::area), const Circle&>);
    static_assert(!std::is_invocable_v<decltype(&Circle::grow), const Circle&>);
    static_assert(can_grow<Circle> && !can_grow<const Circle>);
}

// Con.3: a const& parameter accepts temporaries and const objects; a plain & doesn't.
TEST(CoreGuidelinesCon, Con3_ConstRefAcceptsTemporaries) {
    const std::string fixed = "abc";
    EXPECT_EQ(length(std::string{"hi"}), 2u);
    EXPECT_EQ(length(fixed), 3u);
    static_assert(std::is_convertible_v<std::string&&, const std::string&>);
    static_assert(!std::is_convertible_v<std::string&&, std::string&>);
    static_assert(!std::is_convertible_v<const std::string&, std::string&>);
}

// Con.4: a const object initialized at run time keeps its value for its lifetime.
TEST(CoreGuidelinesCon, Con4_RunTimeValueFixedAfterConstruction) {
    std::string name = "world";
    const std::string greeting = "Hello, " + name;
    name = "there";
    EXPECT_EQ(greeting, "Hello, world");
    static_assert(!std::is_assignable_v<const std::string&, const char*>);
}

// Con.5: a constexpr value is a constant expression: usable in static_assert and as an array bound.
TEST(CoreGuidelinesCon, Con5_ConstexprIsComputedAtCompileTime) {
    constexpr int side = 4;
    constexpr int area = square(side);
    static_assert(area == 16);
    std::array<int, square(3)> cells{};
    EXPECT_EQ(cells.size(), 9u);
    EXPECT_EQ(area, 16);
}
