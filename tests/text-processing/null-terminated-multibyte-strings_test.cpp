// Checks claims made by docs/examples/text-processing/null-terminated-multibyte-strings.md.
#include <cerrno>
#include <climits>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>
#if !defined(__APPLE__)
#include <cuchar>
#endif

#include <gtest/gtest.h>

namespace {

constexpr std::size_t kInvalid = static_cast<std::size_t>(-1);
constexpr std::size_t kIncomplete = static_cast<std::size_t>(-2);
[[maybe_unused]] constexpr std::size_t kStored = static_cast<std::size_t>(-3);

const char* const kHello = "h\xC3\xA9llo \xE2\x82\xAC";  // "héllo €" in UTF-8

// Switches the C locale to a UTF-8 one for the test's lifetime and restores it after.
class Utf8Locale {
public:
    Utf8Locale() : saved_(std::setlocale(LC_ALL, nullptr)) {
        for (const char* n : {"en_US.UTF-8", "C.UTF-8", "C.utf8", "en_US.utf8"})
            if (std::setlocale(LC_ALL, n)) {
                ok_ = true;
                break;
            }
    }
    ~Utf8Locale() { std::setlocale(LC_ALL, saved_.c_str()); }
    explicit operator bool() const { return ok_; }

private:
    std::string saved_;
    bool ok_ = false;
};

#define REQUIRE_UTF8(loc) \
    if (!(loc)) GTEST_SKIP() << "no UTF-8 C locale (tried en_US.UTF-8, C.UTF-8) is installed"

}  // namespace

// "Nothing decodes until you pick a locale": programs start in "C", where MB_CUR_MAX is 1.
TEST(NullTerminatedMultibyteStrings, StartsInCLocale) {
    EXPECT_STREQ(std::setlocale(LC_ALL, nullptr), "C");
    EXPECT_EQ(static_cast<std::size_t>(MB_CUR_MAX), 1u);
}

// "Mental model" / "Nothing decodes...": "héllo €" is 10 bytes but 7 characters in UTF-8.
TEST(NullTerminatedMultibyteStrings, CountCharacters) {
    Utf8Locale loc;
    REQUIRE_UTF8(loc);
    std::mbstate_t st{};
    std::size_t chars = 0;
    std::vector<std::size_t> lens;
    for (const char *p = kHello, *end = kHello + std::strlen(kHello); p < end; ++chars) {
        std::size_t len = std::mbrlen(p, end - p, &st);
        ASSERT_NE(len, kInvalid);
        ASSERT_NE(len, kIncomplete);
        lens.push_back(len);
        p += len;
    }
    EXPECT_EQ(std::strlen(kHello), 10u);
    EXPECT_EQ(chars, 7u);
    EXPECT_EQ(lens, (std::vector<std::size_t>{1, 2, 1, 1, 1, 1, 3}));
}

// "Nothing decodes...": MB_CUR_MAX in a UTF-8 locale is 4 on macOS, 6 on glibc.
TEST(NullTerminatedMultibyteStrings, MbCurMaxInUtf8) {
    Utf8Locale loc;
    REQUIRE_UTF8(loc);
#if defined(__APPLE__)
    EXPECT_EQ(static_cast<std::size_t>(MB_CUR_MAX), 4u);
#elif defined(__GLIBC__)
    EXPECT_EQ(static_cast<std::size_t>(MB_CUR_MAX), 6u);
#else
    GTEST_SKIP() << "MB_CUR_MAX is implementation-defined; page states macOS and glibc only";
#endif
    EXPECT_LE(static_cast<long>(MB_CUR_MAX), static_cast<long>(MB_LEN_MAX));
}

// "Return values": -1 with EILSEQ for invalid input; -2 for a valid prefix, then finish it.
TEST(NullTerminatedMultibyteStrings, InvalidAndIncomplete) {
    Utf8Locale loc;
    REQUIRE_UTF8(loc);
    std::mbstate_t st{};
    EXPECT_TRUE(std::mbsinit(&st));  // a zeroed state is the initial state
    errno = 0;
    EXPECT_EQ(std::mbrlen("\xFF", 1, &st), kInvalid);
    EXPECT_EQ(errno, EILSEQ);

    st = {};  // state is unspecified after an error: reset it
    const char* euro = "\xE2\x82\xAC";
    EXPECT_EQ(std::mbrlen(euro, 2, &st), kIncomplete);  // split across a buffer boundary
    EXPECT_FALSE(std::mbsinit(&st));
    EXPECT_EQ(std::mbrlen(euro + 2, 1, &st), 1u);  // the remaining byte completes it
    EXPECT_TRUE(std::mbsinit(&st));
    EXPECT_EQ(std::mbrlen("", 1, &st), 0u);  // 0 means the null character
}

