// Checks claims made by docs/examples/utilities/expected.md.
#include <charconv>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <version>

#ifdef __cpp_lib_expected
#include <expected>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

#ifdef __cpp_lib_expected
namespace {
enum class Err { empty, not_a_number, overflow, negative };

std::expected<int, Err> to_int(std::string_view s) {
    if (s.empty()) return std::unexpected{Err::empty};
    int value;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec == std::errc::invalid_argument) return std::unexpected{Err::not_a_number};
    if (ec == std::errc::result_out_of_range) return std::unexpected{Err::overflow};
    return value;
}

std::expected<int, Err> check_positive(int x) {
    if (x < 0) return std::unexpected{Err::negative};
    return x;
}
int square(int x) { return x * x; }
std::string to_message(Err e) { return "error " + std::to_string(static_cast<int>(e)); }

std::expected<int, std::string> parse_and_square(std::string_view text) {
    return to_int(text).and_then(check_positive).transform(square).transform_error(to_message);
}
}  // namespace
#endif

// "Returning success or error": each failure path returns its own error.
TEST(Expected, ReturningSuccessOrError) {
    CPPDOCS_REQUIRE(__cpp_lib_expected);
#ifdef __cpp_lib_expected
    EXPECT_EQ(to_int("42"), 42);
    EXPECT_EQ(to_int("").error(), Err::empty);
    EXPECT_EQ(to_int("abc").error(), Err::not_a_number);
    EXPECT_EQ(to_int("99999999999999999999").error(), Err::overflow);
#endif
}

// "Consuming the result": truthy on success; value() throws bad_expected_access<E>; value_or; error().
TEST(Expected, ConsumingTheResult) {
    CPPDOCS_REQUIRE(__cpp_lib_expected);
#ifdef __cpp_lib_expected
    auto r = to_int("42");
    ASSERT_TRUE(r);
    EXPECT_EQ(*r, 42);
    EXPECT_EQ(r.value(), 42);
    EXPECT_EQ(r.value_or(0), 42);

    auto bad = to_int("x");
    EXPECT_FALSE(bad);
    EXPECT_EQ(bad.value_or(0), 0);
    EXPECT_EQ(bad.error(), Err::not_a_number);
    EXPECT_THROW((void)bad.value(), std::bad_expected_access<Err>);
    try {
        (void)bad.value();
    } catch (const std::bad_expected_access<Err>& e) {
        EXPECT_EQ(e.error(), Err::not_a_number);  // the exception carries the error
    }
#endif
}

// "Monadic chaining": and_then / transform run on success; transform_error maps the error.
TEST(Expected, MonadicChaining) {
    CPPDOCS_REQUIRE(__cpp_lib_expected);
#ifdef __cpp_lib_expected
    EXPECT_EQ(parse_and_square("7"), 49);
    EXPECT_EQ(parse_and_square("-5").error(), "error 3");  // Err::negative
    EXPECT_EQ(parse_and_square("").error(), "error 0");    // Err::empty
#endif
}

// "Gotchas": std::unexpected picks the error path even when T and E are the same type.
TEST(Expected, UnexpectedSelectsErrorPath) {
    CPPDOCS_REQUIRE(__cpp_lib_expected);
#ifdef __cpp_lib_expected
    std::expected<int, int> ok = 5;
    std::expected<int, int> err = std::unexpected{5};
    EXPECT_TRUE(ok.has_value());
    EXPECT_FALSE(err.has_value());
    EXPECT_EQ(err.error(), 5);
#endif
}

// "Gotchas": expected<void, E> is valid — success with no payload, or an error.
TEST(Expected, VoidValueType) {
    CPPDOCS_REQUIRE(__cpp_lib_expected);
#ifdef __cpp_lib_expected
    std::expected<void, Err> ok;
    std::expected<void, Err> err = std::unexpected{Err::empty};
    EXPECT_TRUE(ok.has_value());
    EXPECT_FALSE(err.has_value());
    EXPECT_EQ(err.error(), Err::empty);
    EXPECT_THROW(err.value(), std::bad_expected_access<Err>);
#endif
}
