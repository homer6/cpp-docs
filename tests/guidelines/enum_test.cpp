// Checks claims made by docs/guidelines/core-guidelines/examples/enum.md.
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include <gtest/gtest.h>

namespace {

// Enum.1
enum class Web_color { red = 0xFF0000, blue = 0x0000FF };
enum class Shirt { red, blue };

// Enum.2
enum class State { idle, running, stopped };
std::string_view name(State s) {
    switch (s) {
    case State::idle:    return "idle";
    case State::running: return "running";
    case State::stopped: return "stopped";
    }
    return "?";
}

// Enum.3
enum PlainColor { plain_red, plain_green };
enum class Color { red, green };
int set_brightness(int level) { return level; }

// Enum.4
enum class Day { mon, tue, wed, thu, fri, sat, sun };
constexpr Day& operator++(Day& d) {
    d = d == Day::sun ? Day::mon : static_cast<Day>(static_cast<std::underlying_type_t<Day>>(d) + 1);
    return d;
}

// Enum.5: a macro named like an ALL_CAPS enumerator; lower-case enumerators don't collide.
#define RED 0xFF0000
enum class Paint { red, green, blue };

// Enum.6
enum { unnamed_buffer_size = 4096 };
constexpr std::size_t buffer_size = 4096;
constexpr int max_users = 8;
constexpr bool verbose = true;

// Enum.7
enum class Weekday { mon, tue, wed };
enum class Direction : std::uint8_t { n, e, s, w };
enum Permissions : std::uint32_t;
constexpr bool takes_permissions(Permissions) { return true; }   // usable before the definition
enum Permissions : std::uint32_t { can_read = 1, can_write = 2 };

// Enum.8
enum class Level { low, medium, high };
enum class Month { jan = 1, feb, mar, apr, may, jun, jul, aug, sep, oct, nov, dec };
enum class Open : unsigned { read = 1, write = 1 << 1, append = 1 << 2 };

template <class E>
constexpr auto value(E e) { return static_cast<std::underlying_type_t<E>>(e); }

}  // namespace

// Enum.1: enumerators of two enum classes with the same name don't clash and keep their own values.
TEST(CoreGuidelinesEnum, Enum1_EnumsInsteadOfMacros) {
    static_assert(value(Web_color::blue) == 0x0000FF);
    static_assert(value(Shirt::blue) == 1);
    static_assert(!std::is_same_v<Web_color, Shirt>);
    EXPECT_NE(value(Web_color::blue), value(Shirt::blue));
}

// Enum.2: an enumeration parameter accepts only its enumerators, not arbitrary ints.
TEST(CoreGuidelinesEnum, Enum2_RelatedConstantsAreOneType) {
    static_assert(!std::is_convertible_v<int, State>);
    EXPECT_EQ(name(State::idle), "idle");
    EXPECT_EQ(name(State::running), "running");
    EXPECT_EQ(name(State::stopped), "stopped");
}

// Enum.3: a plain enum converts to int implicitly; an enum class doesn't (only by static_cast).
TEST(CoreGuidelinesEnum, Enum3_EnumClassDoesNotConvert) {
    static_assert(std::is_convertible_v<PlainColor, int>);
    static_assert(!std::is_convertible_v<Color, int>);
    static_assert(!std::is_invocable_v<decltype(&set_brightness), Color>);
    static_assert(std::is_invocable_v<decltype(&set_brightness), PlainColor>);
    EXPECT_EQ(set_brightness(plain_green), 1);   // the silent conversion
    EXPECT_EQ(set_brightness(static_cast<int>(Color::green)), 1);
}

// Enum.4: an operator++ defined once on the enumeration steps through the days and wraps from sun to mon.
TEST(CoreGuidelinesEnum, Enum4_OperationsOnEnumerations) {
    Day d = Day::sat;
    EXPECT_EQ(++d, Day::sun);
    EXPECT_EQ(++d, Day::mon);
    EXPECT_EQ(++d, Day::tue);
    constexpr Day next = [] { Day x = Day::sun; return ++x; }();
    static_assert(next == Day::mon);
}

// Enum.5: lower-case enumerators compile next to ALL_CAPS macros and keep their own values.
TEST(CoreGuidelinesEnum, Enum5_NoAllCapsEnumerators) {
    static_assert(value(Paint::red) == 0);
    static_assert(RED == 0xFF0000);   // the macro is untouched too
    EXPECT_EQ(value(Paint::blue), 2);
}
#undef RED

// Enum.6: an unnamed enum's constant has an anonymous enumeration type; constexpr variables have the type you chose.
TEST(CoreGuidelinesEnum, Enum6_ConstexprInsteadOfUnnamedEnums) {
    static_assert(std::is_enum_v<decltype(unnamed_buffer_size)>);
    static_assert(std::is_same_v<decltype(buffer_size), const std::size_t>);
    static_assert(std::is_same_v<decltype(max_users), const int>);
    static_assert(std::is_same_v<decltype(verbose), const bool>);
    EXPECT_EQ(buffer_size, static_cast<std::size_t>(unnamed_buffer_size));
}

// Enum.7: an enum class is int underneath by default; a named type sets its size and allows a forward declaration.
TEST(CoreGuidelinesEnum, Enum7_UnderlyingTypeOnlyWhenNeeded) {
    static_assert(std::is_same_v<std::underlying_type_t<Weekday>, int>);
    static_assert(sizeof(Direction) == 1);
    static_assert(std::is_same_v<std::underlying_type_t<Permissions>, std::uint32_t>);
    static_assert(takes_permissions(can_write));
    EXPECT_EQ(value(Direction::w), 3);
}

// Enum.8: enumerators without values count 0, 1, 2…; explicit values give conventional numbers and bit flags.
TEST(CoreGuidelinesEnum, Enum8_ValuesOnlyWhenNeeded) {
    static_assert(value(Level::low) == 0 && value(Level::medium) == 1 && value(Level::high) == 2);
    static_assert(value(Month::feb) == 2 && value(Month::dec) == 12);
    static_assert(value(Open::append) == 4);
    EXPECT_EQ(value(Open::read) | value(Open::write) | value(Open::append), 7u);
}
