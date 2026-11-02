// Checks claims made by docs/examples/numerics/bit.md.
#include <bit>
#include <cstdint>
#include <type_traits>
#include <version>

// libstdc++'s C++ <stdbit.h> (GCC 15+, C++26 mode); not glibc's C-only header.
// FreeBSD 15's system <stdbit.h> is C-only (_Bool), so without libstdc++'s own header
// (GCC 14.2, the FreeBSD package) including it doesn't compile; skip it there.
#if __has_include(<stdbit.h>) && defined(__GLIBCXX__) && __cplusplus > 202302L && \
    !(defined(__FreeBSD__) && _GLIBCXX_RELEASE < 15)
#include <stdbit.h>
#endif
#if defined(_GLIBCXX_STDBIT_H) || defined(__cpp_lib_stdbit_h)
#define CPPDOCS_HAS_STDBIT 1
#else
#define CPPDOCS_HAS_STDBIT 0
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
template <class T>
constexpr bool popcount_accepts = requires(T x) { std::popcount(x); };
template <class T>
constexpr bool rotl_accepts = requires(T x) { std::rotl(x, 3); };
}  // namespace

// "std::bit_cast": raw IEEE-754 bits of 1.5f, and the round trip is exact.
TEST(Bit, BitCastRoundTrips) {
    float f = 1.5f;
    auto bits = std::bit_cast<std::uint32_t>(f);
    EXPECT_EQ(bits, 0x3FC00000u);  // sign 0, exponent 127, mantissa .1b
    EXPECT_EQ(std::bit_cast<float>(bits), f);
}

// "Counting and inspecting bits": the values in the comments.
TEST(Bit, CountingValues) {
    static_assert(std::popcount(0b1011u) == 3);
    static_assert(std::countr_zero(0b0001'0000u) == 4);
    static_assert(std::countl_zero(0b0001'0000u) == 32 - 5);  // 32-bit unsigned
    static_assert(std::bit_width(13u) == 4);
    static_assert(std::countr_one(0b0111u) == 3);
}

// "Power-of-two helpers" and "Rotates": bits wrap around.
TEST(Bit, PowerOfTwoAndRotate) {
    static_assert(std::has_single_bit(16u));
    static_assert(!std::has_single_bit(17u));
    static_assert(std::bit_ceil(17u) == 32);
    static_assert(std::bit_floor(17u) == 16);
    static_assert(std::rotl(std::uint32_t{0x8000'0001}, 3) == 0x0000'000Cu);
    static_assert(std::rotr(std::uint32_t{0x0000'000C}, 3) == 0x8000'0001u);
}

// "Gotchas": the counting/rotate functions reject signed types.
TEST(Bit, SignedTypesRejected) {
    static_assert(!popcount_accepts<int>);
    static_assert(!rotl_accepts<int>);
    static_assert(popcount_accepts<unsigned>);
    static_assert(rotl_accepts<std::uint32_t>);
}

// "Endianness & byte swap": 0x1234 -> 0x3412; 0x1234u -> 0x34120000 (C++23).
TEST(Bit, Byteswap) {
    CPPDOCS_REQUIRE(__cpp_lib_byteswap);
#ifdef __cpp_lib_byteswap
    static_assert(std::byteswap(std::uint16_t{0x1234}) == 0x3412);
    static_assert(std::byteswap(std::uint32_t{0x1234u}) == 0x34120000u);
#endif
}

// "std::bit_cast": both types must be trivially copyable and the same size.
template <class To, class From>
concept BitCastable = requires(From f) { std::bit_cast<To>(f); };
TEST(Bit, BitCastNeedsSameSize) {
    static_assert(BitCastable<std::uint32_t, float>);
    static_assert(!BitCastable<std::uint64_t, float>);
    static_assert(std::bit_cast<std::uint32_t>(1.5f) == 0x3FC00000u);  // constexpr
}

// "Endianness": std::endian::native is little on every local toolchain (arm64, x86-64).
TEST(Bit, NativeEndian) {
    static_assert(std::endian::native == std::endian::little ||
                  std::endian::native == std::endian::big);
    std::uint32_t v = 1;
    bool little = *reinterpret_cast<unsigned char*>(&v) == 1;
    EXPECT_EQ(little, std::endian::native == std::endian::little);
}

// "C compatibility: <stdbit.h> (C++26)": global-namespace C23 bit functions.
// GCC 15 ships the header without the feature macro; libc++ 22 lacks it.
TEST(Bit, StdbitH) {
#if CPPDOCS_HAS_STDBIT
    static_assert(std::is_same_v<decltype(stdc_count_ones(0b1011u)), unsigned int>);
    EXPECT_EQ(stdc_count_ones(0b1011u), 3u);
    EXPECT_EQ(stdc_bit_width(13u), 4u);
    EXPECT_EQ(stdc_leading_zeros_ui(1u), 31u);
    EXPECT_TRUE(stdc_has_single_bit(16u));
    EXPECT_EQ(stdc_bit_ceil(17u), 32u);
#ifdef __cpp_lib_stdbit_h
    EXPECT_GE(__cpp_lib_stdbit_h, 202603L);
#endif
#else
    GTEST_SKIP() << "<stdbit.h> (C++26) not provided by this standard library";
#endif
}
