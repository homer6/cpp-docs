// Checks claims made by docs/examples/language-support/non-local-jumps.md.
#include <csetjmp>

#include <gtest/gtest.h>

namespace {
std::jmp_buf g_env;
int frames_left = 0;

[[gnu::noinline]] int depth(int n) {
    ++frames_left;
    if (n == 3) std::longjmp(g_env, 42);
    if (n > 100) return 0;  // never reached; keeps -Winfinite-recursion quiet
    return depth(n + 1) + 1;
}

// "Escaping from deep recursion": setjmp returns 0, then 42 after the longjmp.
int escape(int& attempts_out) {
    volatile int attempts = 0;
    switch (setjmp(g_env)) {
    case 0:
        attempts = attempts + 1;
        depth(0);
        return -1;  // not reached
    case 42:
        attempts_out = attempts;
        return 42;
    }
    return -2;
}

// "longjmp(env, 0) returns 1".
int zero_becomes_one() {
    std::jmp_buf env;
    int seen = 0;
    switch (setjmp(env)) {
    case 0:
        std::longjmp(env, 0);
    default:
        seen = 1;
    }
    return seen;
}

struct Plain { int plain, vol; };
[[gnu::noinline]] void deep() { std::longjmp(g_env, 1); }
Plain volatile_matters() {
    int plain = 0;
    volatile int vol = 0;
    if (setjmp(g_env) == 0) {
        plain = 7;
        vol = 7;
        deep();
    }
    return {plain, vol};
}
}  // namespace

// "Escaping from deep recursion" and "Mental model": four frames entered, all
// abandoned by one longjmp.
TEST(NonLocalJumps, EscapeFromDeepRecursion) {
    frames_left = 0;
    int attempts = 0;
    EXPECT_EQ(escape(attempts), 42);
    EXPECT_EQ(attempts, 1);  // volatile kept its value
    EXPECT_EQ(frames_left, 4);
}

// "longjmp(env, 0) returns 1".
TEST(NonLocalJumps, LongjmpZeroReturnsOne) {
    EXPECT_EQ(zero_becomes_one(), 1);
}

// "Gotchas": the volatile local keeps its post-setjmp value; the plain one is
// indeterminate (the page saw 7 at -O0, 0 at -O2 with GCC 15 and Clang 22).
TEST(NonLocalJumps, VolatileLocalSurvives) {
    Plain r = volatile_matters();
    EXPECT_EQ(r.vol, 7);
#ifndef __OPTIMIZE__
    EXPECT_EQ(r.plain, 7);  // what -O0 (this Debug build) produced
#endif
}
