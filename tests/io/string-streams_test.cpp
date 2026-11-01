// Checks claims made by docs/examples/io/string-streams.md.
#include <format>
#include <iterator>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <version>
#if __has_include(<spanstream>)
#include <spanstream>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Building strings": the built string, and the std::format equivalent.
TEST(StringStreams, Build) {
    std::ostringstream oss;
    oss << "x=" << 42 << ", y=" << 3.14;
    EXPECT_EQ(oss.str(), "x=42, y=3.14");
    EXPECT_EQ(std::format("x={}, y={}", 42, 3.14), oss.str());
}

// "Parsing / tokenizing": three ints, word splitting, and checked parsing.
TEST(StringStreams, ParseAndTokenize) {
    std::istringstream iss{"10 20 30"};
    int a, b, c;
    iss >> a >> b >> c;
    EXPECT_EQ(a + b + c, 60);
    EXPECT_EQ(c, 30);
    std::istringstream line{"the quick brown fox"};
    std::vector<std::string> words{std::istream_iterator<std::string>{line},
                                   std::istream_iterator<std::string>{}};
    EXPECT_EQ(words, (std::vector<std::string>{"the", "quick", "brown", "fox"}));
    std::istringstream bad{"abc"};
    int value;
    EXPECT_FALSE(bad >> value);
    int n;
    std::istringstream{"123"} >> n;
    EXPECT_EQ(n, 123);
}

// "Gotchas": str() returns a copy; the rvalue overload (C++20) moves the buffer out.
TEST(StringStreams, StrIsACopy) {
    std::ostringstream oss;
    oss << "hello";
    std::string s = oss.str();
    s[0] = 'J';
    EXPECT_EQ(oss.str(), "hello");
    std::string moved = std::move(oss).str();
    EXPECT_EQ(moved, "hello");
}

// "Gotchas": reuse needs both str("") and clear().
TEST(StringStreams, ReuseNeedsStrAndClear) {
    std::istringstream in{"1"};
    int x;
    in >> x >> x;  // second read hits EOF → failbit
    EXPECT_TRUE(in.fail());
    in.str("5");
    EXPECT_FALSE(in >> x);  // flags still set: nothing read
    in.clear();
    in.str("5");
    EXPECT_TRUE(in >> x);
    EXPECT_EQ(x, 5);
}

// "Gotchas": >> stops at whitespace; getline reads the phrase.
TEST(StringStreams, ExtractionStopsAtWhitespace) {
    std::istringstream a{"two words"}, b{"two words"};
    std::string s;
    a >> s;
    EXPECT_EQ(s, "two");
    std::getline(b, s);
    EXPECT_EQ(s, "two words");
}

// "Fixed buffers: std::spanstream (C++23)": writes into the caller's buffer, never
// grows (a full stream sets failbit), and ispanstream parses a view in place.
TEST(StringStreams, SpanStreamFixedBuffer) {
    CPPDOCS_REQUIRE(__cpp_lib_spanstream);
#ifdef __cpp_lib_spanstream
    char buf[16];
    std::ospanstream out{std::span{buf}};
    out << "x=" << 42;
    std::span<char> written = out.span();
    EXPECT_EQ(std::string_view(written.data(), written.size()), "x=42");
    EXPECT_EQ(written.data(), buf);  // no copy: the stream writes into buf
    EXPECT_FALSE(out.fail());

    out << " and far more than sixteen chars";
    EXPECT_TRUE(out.fail());
    EXPECT_EQ(out.span().size(), 16u);

    std::ispanstream in{std::string_view{"10 20 30"}};
    int a, b, c;
    in >> a >> b >> c;
    EXPECT_EQ(a, 10);
    EXPECT_EQ(b, 20);
    EXPECT_EQ(c, 30);
#endif
}
