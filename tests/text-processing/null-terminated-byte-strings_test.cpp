// Checks claims made by docs/examples/text-processing/null-terminated-byte-strings.md.
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "support/capture.hpp"

// "Mental model": strlen doesn't count the terminator.
TEST(NullTerminatedByteStrings, StrlenExcludesTerminator) {
    const char s[] = "hi!";
    EXPECT_EQ(std::strlen(s), 3u);
    EXPECT_EQ(sizeof s, 4u);
}

// "Examine and search": the documented printf output.
TEST(NullTerminatedByteStrings, ExamineAndSearch) {
    auto out = cppdocs::capture_stdout([] {
        const char* hay = "needle in haystack";
        std::printf("%zu\n", std::strlen(hay));
        std::printf("%s\n", std::strstr(hay, "hay"));
        std::printf("%s\n", std::strrchr(hay, 'e'));
        std::printf("%zu\n", std::strspn("123abc", "0123456789"));
        std::printf("%zu\n", std::strcspn("key=value", "="));
        std::printf("%d\n", std::strcmp("apple", "banana") < 0);
    });
    EXPECT_EQ(out, "18\nhaystack\ne in haystack\n3\n3\n1\n");
}

// "Classify characters safely": upper() via unsigned char maps ASCII letters only.
TEST(NullTerminatedByteStrings, ClassifySafely) {
    auto upper = [](std::string s) {
        std::ranges::transform(s, s.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });
        return s;
    };
    EXPECT_EQ(upper("hello, World 42"), "HELLO, WORLD 42");
}

// "Classify characters safely": char signedness is implementation-defined
// (signed on arm64 macOS, unsigned on arm64 Linux).
TEST(NullTerminatedByteStrings, CharSignedness) {
#if defined(__APPLE__) && defined(__aarch64__)
    EXPECT_EQ(CHAR_MIN, -128);
    EXPECT_TRUE(std::is_signed_v<char>);
#elif defined(__linux__) && defined(__aarch64__)
    EXPECT_EQ(CHAR_MIN, 0);
    EXPECT_FALSE(std::is_signed_v<char>);
#else
    GTEST_SKIP() << "page only states char signedness for arm64 macOS and arm64 Linux";
#endif
}

// "Convert text to numbers with error checking": strtol skips space, stops at 'x', reports ERANGE.
TEST(NullTerminatedByteStrings, Strtol) {
    char* end = nullptr;
    errno = 0;
    long v = std::strtol("  42xyz", &end, 10);
    EXPECT_EQ(v, 42);
    EXPECT_STREQ(end, "xyz");

    errno = 0;
    v = std::strtol("99999999999999999999", &end, 10);
    EXPECT_EQ(v, LONG_MAX);
    EXPECT_EQ(errno, ERANGE);
}

// "Convert text to numbers": atoi returns 0 when there is nothing to parse.
TEST(NullTerminatedByteStrings, AtoiReturnsZeroOnFailure) {
    EXPECT_EQ(std::atoi("abc"), 0);
}

// "memmove handles overlap": overlapping move gives "ababcd".
TEST(NullTerminatedByteStrings, Memmove) {
    char buf[] = "abcdef";
    std::memmove(buf + 2, buf, 4);
    EXPECT_STREQ(buf, "ababcd");
}

// "Gotchas": strncpy doesn't terminate when the source is >= n, and zero-pads short copies.
TEST(NullTerminatedByteStrings, StrncpyTermination) {
    const char* src = "hello";  // via a variable to avoid -Wstringop-truncation on the literal
    char buf[4];
    std::strncpy(buf, src, sizeof buf);
    EXPECT_EQ(std::memchr(buf, '\0', sizeof buf), nullptr);
    EXPECT_EQ(std::memcmp(buf, "hell", 4), 0);

    char pad[6];
    std::memset(pad, 'X', sizeof pad);
    std::strncpy(pad, "hi", sizeof pad);
    EXPECT_EQ(std::memcmp(pad, "hi\0\0\0\0", 6), 0);
}

