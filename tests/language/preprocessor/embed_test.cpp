// Checks claims made by docs/examples/language/preprocessor/embed.md.
// Data files live next to this file in embed_data/ (hello.txt = "Hello", empty.bin,
// ff.bin = two 0xFF bytes); quote-form #embed searches relative to this file.
#include <initializer_list>
#include <type_traits>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// Clang 22 accepts #embed in C++ as an extension and warns (-Wc23-extensions);
// GCC 15/16 implement the C++26 feature.
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wc23-extensions"
#endif

#if defined(__has_embed)
#define EMBED_TEST_OK 1
#define SKIP_WITHOUT_EMBED() (void)0
#else
#define SKIP_WITHOUT_EMBED() GTEST_SKIP() << "needs #embed / __has_embed (not supported by this toolchain)"
#endif

#ifdef EMBED_TEST_OK
namespace {
constexpr unsigned char hello[] = {
#embed "embed_data/hello.txt"
};
constexpr char text[] = {
#embed "embed_data/hello.txt" suffix(, 0)
};
constexpr unsigned char magic[] = {
#embed "embed_data/hello.txt" limit(2)
};
constexpr unsigned char middle[] = {
#if defined(__clang__)
#embed "embed_data/hello.txt" clang::offset(1) limit(2)
#else
#embed "embed_data/hello.txt" gnu::offset(1) limit(2)
#endif
};
constexpr unsigned char empty_data[] = {
#embed "embed_data/empty.bin" prefix(0xAA, ) suffix(, 0xBB) if_empty(0xFF)
};
constexpr unsigned char hello_data[] = {
#embed "embed_data/hello.txt" prefix(0xAA, ) suffix(, 0xBB) if_empty(0xFF)
};

#if __has_embed("embed_data/config.json") == __STDC_EMBED_FOUND__
#define HAVE_CONFIG 1
#endif
#ifdef HAVE_CONFIG
constexpr char config[] = {
#embed "embed_data/config.json" suffix(, 0)
};
#else
constexpr char config[] = "{}";
#endif

constexpr int embed_test_n = 2;
}  // namespace
#endif

// "Examples": bytes into an array, suffix(, 0) for a terminator, limit(2), vendor offset.
TEST(Embed, BytesSuffixLimitOffset) {
    SKIP_WITHOUT_EMBED();
#ifdef EMBED_TEST_OK
    static_assert(sizeof hello == 5 && hello[0] == 'H');
    static_assert(sizeof text == 6 && text[5] == '\0');
    EXPECT_STREQ(text, "Hello");
    static_assert(sizeof magic == 2 && magic[1] == 'e');
    static_assert(sizeof middle == 2 && middle[0] == 'e' && middle[1] == 'l');  // offset, then limit
#endif
}

// "Mental model" / "Gotchas": each element is an int literal, not a byte.
TEST(Embed, ElementsAreIntLiterals) {
    SKIP_WITHOUT_EMBED();
#ifdef EMBED_TEST_OK
#if defined(__clang__)
    constexpr char plain[] = {  // 0xFF into signed char: GCC rejects (narrowing), Clang accepts
#embed "embed_data/ff.bin"
    };
    EXPECT_EQ(sizeof plain, 2u);
    EXPECT_EQ(plain[0], static_cast<char>(0xFF));
#endif
#if defined(__clang__) && __clang_major__ < 21
    // Apple Clang 17 is LLVM 19, so the same check covers it; Apple Clang 21 accepts it.
    GTEST_SKIP() << "Clang 19/20 count an #embed in a braced list as one element when deducing "
                    "std::initializer_list (\"excess elements in array initializer\"; "
                    "llvm/llvm-project#122306, fixed in Clang 21)";
#else
    auto il = {
#embed "embed_data/hello.txt"
    };
    static_assert(std::is_same_v<decltype(il), std::initializer_list<int>>);
    EXPECT_EQ(il.size(), 5u);
    EXPECT_EQ(*il.begin(), 72);  // 'H'
#endif
#endif
}

// "if_empty, prefix, suffix": prefix/suffix only for non-empty, if_empty only for empty.
TEST(Embed, IfEmptyPrefixSuffix) {
    SKIP_WITHOUT_EMBED();
#ifdef EMBED_TEST_OK
    static_assert(sizeof empty_data == 1 && empty_data[0] == 0xFF);
    static_assert(sizeof hello_data == 7 && hello_data[0] == 0xAA && hello_data[1] == 'H' &&
                  hello_data[6] == 0xBB);
#endif
}

