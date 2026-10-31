// Checks claims made by docs/examples/language/declarations-initialization/enumerations.md.
#include <cstdint>
#include <type_traits>
#include <utility>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
enum class Color { red, green, blue };
enum class Status : std::uint8_t { ok, warn, error };
enum Direction { north, south, east, west };
enum Flags : unsigned { a = 1, b = 2, c = 4 };

bool is_red(Color c) {
    using enum Color;
    return c == red;
}

template <class T> concept HasBitOr = requires(T x) { x | x; };
}  // namespace

// "Scoped enums": values 0, 1, 2; no implicit conversion; chosen underlying type.
TEST(Enumerations, ScopedEnums) {
    Color c = Color::green;
    EXPECT_EQ(static_cast<int>(c), 1);
    EXPECT_EQ(static_cast<int>(Color::red), 0);
    EXPECT_EQ(static_cast<int>(Color::blue), 2);
    static_assert(!std::is_convertible_v<Color, int>);
    static_assert(!std::is_convertible_v<int, Color>);
    static_assert(std::is_same_v<std::underlying_type_t<Status>, std::uint8_t>);
    static_assert(sizeof(Status) == 1);  // fixed size for ABI/serialization
    static_assert(std::is_same_v<std::underlying_type_t<Color>, int>);  // scoped default is int
}

// "Unscoped enums": names leak and implicitly convert to int; explicit values and underlying type.
TEST(Enumerations, UnscopedEnums) {
    Direction d = north;
    int x = north;
    EXPECT_EQ(d, north);
    EXPECT_EQ(x, 0);
    EXPECT_EQ(int{west}, 3);
    static_assert(std::is_convertible_v<Direction, int>);
    static_assert(std::is_same_v<std::underlying_type_t<Flags>, unsigned>);
    EXPECT_EQ(a | b | c, 7u);  // works as bit flags because they convert
}

// "Using scoped enum values conveniently (C++20)": using enum brings the enumerators into scope.
TEST(Enumerations, UsingEnum) {
    EXPECT_TRUE(is_red(Color::red));
    EXPECT_FALSE(is_red(Color::blue));
}

// "Helpers": std::to_underlying(Status::error) is std::uint8_t{2}.
TEST(Enumerations, ToUnderlying) {
    CPPDOCS_REQUIRE(__cpp_lib_to_underlying);
#ifdef __cpp_lib_to_underlying
    auto code = std::to_underlying(Status::error);
    static_assert(std::is_same_v<decltype(code), std::uint8_t>);
    EXPECT_EQ(code, std::uint8_t{2});
    EXPECT_EQ(code, static_cast<std::underlying_type_t<Status>>(Status::error));
#endif
}

// Gotchas: enum class has no bitwise operators out of the box.
TEST(Enumerations, ScopedEnumsHaveNoBitwiseOps) {
    static_assert(!HasBitOr<Color>);
    static_assert(HasBitOr<Flags>);
}
