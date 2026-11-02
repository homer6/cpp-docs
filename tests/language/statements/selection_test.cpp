// Checks claims made by docs/examples/language/statements/selection.md.
#include <charconv>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
std::string trace;
void handle_low() { trace += "low "; }
void handle3() { trace += "3 "; }
void handle4() { trace += "4 "; }
void handle_default() { trace += "default "; }

void dispatch(int code) {  // the page's switch, with classify() = identity
    switch (code) {
        case 1:
        case 2:  handle_low(); break;
        case 3:  handle3();
                 [[fallthrough]];
        case 4:  handle4(); break;
        default: handle_default();
    }
}

template <class T> std::string kind(T v) {
    if constexpr (std::is_pointer_v<T>) return "pointer:" + std::to_string(*v);
    else return "value:" + std::to_string(v);  // `*v` would not compile for int; discarded branch
}
}  // namespace

// "switch": 1 and 2 share a handler; 3 falls through into 4; others hit default.
TEST(Selection, SwitchFallthrough) {
    for (int code : {1, 2, 3, 4, 9}) {
        trace.clear();
        dispatch(code);
        switch (code) {
            case 1: case 2: EXPECT_EQ(trace, "low "); break;
            case 3: EXPECT_EQ(trace, "3 4 "); break;
            case 4: EXPECT_EQ(trace, "4 "); break;
            default: EXPECT_EQ(trace, "default ");
        }
    }
}

// "if — with an init-statement": the variable is visible in both branches.
TEST(Selection, IfWithInitStatement) {
    std::map<std::string, int> m{{"a", 1}};
    std::string result;
    for (std::string key : {"a", "b"}) {
        if (auto it = m.find(key); it != m.end())
            result += std::to_string(it->second);
        else
            result += "missing(" + std::string(it == m.end() ? "end" : "?") + ")";
    }
    EXPECT_EQ(result, "1missing(end)");
}

// "if constexpr": the untaken branch is discarded per instantiation.
TEST(Selection, IfConstexprDiscardsBranch) {
    int x = 7;
    EXPECT_EQ(kind(x), "value:7");
    EXPECT_EQ(kind(&x), "pointer:7");
}

namespace {
enum class Color { red, green, blue };
int classify(int x) { return x % 5; }

std::string dispatch_with_init(int x) {
    switch (auto code = classify(x); code) {  // init-statement in a switch (C++17)
        case 1:  return "low " + std::to_string(code);
        default: return "other " + std::to_string(code);
    }
}

std::string describe(Color c) {
    switch (c) {  // enum cases
        case Color::red: {
            std::string s = "warm";  // declaration in a braced case body
            return s;
        }
        case Color::green:
        case Color::blue: return "cool";
    }
    return "?";
}
}  // namespace

// "switch": init-statement, enum cases, and a braced case body that declares a variable.
TEST(Selection, SwitchInitEnumAndBracedCase) {
    EXPECT_EQ(dispatch_with_init(6), "low 1");
    EXPECT_EQ(dispatch_with_init(8), "other 3");
    EXPECT_EQ(describe(Color::red), "warm");
    EXPECT_EQ(describe(Color::blue), "cool");
}

// Needs P0963 (MSVC has no C++26 core features yet) and from_chars_result's operator bool
// (P2497; MS STL doesn't have it, so clang-cl can't build it either).
#if __cpp_structured_bindings >= 202403L && !defined(_MSVC_STL_VERSION)
#define SELECTION_SB_CONDITION 1
#endif

namespace {
#ifdef SELECTION_SB_CONDITION
int parse_or(std::string_view s, int fallback) {
    int value = 0;
    if (auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value))
        return ptr == s.data() + s.size() ? value : fallback;
    else
        return fallback;
}
#endif
template <class T>
concept TestableAsCondition = requires(T t) { t ? 0 : 1; };
}  // namespace

// "Structured binding as a condition (C++26)": the whole object is tested, then bound.
TEST(Selection, StructuredBindingAsCondition) {
    static_assert(!TestableAsCondition<std::pair<int, bool>>);  // pair has no operator bool
    CPPDOCS_REQUIRE_VERSION(__cpp_structured_bindings, 202403L);
#if defined(_MSVC_STL_VERSION)
    GTEST_SKIP() << "MS STL's std::from_chars_result has no operator bool (P2497)";
#endif
#ifdef SELECTION_SB_CONDITION
    EXPECT_EQ(parse_or("42", -1), 42);
    EXPECT_EQ(parse_or("x", -1), -1);
    EXPECT_EQ(parse_or("42x", -1), -1);  // parsed, but not all of the string
#endif
}
