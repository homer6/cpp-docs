// Checks claims made by docs/security/integers.md.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <version>
#if __has_include(<stdckdint.h>)
#  include <stdckdint.h>
#  define CPPDOCS_HAVE_STDCKDINT 1
#endif

#include <gtest/gtest.h>

namespace {

constexpr std::size_t kMax = std::numeric_limits<std::size_t>::max();

constexpr bool valid_slice(std::size_t total, std::size_t offset, std::size_t length) noexcept {
    return offset <= total && length <= total - offset;
}

constexpr std::optional<std::size_t>
checked_allocation_size(std::size_t count, std::size_t element_size, std::size_t budget) noexcept {
    if (element_size == 0 || count > budget / element_size) return std::nullopt;
    return count * element_size;
}

bool total_length(std::size_t a, std::size_t b, std::size_t& out) {
#if defined(CPPDOCS_HAVE_STDCKDINT)
    return !ckd_add(&out, a, b);
#else
    if (b > kMax - a) return false;
    out = a + b;
    return true;
#endif
}

}  // namespace

// "Check before the arithmetic": the slice check never computes offset + length, so a
// length chosen to wrap the sum is rejected.
TEST(SecurityIntegers, ValidSliceRejectsWrappingLength) {
    EXPECT_TRUE(valid_slice(10, 4, 6));
    EXPECT_TRUE(valid_slice(10, 10, 0));
    EXPECT_FALSE(valid_slice(10, 4, 7));
    EXPECT_FALSE(valid_slice(10, 11, 0));
    EXPECT_FALSE(valid_slice(10, 4, kMax));       // 4 + kMax wraps to 3, which a naive check accepts
    std::size_t naive_end = 4 + kMax;             // what the naive check would compute (well-defined wrap)
    EXPECT_EQ(naive_end, 3u);
}

// "Check before the arithmetic": the allocation size is checked against a budget,
// and products that would wrap are rejected.
TEST(SecurityIntegers, AllocationSizeCheckedAgainstBudget) {
    EXPECT_EQ(checked_allocation_size(1000, 4, 4096), 4000u);
    EXPECT_EQ(checked_allocation_size(1024, 4, 4096), 4096u);
    EXPECT_FALSE(checked_allocation_size(1025, 4, 4096));
    EXPECT_FALSE(checked_allocation_size(1, 0, 4096));
    EXPECT_FALSE(checked_allocation_size(kMax / 2, 4, kMax));   // the product would wrap
}

// "Check before the arithmetic": a checked add (ckd_add where available) reports overflow.
TEST(SecurityIntegers, CheckedAddReportsOverflow) {
    std::size_t out = 0;
    EXPECT_TRUE(total_length(2, 3, out));
    EXPECT_EQ(out, 5u);
    EXPECT_FALSE(total_length(kMax, 1, out));
}

// "Check before the arithmetic": <stdckdint.h> is usable from C++, and libstdc++ 16+
// defines __cpp_lib_stdckdint_h (202603L).
TEST(SecurityIntegers, StdckdintAvailability) {
#if !defined(CPPDOCS_HAVE_STDCKDINT)
    GTEST_SKIP() << "<stdckdint.h> not available to C++ with this toolchain (P3370R1, C++26)";
#else
    int r = 0;
    EXPECT_TRUE(ckd_add(&r, std::numeric_limits<int>::max(), 1));
    EXPECT_FALSE(ckd_mul(&r, 1000, 1000));
    EXPECT_EQ(r, 1000000);
#  if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE >= 16
#    if defined(__cpp_lib_stdckdint_h)
    EXPECT_GE(__cpp_lib_stdckdint_h, 202603L);
#    else
    ADD_FAILURE() << "libstdc++ 16 should define __cpp_lib_stdckdint_h";
#    endif
#  endif
#endif
}

// "Define the numeric domain": in_range and cmp_* compare values, while a cast keeps
// only the bits.
TEST(SecurityIntegers, InRangeAndCmpCompareValues) {
    std::int64_t from_wire = -1;
    EXPECT_FALSE(std::in_range<std::size_t>(from_wire));
    EXPECT_FALSE(std::in_range<std::uint16_t>(70000));
    EXPECT_TRUE(std::in_range<std::uint16_t>(65535));
    EXPECT_TRUE(std::cmp_less(from_wire, 0u));
    EXPECT_EQ(static_cast<std::size_t>(from_wire), kMax);
    EXPECT_EQ(static_cast<std::uint16_t>(70000), 70000 - 65536);   // truncated, not rejected
}
