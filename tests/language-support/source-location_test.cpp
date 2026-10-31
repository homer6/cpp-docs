// Checks claims made by docs/examples/language-support/source-location.md.
#include <cstdint>
#include <cstring>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>

#include <gtest/gtest.h>

namespace {
std::source_location log_at(std::source_location loc = std::source_location::current()) {
    return loc;
}
std::source_location body_current() { return std::source_location::current(); }
std::source_location compute() { return log_at(); }  // the call site is here
}  // namespace

// "Logging that knows its caller": as a default argument it captures the call site.
TEST(SourceLocation, DefaultArgumentCapturesCaller) {
    auto loc = compute();
    EXPECT_NE(std::string_view(loc.function_name()).find("compute"), std::string_view::npos)
        << loc.function_name();
    EXPECT_NE(std::string_view(loc.file_name()).find("source-location_test.cpp"), std::string_view::npos);
    int line = __LINE__ + 1;
    auto here = log_at();
    EXPECT_EQ(here.line(), static_cast<std::uint_least32_t>(line));
}

// "Gotchas": inside the body it records the helper's own location.
TEST(SourceLocation, InBodyCapturesHelper) {
    auto loc = body_current();
    EXPECT_NE(std::string_view(loc.function_name()).find("body_current"), std::string_view::npos);
}

// "What it exposes": the member types.
TEST(SourceLocation, MemberTypes) {
    auto loc = std::source_location::current();
    static_assert(std::is_same_v<decltype(loc.file_name()), const char*>);
    static_assert(std::is_same_v<decltype(loc.function_name()), const char*>);
    static_assert(std::is_same_v<decltype(loc.line()), std::uint_least32_t>);
    static_assert(std::is_same_v<decltype(loc.column()), std::uint_least32_t>);
    EXPECT_GT(loc.column(), 0u);
}

// "Gotchas": function_name() formatting is implementation-defined — GCC prints
// the signature, Clang 22 too; the exact text differs.
TEST(SourceLocation, FunctionNameIsImplementationDefined) {
    std::string name = body_current().function_name();
#if defined(__clang__)
    EXPECT_EQ(name, "std::source_location (anonymous namespace)::body_current()");
#elif defined(__GNUC__)
    EXPECT_EQ(name, "std::source_location {anonymous}::body_current()");
#else
    GTEST_SKIP() << "spelling recorded only for GCC and Clang";
#endif
}
