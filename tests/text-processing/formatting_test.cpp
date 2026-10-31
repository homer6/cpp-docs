// Checks claims made by docs/examples/text-processing/formatting.md.
#include <format>
#include <iterator>
#include <locale>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "support/capture.hpp"
#include "support/feature.hpp"

#ifdef __cpp_lib_print
#include <print>
#endif

// "Mental model": "{} = {:.2f}" with "pi", 3.14159 gives "pi = 3.14".
TEST(Formatting, MentalModel) {
    EXPECT_EQ(std::format("{} = {:.2f}", "pi", 3.14159), "pi = 3.14");
}

// "The core functions": format_to, format_to_n, formatted_size, vformat.
TEST(Formatting, CoreFunctions) {
    static_assert(std::is_same_v<decltype(std::format("{}", 1)), std::string>);

    std::string out;
    std::format_to(std::back_inserter(out), "{}-{}", 1, 2);
    EXPECT_EQ(out, "1-2");

    char buf[4] = {'.', '.', '.', '.'};
    auto r = std::format_to_n(buf, 3, "{}", 123456);
    EXPECT_EQ(r.out, buf + 3);  // wrote at most n chars
    EXPECT_EQ(r.size, 6);       // but reports the full length
    EXPECT_EQ((std::string_view{buf, 4}), "123.");

    EXPECT_EQ(std::formatted_size("{:>8}", "hi"), 8u);

    std::string fmt = "{} and {}";  // only known at run time
    int a = 1, b = 2;
    EXPECT_EQ(std::vformat(fmt, std::make_format_args(a, b)), "1 and 2");
}

// "The core functions": a runtime format string that's malformed throws format_error from vformat.
TEST(Formatting, VformatRuntimeError) {
    std::string fmt = "{} {}";
    int a = 1;
    EXPECT_THROW((void)std::vformat(fmt, std::make_format_args(a)), std::format_error);
}

// "Format spec mini-reference": every value in the comments.
TEST(Formatting, SpecMiniReference) {
    EXPECT_EQ(std::format("{:>8}", "hi"), "      hi");
    EXPECT_EQ(std::format("{:*<8}", "hi"), "hi******");
    EXPECT_EQ(std::format("{:^8}", "hi"), "   hi   ");
    EXPECT_EQ(std::format("{:+d}", 42), "+42");
    EXPECT_EQ(std::format("{:08.3f}", 3.14), "0003.140");
    EXPECT_EQ(std::format("{:#x}", 255), "0xff");
    EXPECT_EQ(std::format("{:b}", 5), "101");
    EXPECT_EQ(std::format("{0} {1} {0}", "a", "b"), "a b a");
    EXPECT_EQ(std::format("{:{}}", "x", 5), "x    ");
}

// "Examples": format, print (no newline added), println (newline added), format_to.
TEST(Formatting, Examples) {
    EXPECT_EQ(std::format("{} items, {:.1f}% done", 3, 66.6), "3 items, 66.6% done");
    std::string buf;
    std::format_to(std::back_inserter(buf), "[{}]", 42);
    EXPECT_EQ(buf, "[42]");

    CPPDOCS_REQUIRE(__cpp_lib_print);
#ifdef __cpp_lib_print
    auto out = cppdocs::capture_stdout([] {
        std::print("Hello, {}!\n", "world");
        std::println("x = {}, y = {}", 1, 2);
    });
    EXPECT_EQ(out, "Hello, world!\nx = 1, y = 2\n");
    EXPECT_EQ(cppdocs::capture_stdout([] { std::print("no newline"); }), "no newline");
#endif
}

// "Making your own type formattable": Point prints "(3, 4)".
struct Point {
    int x, y;
};

template <>
struct std::formatter<Point> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Point& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};

TEST(Formatting, CustomFormatter) {
    EXPECT_EQ(std::format("{}", Point{3, 4}), "(3, 4)");
    CPPDOCS_REQUIRE(__cpp_lib_print);
#ifdef __cpp_lib_print
    EXPECT_EQ(cppdocs::capture_stdout([] { std::print("{}\n", Point{3, 4}); }), "(3, 4)\n");
#endif
}

// "Making your own type formattable": delegate to an existing formatter to accept its spec.
struct Meters {
    int value;
};

template <>
struct std::formatter<Meters> : std::formatter<int> {
    auto format(const Meters& m, std::format_context& ctx) const {
        auto out = std::formatter<int>::format(m.value, ctx);
        return std::format_to(out, "m");
    }
};

TEST(Formatting, DelegatingFormatter) {
    EXPECT_EQ(std::format("{:>5}", Meters{42}), "   42m");
    EXPECT_EQ(std::format("{:#x}", Meters{255}), "0xffm");
}

// "Gotchas": literal braces are doubled.
TEST(Formatting, DoubledBraces) {
    EXPECT_EQ(std::format("{{}}"), "{}");
    EXPECT_EQ(std::format("{{{}}}", 7), "{7}");
}

// "Gotchas": locale-independent by default; the L spec with a std::locale opts in.
TEST(Formatting, LocaleOptIn) {
    std::locale loc;
    try {
        loc = std::locale("en_US.UTF-8");
    } catch (const std::runtime_error&) {
        GTEST_SKIP() << "en_US.UTF-8 locale not available";
    }
    EXPECT_EQ(std::format(loc, "{}", 1234567), "1234567");    // no L: locale ignored
    EXPECT_EQ(std::format(loc, "{:L}", 1234567), "1,234,567");
}
