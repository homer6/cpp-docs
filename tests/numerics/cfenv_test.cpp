// Checks claims made by docs/examples/numerics/cfenv.md.
//
// Operands are volatile (as on the page) so nothing folds at compile time;
// the harness builds at -O0 and doesn't use #pragma STDC FENV_ACCESS (GCC
// warns that it ignores it).
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>

#include <gtest/gtest.h>

namespace {
volatile double zero = 0.0, one = 1.0, three = 3.0;

std::string third() {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.20f", one / three);
    return buf;
}

double quiet_reciprocal(double x) {
    std::fenv_t env;
    std::feholdexcept(&env);
    volatile double r = 1.0 / x;
    bool divbyzero = std::fetestexcept(FE_DIVBYZERO);
    std::feclearexcept(FE_DIVBYZERO);
    std::feupdateenv(&env);
    return divbyzero ? 0.0 : r;
}

// Restores round-to-nearest even if an assertion fails mid-test.
struct RoundGuard {
    int saved = std::fegetround();
    ~RoundGuard() { std::fesetround(saved); }
};
}  // namespace

// "Clear, compute, test": 1/0 raises FE_DIVBYZERO, 1/3 FE_INEXACT, sqrt(-1) FE_INVALID.
TEST(Cfenv, ClearComputeTest) {
    volatile double r;
    std::feclearexcept(FE_ALL_EXCEPT);
    r = one / zero;
    EXPECT_EQ(std::fetestexcept(FE_DIVBYZERO | FE_INEXACT | FE_INVALID), FE_DIVBYZERO);
    std::feclearexcept(FE_ALL_EXCEPT);
    r = one / three;
    EXPECT_EQ(std::fetestexcept(FE_DIVBYZERO | FE_INEXACT | FE_INVALID), FE_INEXACT);
    std::feclearexcept(FE_ALL_EXCEPT);
    r = std::sqrt(-one);
    EXPECT_EQ(std::fetestexcept(FE_DIVBYZERO | FE_INEXACT | FE_INVALID), FE_INVALID);
    (void)r;
}

// "Mental model": flags are sticky until cleared; fetestexcept returns the set subset.
TEST(Cfenv, FlagsAreSticky) {
    std::feclearexcept(FE_ALL_EXCEPT);
    volatile double r = one / zero;
    r = one + one;  // exact: doesn't clear anything
    (void)r;
    EXPECT_TRUE(std::fetestexcept(FE_DIVBYZERO));
    EXPECT_EQ(std::feraiseexcept(FE_OVERFLOW), 0);
    EXPECT_EQ(std::fetestexcept(FE_OVERFLOW | FE_DIVBYZERO | FE_INVALID), FE_OVERFLOW | FE_DIVBYZERO);
    std::fexcept_t saved;
    EXPECT_EQ(std::fegetexceptflag(&saved, FE_ALL_EXCEPT), 0);
    std::feclearexcept(FE_ALL_EXCEPT);
    EXPECT_EQ(std::fetestexcept(FE_ALL_EXCEPT), 0);
    EXPECT_EQ(std::fesetexceptflag(&saved, FE_ALL_EXCEPT), 0);
    EXPECT_TRUE(std::fetestexcept(FE_OVERFLOW));
    std::feclearexcept(FE_ALL_EXCEPT);
}

// "Changing the rounding mode": the rint values and the printed 1/3 column
// (printed while the mode is active, as on the page).
TEST(Cfenv, RoundingModes) {
    RoundGuard guard;
    struct Row { int mode; double up, down; const char* third; };
    const Row rows[] = {
        {FE_TONEAREST, 2.0, -2.0, "0.33333333333333331483"},
        {FE_UPWARD, 3.0, -2.0, "0.33333333333333337035"},
        {FE_DOWNWARD, 2.0, -3.0, "0.33333333333333331482"},
        {FE_TOWARDZERO, 2.0, -2.0, "0.33333333333333331482"},
    };
    volatile double h = 2.5;
    for (const auto& r : rows) {
        ASSERT_EQ(std::fesetround(r.mode), 0);
        EXPECT_EQ(std::fegetround(), r.mode);
        EXPECT_EQ(std::rint(h), r.up) << r.mode;
        EXPECT_EQ(std::rint(-h), r.down) << r.mode;
        EXPECT_EQ(std::nearbyint(h), r.up) << r.mode;
        EXPECT_EQ(std::round(-h), -3.0);  // round and lround ignore the mode
        EXPECT_EQ(std::lround(h), 3L);
        EXPECT_EQ(third(), r.third) << r.mode;
    }
}

