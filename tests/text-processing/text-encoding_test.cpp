// Checks claims made by docs/examples/text-processing/text-encoding.md.
#include <version>
#if defined(__cpp_lib_text_encoding)
#include <locale>
#include <string>
#include <string_view>
#include <text_encoding>
#include <vector>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Declaration": id::UTF8 is 106, max_name_length is 63, default is unknown with name "".
TEST(TextEncoding, DeclarationBasics) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_text_encoding, 202306L);
#ifdef __cpp_lib_text_encoding
    using te = std::text_encoding;
    static_assert(static_cast<int>(te::id::UTF8) == 106);
    static_assert(static_cast<int>(te::id::other) == 1);
    static_assert(static_cast<int>(te::id::unknown) == 2);
    static_assert(static_cast<int>(te::id::ASCII) == 3);
    static_assert(static_cast<int>(te::id::ISOLatin1) == 4);
    static_assert(te::max_name_length == 63);
    constexpr te d;
    static_assert(d.mib() == te::id::unknown);
    EXPECT_STREQ(d.name(), "");
#endif
}

// "What am I running with?": literal() is UTF-8 (the static_assert in the example holds).
TEST(TextEncoding, LiteralIsUtf8) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_text_encoding, 202306L);
#ifdef __cpp_lib_text_encoding
    constexpr auto lit = std::text_encoding::literal();
    static_assert(lit.mib() == std::text_encoding::id::UTF8, "literals must be UTF-8");
    EXPECT_EQ(lit, std::text_encoding::id::UTF8);
#endif
}

// "What am I running with?": environment()/environment_is() need -lstdc++exp with libstdc++.
TEST(TextEncoding, Environment) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_text_encoding, 202306L);
    GTEST_SKIP() << "environment() needs -lstdc++exp with libstdc++ and its result depends on "
                    "the LANG/LC_ALL the test runs with; the suite doesn't link stdc++exp";
}

// "Looking encodings up by name": the documented output.
TEST(TextEncoding, LookupByName) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_text_encoding, 202306L);
#ifdef __cpp_lib_text_encoding
    std::text_encoding e{"utf8"};
    EXPECT_STREQ(e.name(), "utf8");  // keeps the name it was created from
    EXPECT_EQ(static_cast<int>(e.mib()), 106);
    EXPECT_TRUE(e == std::text_encoding::id::UTF8);

    EXPECT_TRUE(std::text_encoding{"UTF-8"} == std::text_encoding{"csUTF8"});

    std::vector<std::string> aliases;
    for (const char* alias : std::text_encoding{std::text_encoding::id::UTF8}.aliases())
        aliases.emplace_back(alias);
    EXPECT_EQ(aliases, (std::vector<std::string>{"UTF-8", "csUTF8"}));

    std::text_encoding made_up{"x-made-up"};
    EXPECT_STREQ(made_up.name(), "x-made-up");
    EXPECT_TRUE(made_up.mib() == std::text_encoding::id::other);
#endif
}

// "Declaration": IANA name matching ignores case, punctuation and leading zeros.
TEST(TextEncoding, LooseNameMatching) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_text_encoding, 202306L);
#ifdef __cpp_lib_text_encoding
    using te = std::text_encoding;
    EXPECT_TRUE(te{"utf8"} == te{"UTF-8"});
    EXPECT_TRUE(te{"utf8"} == te{"csUTF8"});
    EXPECT_TRUE(te{"u.t.f-008"} == te::id::UTF8);  // leading zeros in a number are ignored
#endif
}

// "The encoding of a locale": classic().encoding() is US-ASCII (mib 3).
TEST(TextEncoding, LocaleEncoding) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_text_encoding, 202306L);
#if defined(__GLIBCXX__)
    GTEST_SKIP() << "std::locale::encoding() needs -lstdc++exp with libstdc++ (GCC 15 and 16 "
                    "fail to link without it); the suite doesn't link stdc++exp";
#elif defined(__cpp_lib_text_encoding)
    auto enc = std::locale::classic().encoding();
    EXPECT_TRUE(enc == std::text_encoding::id::ASCII);
#endif
}

// "Gotchas": libstdc++ defines __cpp_lib_text_encoding == 202306L in GCC 15 and 16.
TEST(TextEncoding, FeatureMacroValue) {
#if defined(__GLIBCXX__) && defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 15
    CPPDOCS_REQUIRE(__cpp_lib_text_encoding);
#ifdef __cpp_lib_text_encoding
    EXPECT_EQ(__cpp_lib_text_encoding, 202306L);
#endif
#elif defined(_LIBCPP_VERSION)
    // libc++ in Clang 22 has no <text_encoding>.
#if _LIBCPP_VERSION < 230000
#if __has_include(<text_encoding>)
    ADD_FAILURE() << "libc++ now has <text_encoding>";
#endif
#else
    GTEST_SKIP() << "page only states libc++ status for Clang 22";
#endif
#else
    GTEST_SKIP() << "page only states libstdc++ (GCC 15/16) and libc++ (Clang 22) status";
#endif
}
