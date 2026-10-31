// Checks claims made by docs/examples/language/basic-concepts/fundamental-types.md.
#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <gtest/gtest.h>

// "Sizes are minimums, not fixed": int has at least 16 bits; <cstdint> aliases are exact.
TEST(FundamentalTypes, SizesAndFixedWidthAliases) {
    static_assert(std::numeric_limits<int>::digits + 1 >= 16);
    static_assert(sizeof(std::int32_t) * CHAR_BIT == 32 && std::is_signed_v<std::int32_t>);
    static_assert(sizeof(std::uint64_t) * CHAR_BIT == 64 && std::is_unsigned_v<std::uint64_t>);
    static_assert(std::numeric_limits<std::int_fast16_t>::digits + 1 >= 16);
    static_assert(sizeof(std::intptr_t) >= sizeof(void*));
    static_assert(std::is_unsigned_v<std::size_t> && std::is_same_v<decltype(sizeof(int)), std::size_t>);
    int arr[4]{};
    static_assert(std::is_signed_v<std::ptrdiff_t>);
    static_assert(std::is_same_v<decltype(&arr[3] - &arr[0]), std::ptrdiff_t>);
    EXPECT_EQ(&arr[3] - &arr[0], 3);
}

// "Initialization defaults": int y{} is 0; digit separators don't change the value.
TEST(FundamentalTypes, ValueInitAndDigitSeparators) {
    int y{};
    EXPECT_EQ(y, 0);
    auto big = 5'000'000;
    static_assert(std::is_same_v<decltype(big), int>);
    EXPECT_EQ(big, 5000000);
}

// "Gotchas": char, signed char and unsigned char are three distinct types; char8_t is distinct from char.
TEST(FundamentalTypes, CharTypesAreDistinct) {
    static_assert(!std::is_same_v<char, signed char>);
    static_assert(!std::is_same_v<char, unsigned char>);
    static_assert(!std::is_same_v<char8_t, char>);
    static_assert(std::is_same_v<decltype(u8"x"), const char8_t(&)[2]>);
}

// "Gotchas": unsigned arithmetic wraps modulo 2^N.
TEST(FundamentalTypes, UnsignedWraps) {
    unsigned u = 0;
    --u;
    EXPECT_EQ(u, std::numeric_limits<unsigned>::max());
    EXPECT_EQ(u + 1u, 0u);
}
