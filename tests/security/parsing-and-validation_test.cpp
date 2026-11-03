// Checks claims made by docs/security/parsing-and-validation.md.
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

std::optional<std::uint16_t> parse_port(std::string_view text) {
    if (text.size() > 1 && text.front() == '0') return std::nullopt;   // "080" and "80" mustn't both mean 80
    unsigned value = 0;
    const char* const last = std::to_address(text.end());
    auto [end, ec] = std::from_chars(std::to_address(text.begin()), last, value);
    if (ec != std::errc{}) return std::nullopt;
    if (end != last) return std::nullopt;
    if (value == 0 || value > 65535) return std::nullopt;
    return static_cast<std::uint16_t>(value);
}

std::optional<std::map<std::string, std::string>>
collect(const std::vector<std::pair<std::string_view, std::string_view>>& fields) {
    std::map<std::string, std::string> out;
    for (auto [name, value] : fields)
        if (!out.emplace(std::string{name}, std::string{value}).second) return std::nullopt;
    return out;
}

std::optional<std::size_t> nesting(std::string_view s, std::size_t max_depth) {
    std::size_t depth = 0, deepest = 0;
    for (char c : s) {
        if (c == '(') {
            if (++depth > max_depth) return std::nullopt;
            if (depth > deepest) deepest = depth;
        } else if (c == ')') {
            if (depth == 0) return std::nullopt;
            --depth;
        } else {
            return std::nullopt;
        }
    }
    if (depth != 0) return std::nullopt;
    return deepest;
}

}  // namespace

// "Parse into constrained values": from_chars plus a full-consumption check and a domain
// check accept a port and reject trailing data, signs, whitespace and out-of-range values.
TEST(SecurityParsing, ParsePortRejectsEverythingButAPort) {
    EXPECT_EQ(parse_port("8080"), 8080);
    EXPECT_EQ(parse_port("65535"), 65535);
    EXPECT_FALSE(parse_port(""));
    EXPECT_FALSE(parse_port("8080abc"));   // trailing data
    EXPECT_FALSE(parse_port(" 80"));       // leading whitespace
    EXPECT_FALSE(parse_port("+80"));       // from_chars takes no '+'
    EXPECT_FALSE(parse_port("-1"));        // or '-' for unsigned
    EXPECT_FALSE(parse_port("0"));         // outside the domain
    EXPECT_FALSE(parse_port("080"));       // leading zero: a second spelling of 80 (found by fuzzing)
    EXPECT_FALSE(parse_port("05"));
    EXPECT_FALSE(parse_port("65536"));
    EXPECT_FALSE(parse_port("99999999999999999999"));   // too big even for unsigned
    EXPECT_FALSE(parse_port(std::string_view("80\0" "1", 4)));   // embedded NUL is trailing data
}

// "Define the ambiguous cases": a duplicate field is rejected instead of first- or last-wins.
TEST(SecurityParsing, DuplicateFieldsRejected) {
    auto ok = collect({{"host", "a"}, {"port", "1"}});
    ASSERT_TRUE(ok);
    EXPECT_EQ(ok->at("host"), "a");
    EXPECT_FALSE(collect({{"host", "a"}, {"host", "b"}}));
}

// "Never let input become code": input passed as an argument is printed as data, braces
// and all, never interpreted as format specifications.
TEST(SecurityParsing, InputAsFormatArgumentIsData) {
    std::string hostile = "{} {:x} {0}";
    EXPECT_EQ(std::format("hello, {}", hostile), "hello, {} {:x} {0}");
}

// "Never let input become code": a runtime format string with a bad specification throws
// format_error (so only program-controlled strings belong there).
TEST(SecurityParsing, RuntimeFormatRejectsBadSpec) {
    std::string bad = "{:q}";
    int value = 1;
    EXPECT_THROW((void)std::vformat(bad, std::make_format_args(value)), std::format_error);
}

// "Bound the parser's work": nesting is limited, and malformed input is rejected.
TEST(SecurityParsing, NestingDepthIsBounded) {
    EXPECT_EQ(nesting("(()())", 8), 2u);
    EXPECT_EQ(nesting("", 8), 0u);
    EXPECT_FALSE(nesting(std::string(100, '(') + std::string(100, ')'), 64));
    EXPECT_EQ(nesting(std::string(64, '(') + std::string(64, ')'), 64), 64u);
    EXPECT_FALSE(nesting("(()", 8));
    EXPECT_FALSE(nesting("())", 8));
    EXPECT_FALSE(nesting("(x)", 8));
}