// "Declaration": __has_embed returns NOT_FOUND (0), FOUND (1) or EMPTY (2).
TEST(Embed, HasEmbedValues) {
    SKIP_WITHOUT_EMBED();
#ifdef EMBED_TEST_OK
    static_assert(__STDC_EMBED_NOT_FOUND__ == 0 && __STDC_EMBED_FOUND__ == 1 &&
                  __STDC_EMBED_EMPTY__ == 2);
#if !(__has_embed("embed_data/hello.txt") == __STDC_EMBED_FOUND__)
    ADD_FAILURE() << "expected: __has_embed(\"embed_data/hello.txt\") == __STDC_EMBED_FOUND__";
#endif
#if !(__has_embed("embed_data/empty.bin") == __STDC_EMBED_EMPTY__)
    ADD_FAILURE() << "expected: __has_embed(\"embed_data/empty.bin\") == __STDC_EMBED_EMPTY__";
#endif
#if !(__has_embed("embed_data/no-such-file.bin") == __STDC_EMBED_NOT_FOUND__)
    ADD_FAILURE() << "expected: __has_embed(\"embed_data/no-such-file.bin\") == __STDC_EMBED_NOT_FOUND__";
#endif
#if !(__has_embed("embed_data/hello.txt" limit(0)) == __STDC_EMBED_EMPTY__)
    ADD_FAILURE() << "expected: __has_embed(\"embed_data/hello.txt\" limit(0)) == __STDC_EMBED_EMPTY__";
#endif
    // "limit(0) still needs the file to exist"
#if !(__has_embed("embed_data/no-such-file.bin" limit(0)) == __STDC_EMBED_NOT_FOUND__)
    ADD_FAILURE() << "expected: __has_embed(\"embed_data/no-such-file.bin\" limit(0)) == __STDC_EMBED_NOT_FOUND__";
#endif
    // an unsupported parameter makes __has_embed return 0
#if !(__has_embed("embed_data/hello.txt" vendor::no_such_param(1)) == 0)
    ADD_FAILURE() << "expected: __has_embed(\"embed_data/hello.txt\" vendor::no_such_param(1)) == 0";
#endif
    // "Optional resources": config.json is absent, so the fallback is used
    static_assert(sizeof config == 3);
    EXPECT_STREQ(config, "{}");
#endif
}

// "Gotchas": limit(n) with a constexpr variable is an unknown identifier, i.e. limit(0).
TEST(Embed, LimitWithConstexprVariableIsZero) {
    SKIP_WITHOUT_EMBED();
#ifdef EMBED_TEST_OK
    static_assert(embed_test_n == 2);
#if !(__has_embed("embed_data/hello.txt" limit(embed_test_n)) == __STDC_EMBED_EMPTY__)
    ADD_FAILURE() << "expected: __has_embed(\"embed_data/hello.txt\" limit(embed_test_n)) == __STDC_EMBED_EMPTY__";
#endif
#endif
}

// "Compiler support (tested)": __cpp_pp_embed, and the standard offset (C++29) not yet accepted.
TEST(Embed, CompilerSupport) {
#if defined(__clang__)
#ifdef __cpp_pp_embed
    ADD_FAILURE() << "page says Clang 22 doesn't define __cpp_pp_embed";
#endif
#elif defined(__GNUC__)
    CPPDOCS_REQUIRE(__cpp_pp_embed);  // GCC 15 adds #embed; GCC 13/14 don't define the macro
#ifdef __cpp_pp_embed
    EXPECT_EQ(__cpp_pp_embed, 202502L);
#endif
#endif
#if defined(__has_embed) && ((defined(__clang__) && __clang_major__ <= 22) || \
                             (!defined(__clang__) && defined(__GNUC__) && __GNUC__ <= 16))
#if !(__has_embed("embed_data/hello.txt" offset(1)) == __STDC_EMBED_NOT_FOUND__)
    ADD_FAILURE() << "expected: __has_embed(\"embed_data/hello.txt\" offset(1)) == __STDC_EMBED_NOT_FOUND__";
#endif
#if defined(__clang__)
#if !(__has_embed("embed_data/hello.txt" clang::offset(1)) == __STDC_EMBED_FOUND__)
    ADD_FAILURE() << "expected: __has_embed(\"embed_data/hello.txt\" clang::offset(1)) == __STDC_EMBED_FOUND__";
#endif
#else
#if !(__has_embed("embed_data/hello.txt" gnu::offset(1)) == __STDC_EMBED_FOUND__)
    ADD_FAILURE() << "expected: __has_embed(\"embed_data/hello.txt\" gnu::offset(1)) == __STDC_EMBED_FOUND__";
#endif
#endif
#else
    GTEST_SKIP() << "the offset-support claim covers GCC 15/16 and Clang 22 only";
#endif
}

#undef SKIP_WITHOUT_EMBED
