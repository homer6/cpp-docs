// Checks claims made by docs/examples/language/statements/jump-statements.md.
#include <algorithm>
#include <cstdio>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/capture.hpp"
#include "support/feature.hpp"

namespace {
std::string trace;
struct Noisy {  // logs construction and destruction
    char id;
    explicit Noisy(char c) : id(c) { trace += '+'; trace += id; }
    ~Noisy() { trace += '-'; trace += id; }
};

constexpr int first_negative(std::span<const int> xs) {
    int index = 0;
    for (int x : xs) {
        if (x >= 0) { ++index; continue; }
        break;
    }
    return index < static_cast<int>(xs.size()) ? index : -1;
}
constexpr int data[] = {3, 1, -4, 1};

std::vector<int> make_ids() { return {1, 2, 3}; }
int log_calls = 0;
void log_line(const std::string&) { ++log_calls; }
void log_twice(const std::string& s) { log_line(s); return log_line(s); }

struct CopyCounter {
    static inline int copies = 0;
    CopyCounter() = default;
    CopyCounter(const CopyCounter&) { ++copies; }
    CopyCounter(CopyCounter&&) noexcept {}
};
CopyCounter make_counter() { CopyCounter c; return c; }  // moved or elided, never copied
#if __cpp_implicit_move >= 202207L
std::unique_ptr<int> pass_through(std::unique_ptr<int>&& p) { return p; }  // C++23: p is an rvalue
#endif

bool contains(const int (&grid)[3][3], int value) {
    for (const auto& row : grid)
        for (int x : row)
            if (x == value) goto found;
    return false;
found:
    return true;
}

// MSVC doesn't implement P2324 (labels at the end of compound statements: "no" in
// Microsoft's conformance table); there's no feature-test macro, so test the compiler.
#if !(defined(_MSC_VER) && !defined(__clang__))
void report(int errors) {
    if (errors == 0) goto done;
    std::printf("%d errors\n", errors);
done:
}
#endif

#if __cpp_constexpr >= 202110L  // P2242; MSVC from VS 2022 17.14
constexpr int abs_or_minus_one(int n) {  // goto may appear in a constexpr function (C++23)...
    if (n < 0) goto negative;
    return n;
negative:
    return -1;
}
#endif
}  // namespace

// "Mental model": break/return/goto out of a scope destroy its locals in reverse order.
TEST(JumpStatements, JumpsRunDestructors) {
    trace.clear();
    for (int i = 0; i < 3; ++i) {
        Noisy a('a'), b('b');
        if (i == 0) break;
    }
    EXPECT_EQ(trace, "+a+b-b-a");

    trace.clear();
    [] { Noisy a('a'); Noisy b('b'); return; }();
    EXPECT_EQ(trace, "+a+b-b-a");

    // "Gotchas": jumping backward destroys the variable and constructs it again.
    trace.clear();
    int round = 0;
again:
    {
        Noisy n(static_cast<char>('0' + round));
        if (++round < 2) goto again;
    }
    EXPECT_EQ(trace, "+0-0+1-1");
}

// "break and continue": first_negative, and break/continue inside a switch in a loop.
TEST(JumpStatements, BreakAndContinue) {
    static_assert(first_negative(data) == 2);
    constexpr int none[] = {1, 2};
    static_assert(first_negative(none) == -1);

    std::string seen;
    for (int i = 0; i < 4; ++i) {
        switch (i) {
            case 1: break;       // leaves the switch only
            case 2: continue;    // goes to the loop
            default: seen += 'd';
        }
        seen += std::to_string(i);
    }
    EXPECT_EQ(seen, "d01d3");

    // "Gotchas": continue in do-while still evaluates the condition.
    int checks = 0, i = 0;
    do {
        ++i;
        continue;
    } while (++checks, i < 3);
    EXPECT_EQ(checks, 3);
}

// "return": braced result, returning a void expression, implicit move of locals.
TEST(JumpStatements, Return) {
    EXPECT_EQ(make_ids(), (std::vector<int>{1, 2, 3}));
    log_calls = 0;
    log_twice("x");
    EXPECT_EQ(log_calls, 2);
    CopyCounter::copies = 0;
    (void)make_counter();
    EXPECT_EQ(CopyCounter::copies, 0);
    CPPDOCS_REQUIRE_VERSION(__cpp_implicit_move, 202207L);
#if __cpp_implicit_move >= 202207L
    EXPECT_EQ(*pass_through(std::make_unique<int>(5)), 5);
#endif
}

// "goto out of nested loops": contains() and the ranges::contains equivalent agree.
TEST(JumpStatements, GotoOutOfNestedLoops) {
    constexpr int grid[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    EXPECT_TRUE(contains(grid, 5));
    EXPECT_FALSE(contains(grid, 10));
    CPPDOCS_REQUIRE(__cpp_lib_ranges_contains);
#ifdef __cpp_lib_ranges_contains
    EXPECT_TRUE(std::ranges::contains(grid | std::views::join, 5));
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 210000
    // libc++ 19 and 20 (Apple Clang 17's too): ranges::contains over views::join of a
    // built-in 2-D array reports a value that isn't there (run 41380889279, #209).
    GTEST_SKIP() << "libc++ " << _LIBCPP_VERSION << " bug: ranges::contains(grid | views::join, 10) "
                    "is true for a value not in the grid; fixed by libc++ 21";
#endif
    EXPECT_FALSE(std::ranges::contains(grid | std::views::join, 10));
#endif
}

// "break in an expansion statement": prints 1, then 120, then stops at the double.
TEST(JumpStatements, BreakInExpansionStatement) {
    CPPDOCS_REQUIRE(__cpp_expansion_statements);
#ifdef __cpp_expansion_statements
    auto out = cppdocs::capture_stdout([] {
        std::tuple<int, char, double> t{1, 'x', 3.5};
        template for (auto e : t) {
            if constexpr (std::is_same_v<decltype(e), double>) break;
            std::printf("%d\n", static_cast<int>(e));
        }
    });
    EXPECT_EQ(out, "1\n120\n");
#endif
}

// "Labels at the end of a block (C++23)": report(0) jumps straight to the label.
TEST(JumpStatements, LabelAtEndOfBlock) {
#if defined(_MSC_VER) && !defined(__clang__)
    GTEST_SKIP() << "MSVC doesn't implement labels at the end of a block (P2324)";
#else
    EXPECT_EQ(cppdocs::capture_stdout([] { report(0); }), "");
    EXPECT_EQ(cppdocs::capture_stdout([] { report(2); }), "2 errors\n");
#endif
}

// "Gotchas": goto in a constexpr function is fine as long as it isn't evaluated.
TEST(JumpStatements, GotoInConstexprFunction) {
    CPPDOCS_REQUIRE_VERSION(__cpp_constexpr, 202110L);
#if __cpp_constexpr >= 202110L
    static_assert(abs_or_minus_one(3) == 3);  // goto not taken during constant evaluation
    volatile int n = -5;
    EXPECT_EQ(abs_or_minus_one(n), -1);       // ...and runs normally at run time
#endif
}
