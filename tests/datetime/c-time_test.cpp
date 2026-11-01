// Checks claims made by docs/examples/datetime/c-time.md.
#include <chrono>
#include <cstdio>
#include <ctime>
#include <format>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

#include "support/capture.hpp"

// "Mental model": time_t is arithmetic (64-bit on the local toolchains).
TEST(CTime, TimeTIsArithmetic) {
    static_assert(std::is_arithmetic_v<std::time_t>);
    EXPECT_EQ(sizeof(std::time_t), 8u);
    EXPECT_GT(std::time(nullptr), 0);
}

// "Now, broken down and formatted": the values in the comments.
TEST(CTime, BrokenDownAndFormatted) {
    auto out = cppdocs::capture_stdout([] {
        std::time_t t = 365 * 86400;
        std::tm utc = *std::gmtime(&t);
        char buf[32];
        std::size_t n = std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &utc);
        std::printf("%s (%zu chars)\n", buf, n);
        std::printf("%d %d\n", utc.tm_year, utc.tm_mon);
        char tiny[4];
        std::printf("%zu\n", std::strftime(tiny, sizeof tiny, "%Y-%m-%d", &utc));
        std::printf("%d\n", std::difftime(std::time(nullptr), t) > 0);
    });
    EXPECT_EQ(out, "1971-01-01 00:00:00 (19 chars)\n71 0\n0\n1\n");
}

// "The same thing with <chrono>": from_time_t + format gives the same date.
TEST(CTime, ChronoBridge) {
    auto tp = std::chrono::system_clock::from_time_t(365 * 86400);
    EXPECT_EQ(std::format("{:%Y-%m-%d}", std::chrono::floor<std::chrono::days>(tp)), "1971-01-01");
}

// "Gotchas": gmtime returns the same static buffer every call.
TEST(CTime, GmtimeReturnsStaticStorage) {
    std::time_t a = 0, b = 86400;
    std::tm* first = std::gmtime(&a);
    std::tm* second = std::gmtime(&b);
    EXPECT_EQ(first, second);
    EXPECT_EQ(first->tm_mday, 2);  // overwritten by the second call
}