// "Gotchas": strncat's n is the number of characters appended, not the buffer size.
TEST(NullTerminatedByteStrings, StrncatCountsAppended) {
    char buf[16] = "ab";
    std::strncat(buf, "cdef", 2);
    EXPECT_STREQ(buf, "abcd");
}

// "Gotchas": strtok modifies its input and merges adjacent delimiters ("a,b,,c" -> a b c).
TEST(NullTerminatedByteStrings, StrtokMergesDelimiters) {
    char input[] = "a,b,,c";
    std::vector<std::string> tokens;
    for (char* t = std::strtok(input, ","); t; t = std::strtok(nullptr, ",")) tokens.emplace_back(t);
    EXPECT_EQ(tokens, (std::vector<std::string>{"a", "b", "c"}));
    EXPECT_EQ(input[1], '\0');  // the first ',' was overwritten
}

// "Gotchas": strcmp returns any sign, not just -1/0/1 — only compare with zero.
TEST(NullTerminatedByteStrings, StrcmpSign) {
    EXPECT_LT(std::strcmp("apple", "banana"), 0);
    EXPECT_GT(std::strcmp("banana", "apple"), 0);
    EXPECT_EQ(std::strcmp("same", "same"), 0);
}

// "Gotchas": C++ has const and non-const overloads of strchr — except libstdc++ on macOS,
// where the C headers only declare C's char* strchr(const char*, int).
TEST(NullTerminatedByteStrings, StrchrOverloads) {
    const char* cs = "abc";
    char ms[] = "abc";
#if defined(__GLIBCXX__) && defined(__APPLE__)
    static_assert(std::is_same_v<decltype(std::strchr(cs, 'b')), char*>);
#else
    static_assert(std::is_same_v<decltype(std::strchr(cs, 'b')), const char*>);
#endif
    static_assert(std::is_same_v<decltype(std::strchr(ms, 'b')), char*>);
    EXPECT_EQ(std::strchr(cs, 'b'), cs + 1);
}

// "Declaration": cctype functions follow the C locale; in "C" only ASCII classifies.
TEST(NullTerminatedByteStrings, CctypeInCLocale) {
    EXPECT_TRUE(std::isalpha('a'));
    EXPECT_FALSE(std::isalpha(0xE9));  // é in Latin-1: not alphabetic in the "C" locale
    EXPECT_TRUE(std::isblank('\t'));
    EXPECT_FALSE(std::isblank('\n'));
}

// "Fixed-width integers in printf / scanf": the values in the comments.
TEST(NullTerminatedByteStrings, CinttypesMacros) {
    auto out = cppdocs::capture_stdout([] {
        std::int64_t big = -9'000'000'000;
        std::printf("%" PRId64 "\n", big);
        std::uint32_t u = 0;
        std::sscanf("4000000000", "%" SCNu32, &u);
        std::printf("%" PRIu32 " %" PRIx32 "\n", u, u);
        char* end = nullptr;
        errno = 0;
        std::intmax_t m = std::strtoimax("  -123abc", &end, 10);
        std::printf("%" PRIdMAX " rest='%s'\n", m, end);
    });
    EXPECT_EQ(out, "-9000000000\n4000000000 ee6b2800\n-123 rest='abc'\n");
    // int64_t is long long on macOS, long on 64-bit Linux: the macro follows it.
    if constexpr (std::is_same_v<std::int64_t, long>) {
        EXPECT_STREQ(PRId64, "ld");
    } else {
        EXPECT_STREQ(PRId64, "lld");
    }
}

// "Fixed-width integers in printf / scanf": std::format needs no macro.
TEST(NullTerminatedByteStrings, FormatReplacesMacros) {
    std::int64_t big = -9'000'000'000;
    std::uint32_t u = 4'000'000'000u;
    EXPECT_EQ(std::format("{} {:x}", big, u), "-9000000000 ee6b2800");
}
