// Checks claims made by docs/guidelines/core-guidelines/examples/stdlib.md.
#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <format>
#include <functional>
#include <iostream>
#include <iterator>
#include <locale>
#include <numeric>
#include <optional>
#include <ostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "support/capture.hpp"

struct UserId {
    int value;
    bool operator==(const UserId&) const = default;
};
template <>
struct std::hash<UserId> {
    std::size_t operator()(const UserId& u) const noexcept { return std::hash<int>{}(u.value); }
};

namespace {
int sum(const std::array<int, 4>& scores) {
    return std::accumulate(scores.begin(), scores.end(), 0);
}

template <class T>
concept has_size = requires(const T& t) { std::size(t); };

struct Entry {
    std::string key;
    int hits;
};

struct Polymorphic {
    virtual ~Polymorphic() = default;
    int x = 0;
};

std::string join(std::string_view a, std::string_view b) {
    std::string out{a};
    out += b;
    return out;
}

std::size_t count_spaces(std::string_view s) {
    return static_cast<std::size_t>(std::ranges::count(s, ' '));
}

void capitalize(std::span<char> word) {
    if (!word.empty())
        word[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(word[0])));
}

std::string read_word(std::istream& in) {
    std::string word;
    in >> word;
    return word;
}

std::optional<int> read_age(std::istream& in) {
    int age = 0;
    if (!(in >> age) || age < 0 || age > 150) return std::nullopt;
    return age;
}

struct Point {
    int x, y;
};
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << '(' << p.x << ", " << p.y << ')';
}

struct Counted {
    int* destroyed;
    explicit Counted(int* d) : destroyed{d} {}
    Counted(const Counted&) = delete;
    Counted& operator=(const Counted&) = delete;
    ~Counted() { ++*destroyed; }
};
}  // namespace

// SL.3: specializing std::hash for our own type is allowed, and makes it usable in unordered containers.
TEST(CoreGuidelinesSL, SL3_HashSpecializationForOwnType) {
    std::unordered_set<UserId> ids{{1}, {2}, {1}};
    EXPECT_EQ(ids.size(), 2u);
    EXPECT_TRUE(ids.contains(UserId{2}));
}

// SL.4: std::format takes each argument's type from the argument, not from the format string.
TEST(CoreGuidelinesSL, SL4_FormatIsTypeSafe) {
    const long long bytes = 2'500'000'000;
    EXPECT_EQ(std::format("{} bytes", bytes), "2500000000 bytes");
    EXPECT_EQ(std::format("{} {}", std::string{"id"}, 7), "id 7");
}

// SL.con.1: std::array keeps its size when passed; a C array parameter is only a pointer.
TEST(CoreGuidelinesSL, SL_con1_ArrayKnowsItsSize) {
    const std::array<int, 4> scores{1, 2, 3, 4};
    EXPECT_EQ(sum(scores), 10);
    static_assert(std::tuple_size_v<std::array<int, 4>> == 4);
    static_assert(has_size<std::array<int, 4>>);
    static_assert(!has_size<const int*>);   // what a `const int scores[4]` parameter really is
    static_assert(std::is_same_v<void(const int[4]), void(const int*)>);
    const std::array<int, 4> copy = scores;
    EXPECT_EQ(copy, scores);   // copyable and comparable, unlike a C array
}

// SL.con.2: vector is contiguous and random access; (n) makes n elements, {n} one element.
TEST(CoreGuidelinesSL, SL_con2_VectorDefaults) {
    std::vector<int> v1(20);
    std::vector<int> v2{20};
    EXPECT_EQ(v1.size(), 20u);
    EXPECT_EQ(v1[19], 0);
    ASSERT_EQ(v2.size(), 1u);
    EXPECT_EQ(v2[0], 20);
    static_assert(std::contiguous_iterator<std::vector<int>::iterator>);
    EXPECT_EQ(&v1[5], v1.data() + 5);
}

// SL.con.3: at() checks the index and throws std::out_of_range.
TEST(CoreGuidelinesSL, SL_con3_AtIsBoundsChecked) {
    const std::vector<int> v{1, 2, 3};
    EXPECT_EQ(v.at(2), 3);
    EXPECT_THROW((void)v.at(3), std::out_of_range);
    const std::vector<int> empty;
    EXPECT_THROW((void)empty.at(empty.size() - 1), std::out_of_range);
}

// SL.con.4: types with a string member or a vtable aren't trivially copyable; assignment resets them.
TEST(CoreGuidelinesSL, SL_con4_NotTriviallyCopyable) {
    static_assert(!std::is_trivially_copyable_v<Entry>);
    static_assert(!std::is_trivially_copyable_v<Polymorphic>);
    Entry e{"key", 3};
    e = {};
    EXPECT_TRUE(e.key.empty());
    EXPECT_EQ(e.hits, 0);
}

// SL.str.1: std::string owns and grows its characters; no sizes or frees to get wrong.
TEST(CoreGuidelinesSL, SL_str1_StringOwnsCharacters) {
    EXPECT_EQ(join("file", ".txt"), "file.txt");
    std::string a = "abc";
    std::string b = a;
    b += 'd';
    EXPECT_EQ(a, "abc");   // a copy owns its own characters
    EXPECT_EQ(b, "abcd");
}