#if !defined(__APPLE__)
// "Decode to code points and encode back": U+0068 ... U+20AC, and € back to E2 82 AC.
TEST(NullTerminatedMultibyteStrings, Mbrtoc32AndC32rtomb) {
    Utf8Locale loc;
    REQUIRE_UTF8(loc);
    std::mbstate_t st{};
    char32_t c32;
    std::vector<char32_t> cps;
    for (const char *p = kHello, *end = kHello + std::strlen(kHello); p < end;) {
        std::size_t len = std::mbrtoc32(&c32, p, end - p, &st);
        ASSERT_LE(len, static_cast<std::size_t>(end - p));
        cps.push_back(c32);
        p += len;
    }
    EXPECT_EQ(cps, (std::vector<char32_t>{0x68, 0xE9, 0x6C, 0x6C, 0x6F, 0x20, 0x20AC}));

    char out[8];
    st = {};
    std::size_t n = std::c32rtomb(out, U'€', &st);
    ASSERT_EQ(n, 3u);
    EXPECT_EQ(std::string(out, n), "\xE2\x82\xAC");
}

#if defined(__cpp_char8_t)
// "mbrtoc8 hands out one UTF-8 code unit per call": 2/C3, then -3/A9 without input.
TEST(NullTerminatedMultibyteStrings, Mbrtoc8) {
    Utf8Locale loc;
    REQUIRE_UTF8(loc);
    const char* e_acute = "\xC3\xA9";
    std::mbstate_t st{};
    char8_t c8;
    EXPECT_EQ(std::mbrtoc8(&c8, e_acute, 2, &st), 2u);
    EXPECT_EQ(c8, 0xC3);
    EXPECT_EQ(std::mbrtoc8(&c8, e_acute + 2, 0, &st), kStored);
    EXPECT_EQ(c8, 0xA9);
}
#endif

// "Gotchas": mbrtoc16 needs two calls outside the BMP; the second returns -3.
TEST(NullTerminatedMultibyteStrings, Mbrtoc16SurrogatePair) {
    Utf8Locale loc;
    REQUIRE_UTF8(loc);
    const char* grin = "\xF0\x9F\x98\x80";  // U+1F600
    std::mbstate_t st{};
    char16_t c16;
    EXPECT_EQ(std::mbrtoc16(&c16, grin, 4, &st), 4u);
    EXPECT_EQ(c16, 0xD83D);
    EXPECT_EQ(std::mbrtoc16(&c16, grin + 4, 0, &st), kStored);
    EXPECT_EQ(c16, 0xDE00);
}

// "Gotchas": in the "C" locale, mbrtoc32 on "\xC3\xA9" returns (size_t)-1 on glibc.
TEST(NullTerminatedMultibyteStrings, CLocaleRejectsMultibyte) {
#if defined(__GLIBC__)
    ASSERT_STREQ(std::setlocale(LC_ALL, nullptr), "C");
    std::mbstate_t st{};
    char32_t c32;
    EXPECT_EQ(std::mbrtoc32(&c32, "\xC3\xA9", 2, &st), kInvalid);
#else
    GTEST_SKIP() << "page states this result for glibc only";
#endif
}
#else
// "Gotchas": <cuchar> is missing on macOS (no <uchar.h> in the SDK).
TEST(NullTerminatedMultibyteStrings, CucharExamples) {
    GTEST_SKIP() << "macOS SDK has no <uchar.h>, so mbrtoc32/c32rtomb/mbrtoc8/mbrtoc16 "
                    "aren't declared (checked with GCC 16 on Linux)";
}
#endif

// "Gotchas": setlocale returns nullptr when a name isn't installed.
TEST(NullTerminatedMultibyteStrings, UnknownLocaleNameReturnsNull) {
    std::string saved = std::setlocale(LC_ALL, nullptr);
    EXPECT_EQ(std::setlocale(LC_ALL, "xx_NOT_A_LOCALE.UTF-8"), nullptr);
    EXPECT_EQ(saved, std::setlocale(LC_ALL, nullptr));  // and leaves the locale unchanged
}
