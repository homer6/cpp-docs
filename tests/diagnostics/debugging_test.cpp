// Checks claims made by docs/examples/diagnostics/debugging.md.
//
// GCC 16 links this area with -lstdc++exp (see tests/CMakeLists.txt); GCC 15
// and libc++ have no <debugging>, so these skip there.
#include <version>

#ifdef __cpp_lib_debugging
#include <csignal>
#include <debugging>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Compiler support": GCC 16.2 advertises 202403L (replaceable is_debugger_present).
TEST(Debugging, FeatureMacroValue) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_debugging, 202403L);
    SUCCEED();
}

// "Stop only when someone is watching": with no debugger attached (as under
// ctest) is_debugger_present() is false and breakpoint_if_debugging() is a no-op.
// The page's replacement of is_debugger_present isn't tested here: on GCC 16.2
// it can't be linked into a program that also calls breakpoint() (see the page).
TEST(Debugging, BreakpointIfDebuggingIsNoOpUnwatched) {
    CPPDOCS_REQUIRE(__cpp_lib_debugging);
#ifdef __cpp_lib_debugging
    if (std::is_debugger_present()) GTEST_SKIP() << "running under a debugger";
    std::breakpoint_if_debugging();
    SUCCEED() << "still running";
#endif
}

// "Gotchas": breakpoint() without a debugger ends the program with SIGTRAP.
TEST(DebuggingDeathTest, BreakpointWithoutDebuggerTraps) {
    CPPDOCS_REQUIRE(__cpp_lib_debugging);
#if defined(_WIN32)
    GTEST_SKIP() << "Windows has no SIGTRAP (breakpoint() raises a breakpoint exception) and "
                    "GoogleTest has no KilledBySignal there";
#elif defined(__cpp_lib_debugging)
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    EXPECT_EXIT(std::breakpoint(), testing::KilledBySignal(SIGTRAP), "");
#endif
}