// SL.str.2: string_view accepts literals and strings without copying; it refers to, not owns, the text.
TEST(CoreGuidelinesSL, SL_str2_StringViewDoesNotOwn) {
    EXPECT_EQ(count_spaces("a b c"), 2u);
    std::string line = "x y";
    EXPECT_EQ(count_spaces(line), 1u);
    std::string_view v = line;
    EXPECT_EQ(v.data(), line.data());   // the same characters, not a copy
    line[0] = 'z';
    EXPECT_EQ(v[0], 'z');
}

// SL.str.5: std::byte has no arithmetic and converts to a number explicitly.
TEST(CoreGuidelinesSL, SL_str5_ByteIsNotACharacter) {
    static_assert(!std::is_arithmetic_v<std::byte>);
    std::byte b{0x41};
    b |= std::byte{0x02};
    EXPECT_EQ(std::to_integer<int>(b), 0x43);
    std::ostringstream os;
    os << std::to_integer<int>(std::byte{0x41});
    EXPECT_EQ(os.str(), "65");   // a value, not the character 'A'
}

// SL.str.10: a std::locale compares std::strings, but not string_views.
TEST(CoreGuidelinesSL, SL_str10_LocaleWorksOnStrings) {
    const std::locale loc = std::locale::classic();
    EXPECT_TRUE(loc(std::string{"apple"}, std::string{"banana"}));
    EXPECT_FALSE(loc(std::string{"banana"}, std::string{"apple"}));
    static_assert(std::is_invocable_v<const std::locale&, const std::string&, const std::string&>);
    static_assert(!std::is_invocable_v<const std::locale&, std::string_view, std::string_view>);
}

// SL.str.11: string_view is read-only; span<char> can change the characters in place.
TEST(CoreGuidelinesSL, SL_str11_SpanMutatesStringViewDoesNot) {
    static_assert(std::is_same_v<decltype(std::declval<std::string_view&>()[0]), const char&>);
    std::string word = "apple";
    capitalize(word);
    EXPECT_EQ(word, "Apple");
}

// SL.str.12: a literal deduces as const char*; the s suffix makes a std::string.
TEST(CoreGuidelinesSL, SL_str12_SSuffixMakesString) {
    using namespace std::string_literals;
    auto plain = std::make_pair("Tokyo", 9.0);
    auto suffixed = std::make_pair("Tokyo"s, 9.0);
    static_assert(std::is_same_v<decltype(plain.first), const char*>);
    static_assert(std::is_same_v<decltype(suffixed.first), std::string>);
    EXPECT_EQ(suffixed.first.size(), 5u);
}

// SL.io.1: >> into a string reads a whole word, skipping whitespace, of any length.
TEST(CoreGuidelinesSL, SL_io1_ReadWholeWords) {
    std::istringstream in{"   supercalifragilistic next"};
    EXPECT_EQ(read_word(in), "supercalifragilistic");
    EXPECT_EQ(read_word(in), "next");
}

// SL.io.2: a failed read into a number leaves the stream failed and the value 0; check before use.
TEST(CoreGuidelinesSL, SL_io2_IllFormedInputIsDetected) {
    std::istringstream bad{"abc"};
    int n = 7;
    bad >> n;
    EXPECT_TRUE(bad.fail());
    EXPECT_EQ(n, 0);
    std::istringstream good{"42"};
    std::istringstream negative{"-5"};
    std::istringstream text{"old"};
    EXPECT_EQ(read_age(good), std::optional<int>{42});
    EXPECT_EQ(read_age(negative), std::nullopt);
    EXPECT_EQ(read_age(text), std::nullopt);
}

// SL.io.3: an operator<< extends streams to a user type.
TEST(CoreGuidelinesSL, SL_io3_StreamsExtendToUserTypes) {
    std::ostringstream os;
    os << "at " << Point{3, 4} << '\n';
    EXPECT_EQ(os.str(), "at (3, 4)\n");
}

// SL.io.50: endl writes the same single '\n' as '\n' (plus a flush), on every platform.
TEST(CoreGuidelinesSL, SL_io50_EndlIsNewlinePlusFlush) {
    std::ostringstream a, b;
    a << "line" << std::endl;
    b << "line" << '\n';
    EXPECT_EQ(a.str(), b.str());
    EXPECT_EQ(a.str(), "line\n");
    auto with_endl = cppdocs::capture_stdout([] { std::cout << 1 << std::endl << 2 << std::endl; });
    auto with_newline = cppdocs::capture_stdout([] { std::cout << 1 << '\n' << 2 << '\n'; });
    EXPECT_EQ(with_endl, "1\n2\n");
    EXPECT_EQ(with_newline, with_endl);
}

// SL.C.1: an exception, unlike longjmp, runs the destructors of the frames it leaves.
TEST(CoreGuidelinesSL, SL_C1_ExceptionsRunDestructors) {
    int destroyed = 0;
    auto parse = [&](bool fail) {
        Counted tokens{&destroyed};
        if (fail) throw std::runtime_error{"parse error"};
    };
    EXPECT_THROW(parse(true), std::runtime_error);
    EXPECT_EQ(destroyed, 1);
}
