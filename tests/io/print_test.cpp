// Checks claims made by docs/examples/io/print.md.
#include <cstdio>
#include <format>
#include <iostream>
#include <print>
#include <sstream>
#include <string>
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
