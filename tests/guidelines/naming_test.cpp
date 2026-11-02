// Checks claims made by docs/guidelines/core-guidelines/examples/naming.md.
#include <chrono>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

using namespace std::literals;

// NL.11: digit separators don't change the value, and literal suffixes pick the type.
TEST(CoreGuidelinesNL, NL11_ReadableLiterals) {
    static_assert(100'000'000 == 100000000);
    static_assert(0b1111'0000 == 0xF0);
    auto timeout = 250ms;
    auto greeting = "hello"s;
    static_assert(std::is_same_v<decltype(timeout), std::chrono::milliseconds>);
    static_assert(std::is_same_v<decltype(greeting), std::string>);
    EXPECT_EQ(timeout.count(), 250);
    EXPECT_EQ(greeting.size(), 5u);
}

// NL.21: in "int* p, q;" only p is a pointer; q is a plain int.
TEST(CoreGuidelinesNL, NL21_OneNamePerDeclaration) {
    int* p = nullptr, q = 0;
    static_assert(std::is_same_v<decltype(p), int*>);
    static_assert(std::is_same_v<decltype(q), int>);
    EXPECT_EQ(p, nullptr);
    EXPECT_EQ(q, 0);
}

// NL.25: in C++, f(void) and f() declare the same function type.
TEST(CoreGuidelinesNL, NL25_VoidParameterListIsNoise) {
    static_assert(std::is_same_v<int(void), int()>);
    SUCCEED();
}

// NL.26: "int const" and "const int" are the same type; only the convention differs.
TEST(CoreGuidelinesNL, NL26_ConstPlacementIsTheSameType) {
    static_assert(std::is_same_v<int const, const int>);
    static_assert(std::is_same_v<char const* const, const char* const>);
    SUCCEED();
}
