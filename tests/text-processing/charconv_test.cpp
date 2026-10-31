// Checks claims made by docs/examples/text-processing/charconv.md.
#include <charconv>
#include <string_view>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

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
