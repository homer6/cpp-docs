// Checks claims made by docs/examples/language/statements/selection.md.
#include <map>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

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
