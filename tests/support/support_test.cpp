// Self-tests for the helpers in tests/support/.
#include <cstdio>
#include <iostream>
#include <print>
#include <version>

#include <gtest/gtest.h>

#include "support/capture.hpp"
#include "support/feature.hpp"

TEST(Support, CaptureSeesCoutPrintAndPrintfInOrder) {
    auto out = cppdocs::capture_stdout([] {
        std::cout << "a";
        std::print("b{}", 1);
        std::printf("c");
        std::cout << "d" << std::endl;
    });
    EXPECT_EQ(out, "ab1cd\n");
}

// Order is kept even if cout was unsynced from stdio before capturing (each
// test runs in its own process under ctest, so this doesn't leak).
TEST(Support, CaptureKeepsOrderWhenCoutIsUnsynced) {
    std::ios::sync_with_stdio(false);
    auto out = cppdocs::capture_stdout([] {
        std::cout << "e";
        std::print("f");
        std::cout << "g";
    });
    EXPECT_EQ(out, "efg");
}

TEST(Support, CaptureRestoresStdout) {
    { cppdocs::CaptureStdout c; std::cout << "hidden"; EXPECT_EQ(c.str(), "hidden"); }
    EXPECT_EQ(cppdocs::capture_stdout([] {}), "");
}

#define CPPDOCS_TEST_DEFINED_ 202207L
TEST(Support, FeatureValueParsesOrReportsMissing) {
    using cppdocs::detail::feature_value;
    static_assert(feature_value(CPPDOCS_EXPAND_STRINGIFY_(CPPDOCS_TEST_DEFINED_),
                                "CPPDOCS_TEST_DEFINED_") == 202207L);
    static_assert(feature_value(CPPDOCS_EXPAND_STRINGIFY_(CPPDOCS_TEST_UNDEFINED_),
                                "CPPDOCS_TEST_UNDEFINED_") == 0);
    EXPECT_EQ(cppdocs::detail::feature_skip_reason("__cpp_lib_x", 0, 1),
              "needs __cpp_lib_x (not defined by this toolchain)");
    EXPECT_EQ(cppdocs::detail::feature_skip_reason("__cpp_lib_x", 202207L, 202306L),
              "needs __cpp_lib_x >= 202306L (this toolchain has 202207L)");
}

TEST(Support, RequirePassesWhenDefined) {
    CPPDOCS_REQUIRE(__cplusplus);  // always defined, so this must not skip
    SUCCEED();
}
