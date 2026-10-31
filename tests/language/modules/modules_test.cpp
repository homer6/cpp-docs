// Checks claims made by docs/examples/language/modules/modules.md.
//
// The harness builds ordinary (non-module) translation units, so the page's
// `export module math;` / `import math;` examples aren't compiled here; that
// needs CMake CXX_MODULES file sets (and -fmodules on GCC 15). These tests check
// what a plain TU can observe: the toolchain's module feature-test macros.
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "import std; (C++23)": the library advertises the std module with __cpp_lib_modules (P2465, 202207L).
TEST(Modules, ImportStdIsAdvertisedByLibrary) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_modules, 202207L);
    SUCCEED() << "this standard library ships `import std;`";
}

// "A module interface unit" / "Using it": named modules aren't built by this harness.
TEST(Modules, NamedModuleImport) {
    GTEST_SKIP() << "named modules (export module math; import math;) need CMake "
                    "CXX_MODULES file sets; not built by this harness";
}
