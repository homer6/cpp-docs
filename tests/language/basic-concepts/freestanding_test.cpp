// Checks claims made by docs/examples/language/basic-concepts/freestanding.md.
#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <span>
#include <string_view>
#include <version>

#include <gtest/gtest.h>

#include "support/capture.hpp"

namespace {
constexpr std::optional<std::size_t> find_byte(std::span<const std::uint8_t> bytes,
                                               std::uint8_t b) {
    for (std::size_t i = 0; i < bytes.size(); ++i)
        if (bytes[i] == b) return i;
    return std::nullopt;
}

constexpr std::array<std::uint8_t, 4> header{0x7F, 'E', 'L', 'F'};

#if __STDC_HOSTED__
void debug_log(std::string_view msg) { std::fwrite(msg.data(), 1, msg.size(), stdout); }
#else
void debug_log(std::string_view) {}
#endif
}  // namespace

// "Detect the implementation kind" / "Guarding a hosted-only path": this suite is hosted.
TEST(Freestanding, HostedBuildTakesHostedPath) {
    static_assert(__STDC_HOSTED__ == 1);
    auto out = cppdocs::capture_stdout([] {
#if __STDC_HOSTED__
        std::puts("hosted");
#else
        std::puts("freestanding");
#endif
    });
    EXPECT_EQ(out, "hosted\n");
    // The page logs to stderr; stdout here so the capture helper can see it.
    EXPECT_EQ(cppdocs::capture_stdout([] { debug_log("msg"); }), "msg");
}

// "Freestanding-friendly code": find_byte works in a constant expression.
TEST(Freestanding, FindByteIsConstexpr) {
    static_assert(find_byte(header, 'L') == 2);
    static_assert(!find_byte(header, 'Z').has_value());
    EXPECT_EQ(find_byte(header, 0x7F), 0u);
}

// "Feature-test macros": the table of __cpp_lib_freestanding_* each local library defines.
TEST(Freestanding, LibraryFreestandingMacros) {
#if defined(__GLIBCXX__) && !defined(__cpp_lib_freestanding_algorithm)
    GTEST_SKIP() << "libstdc++ before GCC 14 (here GCC " << __GNUC__
                 << ") defines no __cpp_lib_freestanding_* macros";
#elif defined(__GLIBCXX__)
    static_assert(__cpp_lib_freestanding_algorithm == 202311L);
    static_assert(__cpp_lib_freestanding_array == 202311L);
    static_assert(__cpp_lib_freestanding_cstring == 202311L);
    static_assert(__cpp_lib_freestanding_optional == 202311L);
    static_assert(__cpp_lib_freestanding_string_view == 202311L);
    static_assert(__cpp_lib_freestanding_variant == 202311L);
#if defined(__cpp_lib_freestanding_memory) || defined(__cpp_lib_freestanding_feature_test_macros)
    FAIL() << "libstdc++ now defines more freestanding macros than the page lists";
#endif
#ifdef __cpp_lib_freestanding_expected
    static_assert(__cpp_lib_freestanding_expected == 202311L);
#else
    // libstdc++ defines it only where <expected> is enabled (__cpp_concepts >= 202002L).
    GTEST_SKIP() << "__cpp_lib_freestanding_expected not defined: libstdc++ gates it on "
                    "std::expected, which needs __cpp_concepts >= 202002L (Clang 18 has "
                 << __cpp_concepts << "L); the other six macros checked";
#endif
#elif defined(_LIBCPP_VERSION)
#if defined(__cpp_lib_freestanding_algorithm) || defined(__cpp_lib_freestanding_array) || \
    defined(__cpp_lib_freestanding_optional) || defined(__cpp_lib_freestanding_feature_test_macros)
    FAIL() << "libc++ now defines __cpp_lib_freestanding_* macros; the page says none";
#endif
#else
    GTEST_SKIP() << "page only lists libstdc++ and libc++";
#endif
}

// "Mental model": -ffreestanding sets __STDC_HOSTED__ to 0 and changes builtins.
TEST(Freestanding, FfreestandingFlag) {
    GTEST_SKIP() << "needs a build with -ffreestanding; the suite builds hosted only (checked by hand: g++-15 and clang++ 22 -ffreestanding give __STDC_HOSTED__ 0)";
}
