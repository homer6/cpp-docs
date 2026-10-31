// Checks claims made by docs/examples/strings/string.md.
#include <cstdint>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

using namespace std::string_literals;

// Header: std::string is std::basic_string<char>.
// "The basic_string family": the alias table and the default template arguments.
TEST(String, Aliases) {
    static_assert(std::is_same_v<std::string, std::basic_string<char, std::char_traits<char>,
                                                                std::allocator<char>>>);
    static_assert(std::is_same_v<std::wstring, std::basic_string<wchar_t>>);
    static_assert(std::is_same_v<std::u8string, std::basic_string<char8_t>>);
    static_assert(std::is_same_v<std::u16string, std::basic_string<char16_t>>);
    static_assert(std::is_same_v<std::u32string, std::basic_string<char32_t>>);
    static_assert(std::is_same_v<
                  std::pmr::string,
                  std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>);
}

// "Mental model": a null terminator is always kept just past the last character.
TEST(String, NullTerminatorAlwaysKept) {
    std::string s = "hello";
    EXPECT_EQ(s.c_str()[s.size()], '\0');
    s += " world, long enough to leave any inline buffer";
    EXPECT_EQ(s.c_str()[s.size()], '\0');
    EXPECT_EQ(s.data(), s.c_str());
}

// "Mental model": SSO — short strings (typically <= 15 chars) live inside the object.
// Not mandated; libstdc++ and libc++ both do it.
TEST(String, SmallStringOptimization) {
    std::string s = "fifteen chars!!";
    ASSERT_EQ(s.size(), 15u);
    auto obj = reinterpret_cast<std::uintptr_t>(&s);
    auto buf = reinterpret_cast<std::uintptr_t>(s.data());
    EXPECT_TRUE(buf >= obj && buf < obj + sizeof s) << "15-char string is not stored inline";
    EXPECT_GE(std::string{}.capacity(), 15u);
}

// "Construction & growth": the values in the comments.
TEST(String, ConstructionAndGrowth) {
    std::string a = "hello";
    std::string b(5, '*');
    std::string c = "x"s + "y";
    std::string d{a.begin(), a.begin() + 3};
    EXPECT_EQ(b, "*****");
    EXPECT_EQ(c, "xy");
    EXPECT_EQ(d, "hel");

    a += " world";
    a.append(3, '!');
    EXPECT_EQ(a, "hello world!!!");
    a.push_back('?');
    EXPECT_EQ(a, "hello world!!!?");
}

// "Access & C interop": at() is bounds-checked; data() is non-const char* since C++17.
TEST(String, AccessAndCInterop) {
    std::string a = "hello";
    EXPECT_EQ(a[0], 'h');
    EXPECT_EQ(a.at(0), 'h');
    EXPECT_THROW((void)a.at(5), std::out_of_range);
    static_assert(std::is_same_v<decltype(a.c_str()), const char*>);
    static_assert(std::is_same_v<decltype(a.data()), char*>);
    char* raw = a.data();
    raw[0] = 'J';
    EXPECT_EQ(a, "Jello");
}

// "Searching & substrings": find returns an index or npos; substr gives "key"/"value".
TEST(String, SearchingAndSubstrings) {
    std::string s = "key=value";
    auto eq = s.find('=');
    ASSERT_EQ(eq, 3u);
    EXPECT_EQ(s.find('#'), std::string::npos);
    EXPECT_EQ(s.substr(0, eq), "key");
    EXPECT_EQ(s.substr(eq + 1), "value");
    EXPECT_TRUE(s.starts_with("key"));
    EXPECT_TRUE(s.ends_with("ue"));
    CPPDOCS_REQUIRE(__cpp_lib_string_contains);
#ifdef __cpp_lib_string_contains
    EXPECT_TRUE(s.contains("alu"));
#endif
}

// "Numeric conversions": stoi/stod/to_string; stoi throws on bad input.
TEST(String, NumericConversions) {
    EXPECT_EQ(std::stoi("42"), 42);
    EXPECT_DOUBLE_EQ(std::stod("3.14"), 3.14);
    EXPECT_EQ(std::to_string(123), "123");
    EXPECT_THROW((void)std::stoi("abc"), std::invalid_argument);
    EXPECT_THROW((void)std::stoi("99999999999999999999"), std::out_of_range);
}

// "Gotchas": embedded '\0' is allowed; size() (not strlen) is the truth.
TEST(String, EmbeddedNull) {
    std::string s("ab\0cd", 5);
    EXPECT_EQ(s.size(), 5u);
    EXPECT_EQ(std::char_traits<char>::length(s.c_str()), 2u);
}

// "Gotchas": substr copies — the result has its own buffer.
TEST(String, SubstrCopies) {
    std::string s = "a long enough string that is definitely on the heap";
    std::string sub = s.substr(2, 4);
    EXPECT_EQ(sub, "long");
    EXPECT_NE(sub.data(), s.data() + 2);
    sub[0] = 'L';
    EXPECT_EQ(s.substr(2, 4), "long");
}

// "Gotchas": "abc" is a const char*-ish literal; ""s makes a std::string.
TEST(String, StringLiteralSuffix) {
    static_assert(std::is_same_v<decltype("abc"), const char(&)[4]>);
    static_assert(std::is_same_v<decltype("abc"s), std::string>);
}

// "Gotchas": growth past capacity reallocates (invalidating pointers); within capacity it doesn't.
// "Complexity": push_back is amortized O(1) — geometric growth means few reallocations.
TEST(String, ReallocationAndGrowth) {
    std::string s;
    s.reserve(100);
    const char* p = s.data();
    for (int i = 0; i < 100; ++i) s.push_back('x');
    EXPECT_EQ(s.data(), p);  // no reallocation within reserved capacity

    s.reserve(s.capacity() + 1);
    EXPECT_NE(s.data(), p);  // growth past capacity moved the buffer

    std::string g;
    int reallocations = 0;
    for (int i = 0; i < 100000; ++i) {
        auto cap = g.capacity();
        g.push_back('x');
        if (g.capacity() != cap) ++reallocations;
    }
    EXPECT_LT(reallocations, 40);  // logarithmic, not linear
}