// "Changing the rounding mode": the last digit of the 1/3 column is printf's
// own rounding. Nearest, downward and toward-zero compute the same double
// (the nearest double is below 1/3); only upward computes the next one up.
TEST(Cfenv, OneThirdDoublesPerMode) {
    RoundGuard guard;
    auto div_in = [](int mode) {
        std::fesetround(mode);
        volatile double q = one / three;
        std::fesetround(FE_TONEAREST);
        return double{q};
    };
    double nearest = div_in(FE_TONEAREST);
    EXPECT_EQ(div_in(FE_DOWNWARD), nearest);
    EXPECT_EQ(div_in(FE_TOWARDZERO), nearest);
    EXPECT_EQ(div_in(FE_UPWARD), std::nextafter(nearest, 1.0));
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.20f", nearest);
    EXPECT_STREQ(buf, "0.33333333333333331483");  // the downward row's value, printed to nearest
}

// "Changing the rounding mode": nearbyint never raises FE_INEXACT; rint does.
TEST(Cfenv, NearbyintDoesNotRaiseInexact) {
    volatile double h = 2.5;
    std::feclearexcept(FE_ALL_EXCEPT);
    volatile double a = std::nearbyint(h);
    EXPECT_FALSE(std::fetestexcept(FE_INEXACT));
    volatile double b = std::rint(h);
    EXPECT_TRUE(std::fetestexcept(FE_INEXACT));
    (void)a, (void)b;
}

// "Hiding intermediate flags": r=0 and the caller doesn't see FE_DIVBYZERO,
// but other flags raised inside are re-raised by feupdateenv.
TEST(Cfenv, HoldAndUpdateEnv) {
    std::feclearexcept(FE_ALL_EXCEPT);
    double r = quiet_reciprocal(zero);
    EXPECT_EQ(r, 0.0);
    EXPECT_FALSE(std::fetestexcept(FE_DIVBYZERO));
    r = quiet_reciprocal(three);  // 1/3 is inexact: that flag survives
    EXPECT_TRUE(std::fetestexcept(FE_INEXACT));
    std::feclearexcept(FE_ALL_EXCEPT);
}

// "How does <cmath> report errors here?": macOS reports 2 (flags only), glibc 3.
TEST(Cfenv, MathErrhandlingPerPlatform) {
#if defined(__APPLE__)
    EXPECT_EQ(math_errhandling, 2);
#elif defined(__GLIBC__)
    EXPECT_EQ(math_errhandling, 3);
#else
    GTEST_SKIP() << "math_errhandling is implementation-defined; page records macOS and glibc";
#endif
}

// "Mental model": the environment is per thread. Whether a new std::thread
// starts with its creator's environment depends on the platform: glibc copies
// it, macOS starts the thread in the default (round-to-nearest) environment.
TEST(Cfenv, EnvironmentIsPerThread) {
    RoundGuard guard;
    ASSERT_EQ(std::fesetround(FE_UPWARD), 0);
    int seen = -1;
    std::thread t{[&] {
        seen = std::fegetround();
        std::fesetround(FE_DOWNWARD);
    }};
    t.join();
    EXPECT_EQ(std::fegetround(), FE_UPWARD);  // the child's change didn't leak back
#if defined(__APPLE__)
    EXPECT_EQ(seen, FE_TONEAREST);
#elif defined(__GLIBC__)
    EXPECT_EQ(seen, FE_UPWARD);
#else
    GTEST_SKIP() << "new-thread environment recorded only for macOS and glibc";
#endif
}
