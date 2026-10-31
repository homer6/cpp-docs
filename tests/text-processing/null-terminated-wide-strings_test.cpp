// Checks claims made by docs/examples/text-processing/null-terminated-wide-strings.md.
#include <clocale>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <cwctype>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "support/capture.hpp"

namespace {

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

std::string narrow(const wchar_t* w) {
    std::mbstate_t st{};
    const wchar_t* src = w;
    std::size_t n = std::wcsrtombs(nullptr, &src, 0, &st);
    if (n == static_cast<std::size_t>(-1)) return {};
    std::string out(n, '\0');
    st = {};
    src = w;
    std::wcsrtombs(out.data(), &src, n, &st);
    return out;
}

}  // namespace

// "Counting and classifying characters": sizeof(wchar_t)=4 and wcslen=7 on macOS and Linux.
TEST(NullTerminatedWideStrings, SizeAndLength) {
    const wchar_t* w = L"héllo €";
#if defined(__APPLE__) || defined(__linux__)
    EXPECT_EQ(sizeof(wchar_t), 4u);
#endif
    if (sizeof(wchar_t) < 4) GTEST_SKIP() << "16-bit wchar_t: page only states macOS/Linux";
    EXPECT_EQ(std::wcslen(w), 7u);
}

// "Counting and classifying characters": C locale: iswalpha(é)=0, towupper(é)=U+00E9.
TEST(NullTerminatedWideStrings, CLocaleClassifiesOnlyAscii) {
    ASSERT_STREQ(std::setlocale(LC_ALL, nullptr), "C");
    EXPECT_EQ(std::iswalpha(L'é') != 0, false);
    EXPECT_EQ(static_cast<unsigned>(std::towupper(L'é')), 0xE9u);
    EXPECT_NE(std::iswalpha(L'a'), 0);
}

// "Counting and classifying characters": UTF-8 locale: iswalpha(é)=1, towupper(é)=U+00C9.
TEST(NullTerminatedWideStrings, Utf8LocaleClassifiesE) {
    Utf8Locale loc;
    REQUIRE_UTF8(loc);
    EXPECT_NE(std::iswalpha(L'é'), 0);
    EXPECT_EQ(static_cast<unsigned>(std::towupper(L'é')), 0xC9u);
}

// "Classification by name": wctype("digit") and wctrans("toupper").
TEST(NullTerminatedWideStrings, ClassificationByName) {
    std::wctype_t digit = std::wctype("digit");
    EXPECT_NE(digit, std::wctype_t{});
    EXPECT_NE(std::iswctype(L'7', digit), 0);
    EXPECT_EQ(std::iswctype(L'x', digit), 0);
    std::wctrans_t up = std::wctrans("toupper");
    EXPECT_EQ(std::towctrans(L'a', up), L'A');
}

// "Wide → UTF-8": narrow(L"héllo €") is 10 bytes of UTF-8.
TEST(NullTerminatedWideStrings, NarrowToUtf8) {
    Utf8Locale loc;
    REQUIRE_UTF8(loc);
    std::string s = narrow(L"héllo €");
    EXPECT_EQ(s.size(), 10u);
    EXPECT_EQ(s, "h\xC3\xA9llo \xE2\x82\xAC");
}

// "Wide → UTF-8": an unrepresentable char makes wcsrtombs return -1 (narrow gives "").
TEST(NullTerminatedWideStrings, NarrowUnrepresentable) {
    ASSERT_STREQ(std::setlocale(LC_ALL, nullptr), "C");
#if defined(__GLIBC__)
    EXPECT_EQ(narrow(L"€"), "");  // € has no encoding in glibc's ASCII "C" locale
#else
    GTEST_SKIP() << "what the C locale does with non-ASCII wide chars is libc-specific";
#endif
}

// "Declaration": wcstok takes a state pointer, so it is reentrant (interleaved tokenizing).
TEST(NullTerminatedWideStrings, WcstokIsReentrant) {
    wchar_t a[] = L"a b", b[] = L"1 2";
    wchar_t *sa = nullptr, *sb = nullptr;
    std::vector<std::wstring> out;
    out.emplace_back(std::wcstok(a, L" ", &sa));
    out.emplace_back(std::wcstok(b, L" ", &sb));
    out.emplace_back(std::wcstok(nullptr, L" ", &sa));
    out.emplace_back(std::wcstok(nullptr, L" ", &sb));
    EXPECT_EQ(out, (std::vector<std::wstring>{L"a", L"1", L"b", L"2"}));
}

// "Gotchas": WCHAR_MAX is signed 2147483647 on arm64 macOS, unsigned 4294967295 on arm64 Linux.
TEST(NullTerminatedWideStrings, WcharMax) {
#if defined(__APPLE__) && defined(__aarch64__)
    EXPECT_EQ(static_cast<std::uintmax_t>(WCHAR_MAX), 2147483647u);
#elif defined(__linux__) && defined(__aarch64__)
    EXPECT_EQ(static_cast<std::uintmax_t>(WCHAR_MAX), 4294967295u);
#else
    GTEST_SKIP() << "page only states WCHAR_MAX for arm64 macOS and arm64 Linux";
#endif
}

// "Gotchas": printf("[%ls]\n", L"héllo") in the "C" locale returns -1 and prints nothing on
// glibc, while macOS writes the raw byte 0xE9.
TEST(NullTerminatedWideStrings, PrintfLsInCLocale) {
    ASSERT_STREQ(std::setlocale(LC_ALL, nullptr), "C");
    int rc = 0;
    auto out = cppdocs::capture_stdout([&] { rc = std::printf("[%ls]\n", L"h\u00E9llo"); });
#if defined(__GLIBC__)
    EXPECT_EQ(rc, -1);
    EXPECT_EQ(out, "");
#elif defined(__APPLE__)
    EXPECT_EQ(rc, 8);
    EXPECT_EQ(out, "[h\xE9llo]\n");
#else
    GTEST_SKIP() << "page states this for glibc and macOS only";
#endif
}

// "Gotchas": towupper maps one unit to one unit, so ß stays ß.
TEST(NullTerminatedWideStrings, TowupperOneToOne) {
    Utf8Locale loc;
    REQUIRE_UTF8(loc);
    EXPECT_EQ(std::towupper(L'ß'), L'ß');
}

// "Gotchas": wcsncpy doesn't terminate when the source fills the buffer.
TEST(NullTerminatedWideStrings, WcsncpyTermination) {
    const wchar_t* src = L"hello";
    wchar_t buf[4];
    std::wcsncpy(buf, src, 4);
    EXPECT_EQ(std::wmemchr(buf, L'\0', 4), nullptr);
}
