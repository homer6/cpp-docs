// Checks claims made by docs/examples/strings/string_view.md.
#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

// "Cheap slicing": substr returns views into the same buffer; remove_prefix moves the start.
TEST(StringView, CheapSlicing) {
    std::string_view sv = "key=value";
    auto eq = sv.find('=');
    std::string_view key = sv.substr(0, eq);
    std::string_view val = sv.substr(eq + 1);
    EXPECT_EQ(key, "key");
    EXPECT_EQ(val, "value");
    EXPECT_EQ(key.data(), sv.data());          // no allocation: same buffer
    EXPECT_EQ(val.data(), sv.data() + eq + 1);

    const char* before = sv.data();
    sv.remove_prefix(4);
    EXPECT_EQ(sv, "value");
    EXPECT_EQ(sv.data(), before + 4);          // just moves the pointer
}

// "Same read API as string": starts_with / ends_with (C++20), contains (C++23).
TEST(StringView, ReadApi) {
    std::string_view sv = "value";
    EXPECT_EQ(sv.size(), 5u);
    EXPECT_EQ(sv.front(), 'v');
    EXPECT_EQ(sv.back(), 'e');
    EXPECT_TRUE(sv.starts_with("val"));
    EXPECT_TRUE(sv.ends_with("ue"));
    EXPECT_TRUE(sv.contains("alu"));
}

// "Gotchas": not null-terminated — a sub-view's data() runs on past its end.
TEST(StringView, NotNullTerminated) {
    std::string_view key = std::string_view{"key=value"}.substr(0, 3);
    EXPECT_EQ(key.size(), 3u);
    EXPECT_EQ(key.data()[3], '=');             // no terminator after "key"
    EXPECT_EQ(std::string(key).c_str()[3], '\0');  // copy into a string to get one
}

// "Gotchas": remove_prefix/remove_suffix shrink the view, not the data.
TEST(StringView, RemoveSuffixLeavesDataAlone) {
    std::string s = "hello world";
    std::string_view sv = s;
    sv.remove_suffix(6);
    EXPECT_EQ(sv, "hello");
    EXPECT_EQ(s, "hello world");
}

// Declaration / aliases: basic_string_view<CharT, Traits = char_traits<CharT>> and its five aliases.
TEST(StringView, Aliases) {
    static_assert(std::is_same_v<std::string_view, std::basic_string_view<char, std::char_traits<char>>>);
    static_assert(std::is_same_v<std::wstring_view, std::basic_string_view<wchar_t>>);
    static_assert(std::is_same_v<std::u8string_view, std::basic_string_view<char8_t>>);
    static_assert(std::is_same_v<std::u16string_view, std::basic_string_view<char16_t>>);
    static_assert(std::is_same_v<std::u32string_view, std::basic_string_view<char32_t>>);
}

// "Mental model": a view of a std::string is {ptr -> 'h', len = 11}; copying copies no characters.
TEST(StringView, PointerPlusLength) {
    std::string owner = "hello world";
    std::string_view sv = owner;
    EXPECT_EQ(sv.data(), owner.data());
    EXPECT_EQ(sv.size(), 11u);
    std::string_view copy = sv;
    EXPECT_EQ(copy.data(), owner.data());
    static_assert(std::is_trivially_copyable_v<std::string_view>);
}

// "Why it exists": one count_spaces signature takes literals, std::string, char* and views.
static std::size_t count_spaces(std::string_view s) { return std::ranges::count(s, ' '); }

TEST(StringView, UniversalParameter) {
    EXPECT_EQ(count_spaces("a b c"), 2u);
    EXPECT_EQ(count_spaces(std::string{"x y"}), 1u);
    char buf[] = "p q r s";
    char* ptr = buf;
    EXPECT_EQ(count_spaces(ptr), 3u);
    EXPECT_EQ(count_spaces(std::string_view{"t u"}), 1u);
    static_assert(std::is_convertible_v<const std::string&, std::string_view>);
    static_assert(std::is_convertible_v<const char*, std::string_view>);
}

// "Same read API as string": empty, operator[], range-for.
TEST(StringView, ReadApiRest) {
    std::string_view sv = "value";
    EXPECT_FALSE(sv.empty());
    EXPECT_EQ(sv[0], 'v');
    std::string seen;
    for (char ch : sv) seen += ch;
    EXPECT_EQ(seen, "value");
}

// "Gotchas": there is no c_str(); it's read-only.
template <class T>
concept has_c_str = requires(T t) { t.c_str(); };
TEST(StringView, NoCStrAndReadOnly) {
    static_assert(!has_c_str<std::string_view>);
    static_assert(has_c_str<std::string>);
    static_assert(std::is_same_v<decltype(std::declval<std::string_view&>()[0]), const char&>);
}
