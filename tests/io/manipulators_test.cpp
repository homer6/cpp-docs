// Checks claims made by docs/examples/io/manipulators.md.
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace {
template <class F>
std::string fresh(F f) {
    std::ostringstream os;
    f(os);
    return os.str();
}
}  // namespace

// "Common manipulators": each line's output, on a fresh stream.
TEST(Manipulators, CommonOutputs) {
    EXPECT_EQ(fresh([](auto& os) { os << std::boolalpha << true; }), "true");
    EXPECT_EQ(fresh([](auto& os) { os << std::hex << 255; }), "ff");
    EXPECT_EQ(fresh([](auto& os) { os << std::showbase << std::hex << 255; }), "0xff");
    EXPECT_EQ(fresh([](auto& os) { os << std::setw(8) << 42; }), "      42");
    EXPECT_EQ(fresh([](auto& os) { os << std::setfill('0') << std::setw(4) << 7; }), "0007");
    EXPECT_EQ(fresh([](auto& os) { os << std::left << std::setw(8) << "hi"; }), "hi      ");
}

// "Common manipulators": the pi lines run in sequence on one stream, so the
// precision of 2 is still in effect for the scientific line.
TEST(Manipulators, FixedScientificPrecision) {
    double pi = 3.14159265;
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << pi;
    EXPECT_EQ(os.str(), "3.14");
    os.str("");
    os << std::scientific << pi;
    EXPECT_EQ(os.str(), "3.14e+00");
    os.str("");
    os << std::setprecision(10) << pi;
    EXPECT_EQ(os.str(), "3.1415926500e+00");
    EXPECT_EQ(fresh([&](auto& s) { s << std::scientific << pi; }), "3.141593e+00");  // default precision 6
}

// "Gotchas": setw applies to the next item only; the others are sticky.
TEST(Manipulators, SetwIsNotSticky) {
    std::ostringstream os;
    os << std::setfill('.') << std::setw(4) << 1 << 2 << std::hex << 10 << ' ' << 11;
    EXPECT_EQ(os.str(), "...12a b");
    EXPECT_EQ(os.fill(), '.');  // still set
}

// "Gotchas": save and restore the flags to stop sticky state leaking.
TEST(Manipulators, SaveRestoreFlags) {
    std::ostringstream os;
    std::ios::fmtflags saved = os.flags();
    os << std::hex << 255 << ' ';
    os.flags(saved);
    os << 255;
    EXPECT_EQ(os.str(), "ff 255");
}

// "Quoting (C++14)": quoted escapes on output and round-trips on input.
TEST(Manipulators, Quoted) {
    EXPECT_EQ(fresh([](auto& os) { os << std::quoted("he said \"hi\""); }), R"("he said \"hi\"")");
    std::string s;
    std::istringstream{"\"a b c\""} >> std::quoted(s);
    EXPECT_EQ(s, "a b c");
    std::stringstream ss;
    ss << std::quoted("x \"y\" z");
    ss >> std::quoted(s);
    EXPECT_EQ(s, "x \"y\" z");
}
