// Checks claims made by docs/examples/io/print.md.
#include <cstdio>
#include <format>
#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <version>

#include <gtest/gtest.h>

#include "support/capture.hpp"
#include "support/feature.hpp"

// "Destinations": print adds no newline, println does, println() is a blank
// line, and the <ostream> overload writes into the stream.
TEST(Print, DestinationsOutput) {
    CPPDOCS_REQUIRE(__cpp_lib_print);
    auto out = cppdocs::capture_stdout([] {
        std::print("a={} ", 1);
        std::println("b={}", 2);
        std::println();
        std::ostringstream os;
        std::println(os, "into a stream {}", 4);
        std::print(std::cout, "{}", os.str());
    });
    EXPECT_EQ(out, "a=1 b=2\n\ninto a stream 4\n");
}

// "Unicode text": on macOS/Linux the UTF-8 bytes go out unchanged.
TEST(Print, Utf8BytesUnchanged) {
    CPPDOCS_REQUIRE(__cpp_lib_print);
    auto out = cppdocs::capture_stdout([] { std::println("héllo ✓ — {}", "naïve"); });
    EXPECT_EQ(out, "héllo ✓ — naïve\n");
}

// "Declaration" table: errors in a runtime format string passed to vprint_* throw std::format_error.
TEST(Print, VprintBadFormatThrows) {
    CPPDOCS_REQUIRE(__cpp_lib_print);
    int x = 1;
    std::string bad = "{:d";
    EXPECT_THROW(
        { cppdocs::CaptureStdout c; std::vprint_unicode(stdout, bad, std::make_format_args(x)); },
        std::format_error);
}

// "Gotchas": mixing std::print with std::cout keeps order while cout is synced (the default).
TEST(Print, MixesWithSyncedCoutInOrder) {
    CPPDOCS_REQUIRE(__cpp_lib_print);
    auto out = cppdocs::capture_stdout([] {
        std::cout << "1 ";
        std::print("2 ");
        std::cout << "3";
    });
    EXPECT_EQ(out, "1 2 3");
}

// "Compiler support": every local toolchain has at least the C++23 feature (202207L).
TEST(Print, FeatureMacroAtLeastCxx23) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_print, 202207L);
    SUCCEED();
}

// "Compiler support": the exact __cpp_lib_print value per toolchain.
TEST(Print, FeatureMacroPerToolchain) {
    CPPDOCS_REQUIRE(__cpp_lib_print);
#if defined(_LIBCPP_VERSION)
    EXPECT_EQ(__cpp_lib_print, 202207L);
#elif defined(__GNUC__) && __GNUC__ == 15
    EXPECT_EQ(__cpp_lib_print, 202211L);
#elif defined(__GNUC__) && __GNUC__ == 16
    EXPECT_EQ(__cpp_lib_print, 202406L);
#else
    GTEST_SKIP() << "value recorded only for GCC 15/16 and libc++ 22";
#endif
}

// "Your own print-like function": format_string + make_format_args + vprint_*.
namespace {
void log_impl(std::FILE* f, std::string_view fmt, std::format_args args) {
    std::fputs("[log] ", f);
    std::vprint_unicode(f, fmt, args);
    std::fputc('\n', f);
}
template <class... Args>
void log_line(std::FILE* f, std::format_string<Args...> fmt, Args&&... args) {
    log_impl(f, fmt.get(), std::make_format_args(args...));
}
}  // namespace
TEST(Print, OwnPrintLikeFunction) {
    auto out = cppdocs::capture_stdout([] { log_line(stdout, "{} items in {} ms", 42, 3.5); });
    EXPECT_EQ(out, "[log] 42 items in 3.5 ms\n");
}

// "Errors": a failed write to a FILE* throws std::system_error.
TEST(Print, FailedWriteThrowsSystemError) {
#if defined(_WIN32)
    std::FILE* ro = std::fopen("NUL", "r");  // Windows' null device
#else
    std::FILE* ro = std::fopen("/dev/null", "r");
#endif
    ASSERT_NE(ro, nullptr);
    EXPECT_THROW(std::println(ro, "x"), std::system_error);
    std::fclose(ro);
}

// "Errors": the stream overloads set badbit like operator<< instead of throwing.
TEST(Print, StreamOverloadSetsBadbit) {
    std::ostream os{nullptr};  // no buffer: every write fails
    os.clear();
    std::println(os, "{}", 1);
    EXPECT_TRUE(os.bad());
}

// "Buffered variants (C++26)": vprint_unicode_buffered (GCC 16.2).
TEST(Print, BufferedVariant) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_print, 202403L);
#if __cpp_lib_print >= 202403L
    auto out = cppdocs::capture_stdout([] {
        std::vprint_unicode_buffered(stdout, "{} {}\n", std::make_format_args("all", "at once"));
    });
    EXPECT_EQ(out, "all at once\n");
#endif
}

// "Gotchas": the runtime format-string wrapper is runtime_format in GCC 15.2,
// dynamic_format in GCC 16.2 (__cpp_lib_format 202603L); libc++ 22 has neither.
TEST(Print, RuntimeFormatWrapperSpelling) {
    std::string fmt = "<{}>";
#if defined(_LIBCPP_VERSION)
    GTEST_SKIP() << "libc++ 22 has neither runtime_format nor dynamic_format";
#elif defined(__cpp_lib_format) && __cpp_lib_format >= 202603L
    EXPECT_EQ(std::format(std::dynamic_format(fmt), 1), "<1>");
#elif defined(__cpp_lib_format) && __cpp_lib_format >= 202311L
    EXPECT_EQ(std::format(std::runtime_format(fmt), 1), "<1>");
#else
    GTEST_SKIP() << "needs __cpp_lib_format >= 202311L";
#endif
}
