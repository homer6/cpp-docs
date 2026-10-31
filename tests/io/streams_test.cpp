// Checks claims made by docs/examples/io/streams.md.
#include <iostream>
#include <limits>
#include <sstream>
#include <streambuf>
#include <string>

#include <gtest/gtest.h>

#include "support/capture.hpp"

namespace {
// Counts flushes (sync calls) that reach the buffer.
struct CountingBuf : std::stringbuf {
    int syncs = 0;
    int sync() override { ++syncs; return std::stringbuf::sync(); }
};
}  // namespace

// "The basics": chained output; >> skips leading whitespace; one token; getline a line.
TEST(Streams, Basics) {
    auto out = cppdocs::capture_stdout([] { std::cout << "x = " << 42 << '\n'; });
    EXPECT_EQ(out, "x = 42\n");
    std::istringstream in{"  7 word rest of line"};
    int n;
    std::string word, line;
    in >> n >> word;
    std::getline(in, line);
    EXPECT_EQ(n, 7);
    EXPECT_EQ(word, "word");
    EXPECT_EQ(line, " rest of line");
}

// "Stream state": truthy iff the read succeeded; eof/fail/good; clear resets.
TEST(Streams, StateFlags) {
    std::istringstream in{"abc"};
    int n;
    EXPECT_FALSE(in >> n);
    EXPECT_TRUE(in.fail());
    EXPECT_FALSE(in.bad());
    EXPECT_FALSE(in.good());
    in.clear();
    EXPECT_TRUE(in.good());
    std::string s;
    in >> s;
    EXPECT_EQ(s, "abc");
    EXPECT_TRUE(in.eof());
}

// "The idiom for reading many values": the loop stops at EOF or bad input.
TEST(Streams, ReadLoop) {
    std::istringstream in{"1 2 3 x 4"};
    int x, sum = 0;
    while (in >> x) sum += x;
    EXPECT_EQ(sum, 6);  // stopped at "x"
}

// "Newlines and flushing" and "Gotchas": endl flushes every time, '\n' doesn't.
TEST(Streams, EndlFlushesNewlineDoesNot) {
    CountingBuf buf;
    std::ostream os{&buf};
    os << "line\n";
    EXPECT_EQ(buf.syncs, 0);
    os << "line" << std::endl;
    EXPECT_EQ(buf.syncs, 1);
    os << std::flush;
    EXPECT_EQ(buf.syncs, 2);
    EXPECT_EQ(buf.str(), "line\nline\n");
}

// "Gotchas": >> leaves the newline, so a following getline reads an empty line;
// std::ws or ignore() consumes it.
TEST(Streams, LeftoverNewline) {
    std::istringstream in{"5\nhello\n"};
    int n;
    std::string line;
    in >> n;
    std::getline(in, line);
    EXPECT_EQ(line, "");
    std::istringstream in2{"5\nhello\n"};
    in2 >> n >> std::ws;
    std::getline(in2, line);
    EXPECT_EQ(line, "hello");
    std::istringstream in3{"5\nhello\n"};
    in3 >> n;
    in3.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(in3, line);
    EXPECT_EQ(line, "hello");
}

// "Gotchas": a failed >> sets failbit and stores 0 (since C++11); later reads
// no-op until clear().
TEST(Streams, FailedReadStoresZero) {
    std::istringstream in{"abc 5"};
    int n = 7;
    in >> n;
    EXPECT_TRUE(in.fail());
    EXPECT_EQ(n, 0);
    int m = 9;
    in >> m;
    EXPECT_EQ(m, 9);  // no-op while failbit is set
    in.clear();
    in.ignore(4);
    in >> m;
    EXPECT_EQ(m, 5);
}
