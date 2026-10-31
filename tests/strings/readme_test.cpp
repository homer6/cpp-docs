// Checks claims made by docs/examples/strings/README.md.
#include <string>
#include <string_view>
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// Table: std::string and char_traits come from <string>; string_view from <string_view> (C++17).
TEST(StringsReadme, HeadersAndSince) {
    static_assert(std::is_same_v<std::string, std::basic_string<char>>);
    static_assert(std::is_same_v<std::string::traits_type, std::char_traits<char>>);
    static_assert(std::is_same_v<std::string_view, std::basic_string_view<char>>);
#ifdef __cpp_lib_string_view
    static_assert(__cpp_lib_string_view >= 201606L);  // C++17
#endif
    CPPDOCS_REQUIRE(__cpp_lib_string_view);
}

// "Which one?": std::string owns and mutates; a string_view reads without copying.
TEST(StringsReadme, OwnVersusView) {
    std::string s = "hello";
    s += " world";
    EXPECT_EQ(s, "hello world");

    std::string_view v = s;
    EXPECT_EQ(v.data(), s.data());  // no allocation, no copy
    EXPECT_EQ(v.size(), s.size());
}

// "Which one?": different comparison rules come from a basic_string<CharT, Traits> instantiation.
struct reverse_order_traits : std::char_traits<char> {
    static bool lt(char a, char b) { return b < a; }
    static int compare(const char* a, const char* b, std::size_t n) {
        for (; n--; ++a, ++b) {
            if (lt(*a, *b)) return -1;
            if (lt(*b, *a)) return 1;
        }
        return 0;
    }
};

TEST(StringsReadme, CustomTraitsChangeComparison) {
    using rstring = std::basic_string<char, reverse_order_traits>;
    EXPECT_LT(std::string("a"), std::string("b"));
    EXPECT_LT(rstring("b"), rstring("a"));
    static_assert(!std::is_same_v<rstring, std::string>);
}
