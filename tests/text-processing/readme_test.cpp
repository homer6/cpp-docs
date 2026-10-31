// Checks claims made by docs/examples/text-processing/README.md.
#include <charconv>
#include <clocale>
#include <format>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "support/capture.hpp"
#include "support/feature.hpp"

#ifdef __cpp_lib_print
#include <print>
#endif

// "Picking a conversion tool": format builds a string from values; print writes it straight out.
TEST(TextProcessingReadme, FormatAndPrint) {
    EXPECT_EQ(std::format("{} of {}", 3, "four"), "3 of four");
    CPPDOCS_REQUIRE(__cpp_lib_print);
#ifdef __cpp_lib_print
    EXPECT_EQ(cppdocs::capture_stdout([] { std::print("{} of {}", 3, "four"); }), "3 of four");
#endif
}

// "Picking a conversion tool": to_chars / from_chars round-trip a double exactly.
TEST(TextProcessingReadme, CharconvRoundTrips) {
    const double x = 0.1 + 0.2;
    char buf[64];
    auto r = std::to_chars(buf, buf + sizeof buf, x);
    ASSERT_EQ(r.ec, std::errc{});
    EXPECT_EQ((std::string_view{buf, r.ptr}), "0.30000000000000004");  // shortest round-trip form
    CPPDOCS_REQUIRE(__cpp_lib_to_chars);
#ifdef __cpp_lib_to_chars
    double y = 0;
    std::from_chars(buf, r.ptr, y);
    EXPECT_EQ(y, x);
#endif
}

// "Picking a conversion tool": to_string allocates (returns a std::string) and is
// locale-affected before C++26; P2587 (C++26, __cpp_lib_to_string) redefines it via format.
TEST(TextProcessingReadme, ToStringLocale) {
    static_assert(std::is_same_v<decltype(std::to_string(1.0)), std::string>);
#if defined(__cpp_lib_to_string) && __cpp_lib_to_string >= 202306L
    EXPECT_EQ(std::to_string(3.14), "3.14");  // C++26: same as std::format("{}", 3.14)
#else
    EXPECT_EQ(std::to_string(3.14), "3.140000");  // pre-C++26: sprintf("%f")
#endif
    std::string saved = std::setlocale(LC_NUMERIC, nullptr);
    if (std::setlocale(LC_NUMERIC, "de_DE.UTF-8") == nullptr)
        GTEST_SKIP() << "de_DE.UTF-8 C locale not available";
    std::string s = std::to_string(3.14);
    std::setlocale(LC_NUMERIC, saved.c_str());
#if defined(__cpp_lib_to_string) && __cpp_lib_to_string >= 202306L
    EXPECT_EQ(s, "3.14");  // locale-independent now
#else
    EXPECT_EQ(s, "3,140000");  // follows the C locale's decimal point
#endif
}
