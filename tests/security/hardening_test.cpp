// Checks claims made by docs/security/hardening.md: in a hardened build
// (CPPDOCS_HARDEN, #305, or a library default that turns its checks on), the
// standard library's precondition checks stop the program on an out-of-bounds
// or empty access instead of letting it read or write past the object (#306).
// Each check runs in a death test; a build without library hardening skips
// them, naming why.
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <version>
#if __has_include(<expected>)
#  include <expected>
#endif

#include <gtest/gtest.h>

namespace {

// Indexes the optimizer can't see through, so the access really happens.
volatile std::size_t three = 3;
volatile std::size_t ten = 10;

}  // namespace

// Library hardening is active when the library's own switch is on, whether
// CPPDOCS_HARDEN set it or the library's default did (libstdc++ 15+ enables
// its assertions in unoptimized builds; see language-versions.md).
#if defined(_GLIBCXX_ASSERTIONS)
#  define CPPDOCS_LIBRARY_HARDENED 1
#elif defined(_LIBCPP_HARDENING_MODE) && defined(_LIBCPP_HARDENING_MODE_NONE)
#  if _LIBCPP_HARDENING_MODE != _LIBCPP_HARDENING_MODE_NONE
#    define CPPDOCS_LIBRARY_HARDENED 1
#  endif
#elif defined(_MSVC_STL_HARDENING)
#  if _MSVC_STL_HARDENING
#    define CPPDOCS_LIBRARY_HARDENED 1
#  endif
#endif

#if defined(CPPDOCS_LIBRARY_HARDENED)
#  define CPPDOCS_REQUIRE_HARDENED() (void)0
#else
#  define CPPDOCS_REQUIRE_HARDENED() \
      GTEST_SKIP() << "library hardening is off in this build (no _GLIBCXX_ASSERTIONS, " \
                      "_LIBCPP_HARDENING_MODE or _MSVC_STL_HARDENING; the *-hardened presets turn it on)"
#endif

// "Standard-library hardening": vector::operator[] past the end stops the program.
TEST(SecurityHardeningDeathTest, VectorIndexOutOfRange) {
    CPPDOCS_REQUIRE_HARDENED();
    std::vector<int> v(3);
    EXPECT_DEATH({ volatile int x = v[three]; (void)x; }, "");
}

// "Standard-library hardening": vector::front() on an empty vector stops the program.
TEST(SecurityHardeningDeathTest, VectorFrontOnEmpty) {
    CPPDOCS_REQUIRE_HARDENED();
    std::vector<int> v;
    EXPECT_DEATH({ volatile int x = v.front(); (void)x; }, "");
}

// "Standard-library hardening": std::array::operator[] past the end stops the program.
TEST(SecurityHardeningDeathTest, ArrayIndexOutOfRange) {
    CPPDOCS_REQUIRE_HARDENED();
    std::array<int, 3> a{};
    EXPECT_DEATH({ volatile int x = a[three]; (void)x; }, "");
}

// "Standard-library hardening": std::span::operator[] past the end stops the program.
TEST(SecurityHardeningDeathTest, SpanIndexOutOfRange) {
    CPPDOCS_REQUIRE_HARDENED();
    int raw[3] = {};
    std::span<int> s{raw};
    EXPECT_DEATH({ volatile int x = s[three]; (void)x; }, "");
}

// "Standard-library hardening": string::operator[] past size() stops the program
// (index == size() is allowed and reads the terminator).
TEST(SecurityHardeningDeathTest, StringIndexOutOfRange) {
    CPPDOCS_REQUIRE_HARDENED();
    std::string str = "abc";
    EXPECT_DEATH({ volatile char c = str[ten]; (void)c; }, "");
}

// "Standard-library hardening": string_view::operator[] past the end stops the program.
TEST(SecurityHardeningDeathTest, StringViewIndexOutOfRange) {
    CPPDOCS_REQUIRE_HARDENED();
    std::string_view sv = "abc";
    EXPECT_DEATH({ volatile char c = sv[ten]; (void)c; }, "");
}

// "Standard-library hardening": dereferencing an empty optional stops the program.
TEST(SecurityHardeningDeathTest, EmptyOptionalDereference) {
    CPPDOCS_REQUIRE_HARDENED();
    std::optional<int> o;
    EXPECT_DEATH({ volatile int x = *o; (void)x; }, "");
}

// "Standard-library hardening": operator* on an expected holding an error stops the program.
TEST(SecurityHardeningDeathTest, ExpectedWithErrorDereference) {
    CPPDOCS_REQUIRE_HARDENED();
#if defined(__cpp_lib_expected)
    std::expected<int, int> e = std::unexpected(1);
    EXPECT_DEATH({ volatile int x = *e; (void)x; }, "");
#else
    GTEST_SKIP() << "needs __cpp_lib_expected (not defined by this toolchain)";
#endif
}

// "Standard-library hardening": the checked accesses in range still work normally.
TEST(SecurityHardening, InRangeAccessUnaffected) {
    std::vector<int> v{1, 2, 3};
    std::string str = "abc";
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(str[3], '\0');   // index == size() is valid for basic_string
    std::optional<int> o = 7;
    EXPECT_EQ(*o, 7);
}
