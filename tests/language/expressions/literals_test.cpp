// Checks claims made by docs/examples/language/expressions/literals.md.
#include <algorithm>
#include <chrono>
#include <complex>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
constexpr long double operator""_km(long double v) { return v * 1000.0; }
}  // namespace

// "Numeric literals": the type and value of each form.
TEST(Literals, NumericLiteralTypesAndValues) {
    static_assert(std::is_same_v<decltype(42), int>);
    static_assert(std::is_same_v<decltype(42u), unsigned int> && std::is_same_v<decltype(42U), unsigned int>);
    static_assert(std::is_same_v<decltype(42L), long>);
    static_assert(std::is_same_v<decltype(42LL), long long>);
    static_assert(0x2A == 42 && 0b1010 == 10 && 052 == 42);
    static_assert(std::is_same_v<decltype(3.14), double>);
    static_assert(std::is_same_v<decltype(3.14f), float>);
    static_assert(std::is_same_v<decltype(3.14L), long double>);
    static_assert(1'000'000 == 1000000);
    static_assert(std::is_same_v<decltype(6.022e23), double> && std::is_same_v<decltype(1e9), double>);
    static_assert(012 == 10);  // "Gotchas": a leading 0 means octal
}

// "Character & string literals": element types and array sizes (including the '\0').
TEST(Literals, CharacterAndStringLiteralTypes) {
    static_assert(std::is_same_v<decltype('A'), char>);
    static_assert(std::is_same_v<decltype(u8'A'), char8_t>);
    static_assert(std::is_same_v<decltype(L"wide"), const wchar_t(&)[5]>);
    auto w = L"wide";
    static_assert(std::is_same_v<decltype(w), const wchar_t*>);  // auto decays the array
    static_assert(std::is_same_v<decltype(u8"utf8"), const char8_t(&)[5]>);
    static_assert(std::is_same_v<decltype(u"utf16"), const char16_t(&)[6]>);
    static_assert(std::is_same_v<decltype(U"utf32"), const char32_t(&)[6]>);
    static_assert(std::is_same_v<decltype("abc"), const char(&)[4]>);
    auto s = "abc";
    static_assert(std::is_same_v<decltype(s), const char*>);
}

// "Character & string literals": raw strings keep backslashes; custom delimiters; concatenation.
TEST(Literals, RawStringsAndConcatenation) {
    EXPECT_EQ(std::string_view(R"(raw \n no escapes)"), "raw \\n no escapes");
    EXPECT_EQ(std::string_view(R"delim(can contain )" here)delim"), "can contain )\" here");
    EXPECT_EQ(std::string_view(R"(C:\temp)"), "C:\\temp");
    EXPECT_EQ(std::string_view("foo" "bar"), "foobar");
}

// "Standard library suffix literals": s, ms, i.
TEST(Literals, StandardSuffixes) {
    using namespace std::string_literals;
    using namespace std::chrono_literals;
    using namespace std::complex_literals;
    auto s = "hello"s;
    auto t = 500ms;
    auto h = 2h;
    auto z = 1i;
    static_assert(std::is_same_v<decltype(s), std::string>);
    static_assert(std::is_same_v<decltype(t), std::chrono::milliseconds>);
    static_assert(std::is_same_v<decltype(h), std::chrono::hours>);
    static_assert(std::is_same_v<decltype(z), std::complex<double>>);
    EXPECT_EQ(s, "hello");
    EXPECT_EQ(t.count(), 500);
    EXPECT_EQ(z, std::complex<double>(0, 1));
}

// "User-defined literals": 5.0_km is 5000.0.
TEST(Literals, UserDefinedLiteral) {
    auto d = 5.0_km;
    static_assert(std::is_same_v<decltype(d), long double>);
    static_assert(5.0_km == 5000.0L);
    EXPECT_EQ(d, 5000.0L);
}

// "Numeric literals": the uz / z suffixes (C++23).
TEST(Literals, SizeTSuffix) {
    CPPDOCS_REQUIRE_VERSION(__cpp_size_t_suffix, 202011L);
#if __cpp_size_t_suffix >= 202011L  // MSVC: VS 2022 17.13 and later
    static_assert(std::is_same_v<decltype(42uz), std::size_t>);
    static_assert(std::is_same_v<decltype(42zu), std::size_t>);
    static_assert(std::is_same_v<decltype(42UZ), std::size_t>);
    static_assert(std::is_same_v<decltype(42z), std::make_signed_t<std::size_t>>);
    std::vector<int> v{1, 2, 3};
    auto smaller = std::min(v.size(), 2uz);
    EXPECT_EQ(smaller, 2uz);
#endif
}
