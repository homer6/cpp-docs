// Checks claims made by docs/examples/text-processing/charconv.md.
#include <charconv>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Number → text": to_chars writes "42" with no terminator; fixed precision 2 gives "3.14".
TEST(Charconv, ToChars) {
    char buf[32];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof buf, 42);
    ASSERT_EQ(ec, std::errc{});
    EXPECT_EQ((std::string_view{buf, ptr}), "42");
    EXPECT_EQ(ptr - buf, 2);  // only the digits were written

    auto r = std::to_chars(buf, buf + sizeof buf, 3.14159, std::chars_format::fixed, 2);
    ASSERT_EQ(r.ec, std::errc{});
    EXPECT_EQ((std::string_view{buf, r.ptr}), "3.14");
}

// "Text → number (and tokenizing)": the loop yields 123, 456, 789.
TEST(Charconv, FromCharsTokenizing) {
    std::string_view s = "123 456 789";
    std::vector<int> values;
    int value;
    const char* p = s.data();
    const char* end = s.data() + s.size();
    while (p < end) {
        auto [next, ec] = std::from_chars(p, end, value);
        if (ec != std::errc{}) break;
        values.push_back(value);
        p = next;
        while (p < end && *p == ' ') ++p;
    }
    EXPECT_EQ(values, (std::vector<int>{123, 456, 789}));
}

// "Gotchas": no leading whitespace or '+' for integers, but '-' is accepted.
TEST(Charconv, FromCharsLeadingCharacters) {
    int v = 0;
    std::string_view ws = " 7", plus = "+7", minus = "-7";
    EXPECT_EQ(std::from_chars(ws.data(), ws.data() + ws.size(), v).ec, std::errc::invalid_argument);
    EXPECT_EQ(std::from_chars(plus.data(), plus.data() + plus.size(), v).ec,
              std::errc::invalid_argument);
    EXPECT_EQ(std::from_chars(minus.data(), minus.data() + minus.size(), v).ec, std::errc{});
    EXPECT_EQ(v, -7);
}

// "Gotchas": locale-independent — "1,234" parses as 1 and stops at the comma.
TEST(Charconv, LocaleIndependent) {
    std::string_view s = "1,234";
    int v = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    EXPECT_EQ(ec, std::errc{});
    EXPECT_EQ(v, 1);
    EXPECT_EQ(*ptr, ',');
}

// "Mental model": from_chars on "123abc" reads 123 and leaves ptr at "abc";
// to_chars of 255 writes "255" with ptr one past.
TEST(Charconv, MentalModel) {
    std::string_view s = "123abc";
    int v = 0;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    EXPECT_EQ(ec, std::errc{});
    EXPECT_EQ(v, 123);
    EXPECT_EQ((std::string_view{p, s.data() + s.size()}), "abc");

    char buf[8];
    auto r = std::to_chars(buf, buf + sizeof buf, 255);
    EXPECT_EQ(r.ec, std::errc{});
    EXPECT_EQ(r.ptr, buf + 3);
    EXPECT_EQ((std::string_view{buf, r.ptr}), "255");
}

// "Signatures": the result structs hold ptr + ec; integers take a base.
TEST(Charconv, ResultStructsAndBase) {
    static_assert(std::is_same_v<decltype(std::to_chars_result::ptr), char*>);
    static_assert(std::is_same_v<decltype(std::from_chars_result::ptr), const char*>);
    static_assert(std::is_same_v<decltype(std::to_chars_result::ec), std::errc>);

    char buf[16];
    auto r = std::to_chars(buf, buf + sizeof buf, 255, 16);
    EXPECT_EQ((std::string_view{buf, r.ptr}), "ff");
    std::string_view bin = "1010";
    int v = 0;
    std::from_chars(bin.data(), bin.data() + bin.size(), v, 2);
    EXPECT_EQ(v, 10);
}

// "Signatures": floats take a chars_format (scientific, fixed, hex, general) plus precision.
TEST(Charconv, FloatFormats) {
    char buf[64];
    auto text = [&](std::to_chars_result r) { return std::string_view{buf, r.ptr}; };
    EXPECT_EQ(text(std::to_chars(buf, buf + sizeof buf, 1500.0, std::chars_format::scientific)),
              "1.5e+03");
    EXPECT_EQ(text(std::to_chars(buf, buf + sizeof buf, 1.5, std::chars_format::fixed)), "1.5");
    EXPECT_EQ(text(std::to_chars(buf, buf + sizeof buf, 1.0, std::chars_format::hex)), "1p+0");
    EXPECT_EQ(text(std::to_chars(buf, buf + sizeof buf, 0.25, std::chars_format::general)), "0.25");
    EXPECT_EQ(text(std::to_chars(buf, buf + sizeof buf, 3.14159, std::chars_format::scientific, 2)),
              "3.14e+00");
}

// "Reading the result": invalid_argument, result_out_of_range, value_too_large.
TEST(Charconv, ErrorCodes) {
    int v = 99;
    std::string_view bad = "abc", big = "99999999999999999999";
    auto r1 = std::from_chars(bad.data(), bad.data() + bad.size(), v);
    EXPECT_EQ(r1.ec, std::errc::invalid_argument);
    EXPECT_EQ(r1.ptr, bad.data());
    auto r2 = std::from_chars(big.data(), big.data() + big.size(), v);
    EXPECT_EQ(r2.ec, std::errc::result_out_of_range);
    EXPECT_EQ(r2.ptr, big.data() + big.size());  // still past the consumed digits
    EXPECT_EQ(v, 99);                            // value untouched on failure

    char small[2];
    auto r3 = std::to_chars(small, small + sizeof small, 12345);
    EXPECT_EQ(r3.ec, std::errc::value_too_large);
    EXPECT_EQ(r3.ptr, small + sizeof small);
}

// "Gotchas": locale-independent float parsing — '.' is the separator, always.
// "Float support landed later": skip if the library lacks floating-point from_chars.
TEST(Charconv, FloatFromChars) {
    CPPDOCS_REQUIRE(__cpp_lib_to_chars);
#ifdef __cpp_lib_to_chars
    std::string_view s = "3.25";
    double d = 0;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), d);
    EXPECT_EQ(ec, std::errc{});
    EXPECT_EQ(d, 3.25);
    EXPECT_EQ(p, s.data() + s.size());
#endif
}
