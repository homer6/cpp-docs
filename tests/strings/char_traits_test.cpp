// Checks claims made by docs/examples/strings/char_traits.md.
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

#include <gtest/gtest.h>

// "Mental model": basic_string and basic_string_view take Traits, defaulting to char_traits<CharT>.
TEST(CharTraits, StringTypesUseTraits) {
    static_assert(std::is_same_v<std::string::traits_type, std::char_traits<char>>);
    static_assert(std::is_same_v<std::string_view::traits_type, std::char_traits<char>>);
    static_assert(std::is_same_v<std::u32string::traits_type, std::char_traits<char32_t>>);
}

// "Interface": specialized for char, wchar_t, char8_t, char16_t, char32_t.
template <class CharT>
constexpr bool has_members() {
    using T = std::char_traits<CharT>;
    return std::is_same_v<typename T::char_type, CharT> && std::is_integral_v<typename T::int_type>;
}
TEST(CharTraits, Specializations) {
    static_assert(has_members<char>());
    static_assert(has_members<wchar_t>());
    static_assert(has_members<char8_t>());
    static_assert(has_members<char16_t>());
    static_assert(has_members<char32_t>());
}

// "Interface": int_type can also hold EOF, distinct from every char value.
TEST(CharTraits, IntTypeHoldsEof) {
    using T = std::char_traits<char>;
    const auto eof = T::eof();
    for (int c = -128; c < 256; ++c) {
        auto ch = static_cast<char>(c);
        EXPECT_FALSE(T::eq_int_type(T::to_int_type(ch), eof)) << c;
        EXPECT_EQ(T::to_char_type(T::to_int_type(ch)), ch);
    }
}

// "Interface" table and "Many are constexpr": eq, lt, compare, length, find, assign, copy, move.
TEST(CharTraits, StaticMembersAreConstexpr) {
    using T = std::char_traits<char>;
    static_assert(T::eq('a', 'a') && !T::eq('a', 'b'));
    static_assert(T::lt('a', 'b'));
    static_assert(T::compare("abc", "abd", 3) < 0);
    static_assert(T::compare("abc", "abd", 2) == 0);
    static_assert(T::length("hello") == 5);
    static_assert(*T::find("hello", 5, 'l') == 'l');
    static_assert(T::find("hello", 5, 'z') == nullptr);
    static_assert([] {
        char buf[6] = {};
        T::copy(buf, "hello", 5);
        T::move(buf + 1, buf, 4);  // overlapping: "hhell"
        T::assign(buf[0], 'X');
        return T::compare(buf, "Xhell", 5) == 0;
    }());
}

// "Example: a case-insensitive string": "Hello" == "HELLO" through ci_traits.
struct ci_traits : std::char_traits<char> {
    static bool eq(char a, char b) {
        return std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
    }
    static bool lt(char a, char b) {
        return std::tolower((unsigned char)a) < std::tolower((unsigned char)b);
    }
    static int compare(const char* a, const char* b, std::size_t n) {
        for (; n--; ++a, ++b) {
            if (lt(*a, *b)) return -1;
            if (lt(*b, *a)) return 1;
        }
        return 0;
    }
};
using ci_string = std::basic_string<char, ci_traits>;

TEST(CharTraits, CaseInsensitiveString) {
    ci_string x = "Hello";
    ci_string y = "HELLO";
    EXPECT_TRUE(x == y);
    EXPECT_TRUE(ci_string("apple") < ci_string("BANANA"));
    EXPECT_FALSE(std::string("Hello") == std::string("HELLO"));  // plain string: case matters
}

// "Gotchas": a custom-traits string is a different type, needing explicit conversion.
TEST(CharTraits, CustomTraitsIsDifferentType) {
    static_assert(!std::is_same_v<ci_string, std::string>);
    static_assert(!std::is_convertible_v<std::string, ci_string>);
    static_assert(!std::is_convertible_v<ci_string, std::string>);
    std::string s = "Hello";
    ci_string c(s.data(), s.size());  // explicit conversion via pointer + length
    EXPECT_TRUE(c == "HELLO");
}

// "Gotchas": length stops at the null character — not a general size.
TEST(CharTraits, LengthStopsAtNull) {
    const char buf[] = "ab\0cd";
    EXPECT_EQ(std::char_traits<char>::length(buf), 2u);
    EXPECT_EQ(sizeof buf - 1, 5u);
}
