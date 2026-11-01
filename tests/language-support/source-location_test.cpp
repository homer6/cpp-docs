// Checks claims made by docs/examples/language-support/source-location.md.
#include <cstdint>
#include <cstring>
#include <format>
#include <utility>
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
#if defined(_MSC_VER)
    GTEST_SKIP() << "MS STL fills function_name() from __builtin_FUNCSIG() (MSVC and clang-cl); "
                    "spelling recorded only for GCC and Clang with their own libraries";
#elif defined(__clang__)
    EXPECT_EQ(name, "std::source_location (anonymous namespace)::body_current()");
#elif defined(__GNUC__)
    EXPECT_EQ(name, "std::source_location {anonymous}::body_current()");
#else
    GTEST_SKIP() << "spelling recorded only for GCC and Clang";
#endif
}

// "Declaration": a default-constructed location is "unknown"; it's trivially copyable.
TEST(SourceLocation, DefaultIsUnknown) {
    std::source_location loc;
    EXPECT_STREQ(loc.file_name(), "");
    EXPECT_STREQ(loc.function_name(), "");
    EXPECT_EQ(loc.line(), 0u);
    EXPECT_EQ(loc.column(), 0u);
    static_assert(std::is_trivially_copyable_v<std::source_location>);
}

// "With a variadic function": the fmt_loc wrapper reports the caller's line.
namespace {
struct fmt_loc {
    std::string_view fmt;
    std::source_location loc;
    fmt_loc(const char* f, std::source_location l = std::source_location::current()) : fmt{f}, loc{l} {}
};
template <class... Args>
std::pair<std::uint_least32_t, std::string> logf(fmt_loc f, const Args&... args) {
    return {f.loc.line(), std::vformat(f.fmt, std::make_format_args(args...))};
}
}  // namespace
TEST(SourceLocation, VariadicWrapper) {
    int line = __LINE__ + 1;
    auto [l, text] = logf("x = {}", 42);
    EXPECT_EQ(l, static_cast<std::uint_least32_t>(line));
    EXPECT_EQ(text, "x = 42");
}

// "Gotchas": column() differs — GCC reports the '(' of the call, Clang the
// start of the function name.
TEST(SourceLocation, ColumnDiffersByCompiler) {
    auto loc = log_at();  // log_at starts at column 16, '(' at column 22
#if defined(__clang__)
    EXPECT_EQ(loc.column(), 16u);
#elif defined(__GNUC__)
    EXPECT_EQ(loc.column(), 22u);
#else
    GTEST_SKIP() << "column recorded only for GCC and Clang";
#endif
}
