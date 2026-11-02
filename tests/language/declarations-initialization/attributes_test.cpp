// Checks claims made by docs/examples/language/declarations-initialization/attributes.md.

#include <expected>
#include <stdexcept>
#include <string>
#include <string_view>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
enum class Status { ok, failed };

[[nodiscard("check the status")]] Status run_status() { return Status::ok; }

[[noreturn]] void fail() { throw std::runtime_error("fail"); }

[[deprecated("use new_api()")]] int old_api() { return 1; }

struct Error { std::string message; };
std::string trace;
void hot_path() { trace += "hot "; }
void rare_path() { trace += "rare "; }
void setup() { trace += "setup "; }
void run() { trace += "run "; }

#ifdef __cpp_lib_expected  // Clang 18 with libstdc++ 14: <expected> needs __cpp_concepts 202002L
[[nodiscard]] std::expected<int, Error> parse(std::string_view s) {
    if (s.empty()) return std::unexpected(Error{"empty"});
    return static_cast<int>(s.size());
}
#endif

void dispatch(const int* ptr, int n) {  // the page's "Common uses" function
    if (ptr) [[likely]] { hot_path(); }
    else     [[unlikely]] { rare_path(); }

    switch (n) {
        case 1: setup(); [[fallthrough]];
        case 2: run(); break;
    }
}

int sum_unused(int x [[maybe_unused]], int y) { return y; }
}  // namespace

// "The standard attributes": each one the page lists is known, at the version its "since" implies.
TEST(Attributes, StandardAttributesAreRecognized) {
    // __has_cpp_attribute is only defined in #if / #elif ([cpp.cond]); GCC and
    // Clang also accept it in a static_assert, MSVC doesn't (C3861).
#if __has_cpp_attribute(noreturn) < 200809L          // C++11
#error "[[noreturn]] not recognized"
#endif
#if __has_cpp_attribute(deprecated) < 201309L        // C++14
#error "[[deprecated]] not recognized"
#endif
#if __has_cpp_attribute(fallthrough) < 201603L       // C++17
#error "[[fallthrough]] not recognized"
#endif
#if __has_cpp_attribute(maybe_unused) < 201603L      // C++17
#error "[[maybe_unused]] not recognized"
#endif
#if __has_cpp_attribute(nodiscard) < 201907L         // reason string: C++20
#error "[[nodiscard]] with a reason not recognized"
#endif
#if __has_cpp_attribute(likely) < 201803L || __has_cpp_attribute(unlikely) < 201803L  // C++20
#error "[[likely]] / [[unlikely]] not recognized"
#endif
#if defined(_MSC_VER) && !defined(__clang__)
    GTEST_SKIP() << "MSVC doesn't implement [[assume]] (P1774R8: \"no\" in Microsoft's conformance table); "
                    "the other attributes were checked";
#elif __has_cpp_attribute(assume) < 202207L          // C++23
    GTEST_SKIP() << "[[assume]] (P1774R8, C++23) not recognized: __has_cpp_attribute(assume) < "
                    "202207L (GCC 13 and Clang 19 add it; Clang 18 ignores it); "
                    "the other attributes were checked";
#endif
}

// "The standard attributes": attributes don't change meaning — the functions behave normally.
TEST(Attributes, AttributedFunctionsBehaveNormally) {
    EXPECT_EQ(run_status(), Status::ok);
    EXPECT_THROW(fail(), std::runtime_error);  // [[noreturn]]: leaves by throwing
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT_EQ(old_api(), 1);  // deprecated warns but still works
#pragma GCC diagnostic pop
    EXPECT_EQ(sum_unused(9, 2), 2);
}

// "The standard attributes" (statement attributes): fallthrough runs into the next case; assume is a no-op.
TEST(Attributes, StatementAttributes) {
    int x = 1;
    switch (x) {
        case 1: ++x; [[fallthrough]];
        case 2: break;
    }
    EXPECT_EQ(x, 2);

    std::string branch;
    if (x > 0) [[likely]] { branch = "then"; }
    else       [[unlikely]] { branch = "else"; }
    EXPECT_EQ(branch, "then");

#if __has_cpp_attribute(assume) >= 202207L
    [[assume(x > 0)]];
#endif
    EXPECT_EQ(x, 2);
}

// "Common uses": likely/unlikely only hint; case 1 falls through into case 2.
TEST(Attributes, CommonUses) {
    int v = 0;
    trace.clear();
    dispatch(&v, 1);
    EXPECT_EQ(trace, "hot setup run ");
    trace.clear();
    dispatch(nullptr, 2);
    EXPECT_EQ(trace, "rare run ");

    CPPDOCS_REQUIRE(__cpp_lib_expected);  // the likely/fallthrough checks above still ran
#ifdef __cpp_lib_expected
    EXPECT_EQ(parse("abc").value(), 3);
    EXPECT_EQ(parse("").error().message, "empty");
#endif
}

// Tests added with the page sections from #56 (#69); kept in their own
// namespace so their helpers and test names can't clash with the ones above.
namespace from_pr69 {
namespace {
#if __has_cpp_attribute(assume) >= 202207L
int count_calls() {
    int count = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wassume"
    [[assume(++count > 0)]];
#pragma GCC diagnostic pop
    return count;
}
#endif
}  // namespace

// "[[assume(expr)]]": the expression is never evaluated.
TEST(Attributes, AssumeIsNotEvaluated) {
#if __has_cpp_attribute(assume) >= 202207L
    EXPECT_EQ(count_calls(), 0);
#else
    GTEST_SKIP() << "[[assume]] (P1774R8, C++23) not recognized by this compiler (an ignored "
                    "attribute would pass trivially)";
#endif
}
}  // namespace from_pr69
