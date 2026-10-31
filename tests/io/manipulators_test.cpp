// Checks claims made by docs/examples/io/manipulators.md.
#include <iomanip>
#include <ios>
#include <ostream>
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

// "Mental model": hex stays on, setw applies to the next item only.
TEST(Manipulators, MentalModelDiagram) {
    EXPECT_EQ(fresh([](auto& os) { os << std::hex << 255 << ' ' << std::setw(4) << 1 << 2; }), "ff    12");
}

// "Common manipulators": the lines run in order on one stream, with the
// page's resets; each comment is the text that line adds.
TEST(Manipulators, CommonOutputsInSequence) {
    std::ostringstream os;
    auto step = [&](auto f) {
        os.str("");
        f(os);
        return os.str();
    };
    EXPECT_EQ(step([](auto& s) { s << std::boolalpha << true; }), "true");
    EXPECT_EQ(step([](auto& s) { s << std::hex << 255; }), "ff");
    EXPECT_EQ(step([](auto& s) { s << std::showbase << 255; }), "0xff");
    EXPECT_EQ(step([](auto& s) { s << std::dec << std::noshowbase; }), "");
    EXPECT_EQ(step([](auto& s) { s << std::setw(8) << 42; }), "      42");
    EXPECT_EQ(step([](auto& s) { s << std::setfill('0') << std::setw(4) << 7; }), "0007");
    EXPECT_EQ(step([](auto& s) { s << std::setfill(' '); }), "");
    EXPECT_EQ(step([](auto& s) { s << std::left << std::setw(8) << "hi"; }), "hi      ");
}

// "Gotchas": without the resets, setw(8) << 42 prints "    0x2a" and
// left << setw(8) << "hi" prints "hi000000".
TEST(Manipulators, WithoutResets) {
    EXPECT_EQ(fresh([](auto& s) { s << std::hex << std::showbase << std::setw(8) << 42; }), "    0x2a");
    EXPECT_EQ(fresh([](auto& s) { s << std::setfill('0') << std::left << std::setw(8) << "hi"; }), "hi000000");
    EXPECT_EQ(fresh([](auto& s) { s << std::setw(2) << "hello"; }), "hello");  // setw never truncates
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

// "Save and restore the state": copyfmt brings back the caller's formatting;
// "Gotchas": fmtflags save/restore works for the flags.
TEST(Manipulators, SaveRestoreFlags) {
    std::ostringstream os;
    std::ios::fmtflags saved = os.flags();
    os << std::hex << 255 << ' ';
    os.flags(saved);
    os << 255;
    EXPECT_EQ(os.str(), "ff 255");

    auto print_hex = [](std::ostream& o, int v) {
        std::ios saved(nullptr);
        saved.copyfmt(o);
        o << std::hex << std::setfill('0') << std::setw(4) << v;
        o.copyfmt(saved);
    };
    std::ostringstream caller;
    print_hex(caller, 255);
    caller << ' ' << std::setw(3) << 7;
    EXPECT_EQ(caller.str(), "00ff   7");  // decimal and ' ' fill again
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
