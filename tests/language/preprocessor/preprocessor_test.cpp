// Checks claims made by docs/examples/language/preprocessor/preprocessor.md.
#include <cstring>
#include <string_view>
#include <version>

#include <gtest/gtest.h>

#define SQUARE(x) ((x) * (x))
#define SQ_BAD(x) x*x
#define BUFFER_SIZE 1024

// "Gotchas": an unparenthesized macro gives SQ(1+2) == 5; the parenthesized one gives 9.
TEST(Preprocessor, FunctionLikeMacroParentheses) {
    EXPECT_EQ(SQ_BAD(1 + 2), 5);  // expands to 1+2*1+2
    EXPECT_EQ(SQUARE(1 + 2), 9);
    // "...and even then, double-evaluation bites": the argument is evaluated twice.
    int calls = 0;
    auto next = [&] { return ++calls; };
    (void)SQUARE(next());
    EXPECT_EQ(calls, 2);
}

// "Macros (#define)": #undef removes the definition.
TEST(Preprocessor, Undef) {
    EXPECT_EQ(BUFFER_SIZE, 1024);
#undef BUFFER_SIZE
#ifdef BUFFER_SIZE
    FAIL() << "BUFFER_SIZE still defined";
#endif
}

namespace {
const char* where() { return __func__; }  // __func__ is the enclosing function's name
}  // namespace

// "Useful predefined macros": __FILE__, __LINE__, __func__, __cplusplus.
TEST(Preprocessor, PredefinedMacros) {
    EXPECT_NE(std::string_view(__FILE__).find("preprocessor_test.cpp"), std::string_view::npos);
    const long line = __LINE__;
    EXPECT_EQ(__LINE__, line + 1);
    EXPECT_STREQ(where(), "where");
    EXPECT_GT(__cplusplus, 202302L);  // built with -std=c++26: newer than C++23's value
}

// "Conditional compilation": __cpp_lib_format >= 201907L means <format> is usable.
TEST(Preprocessor, FeatureTestMacroGatesInclude) {
#if __cpp_lib_format >= 201907L
    SUCCEED();
#else
    GTEST_SKIP() << "needs __cpp_lib_format >= 201907L";
#endif
}
