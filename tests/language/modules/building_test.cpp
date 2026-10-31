// Checks claims made by docs/examples/language/modules/building.md.
//
// CMake provides the std module only when it can: CMake 4.5+ for any C++20
// target it scans (GCC 16+ on macOS), or CMake 4.3/4.4 with the experimental
// gate that the top-level CMakeLists.txt sets. modules.cmake defines
// CPPDOCS_IMPORT_STD in that case and CPPDOCS_IMPORT_STD_SKIP="why not" otherwise.
#include <climits>
#include <string>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "language/modules/named_modules.hpp"
#include "support/feature.hpp"

#ifdef CPPDOCS_IMPORT_STD
import stdlib_demo;
import stdcompat_demo;
#endif

#ifndef CPPDOCS_IMPORT_STD_SKIP
#define CPPDOCS_IMPORT_STD_SKIP "import std isn't available in this configuration"
#endif

// "import std; and import std.compat;": a module unit can import std and hand std types to an importer that #includes the headers.
TEST(ImportStd, ModuleUsingImportStd) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_modules, 202207L);
#ifndef CPPDOCS_IMPORT_STD
    GTEST_SKIP() << CPPDOCS_IMPORT_STD_SKIP;
#else
    std::vector<std::string> parts{"im", "port"};
    EXPECT_EQ(joined(parts), "import");       // same std::string entity on both sides
#endif
}

// "import std; and import std.compat;" (neither exports macros): the module needed <climits> in its global module fragment for INT_MAX.
TEST(ImportStd, MacrosNeedTheHeader) {
#ifndef CPPDOCS_IMPORT_STD
    GTEST_SKIP() << CPPDOCS_IMPORT_STD_SKIP;
#else
    EXPECT_EQ(int_max(), INT_MAX);
#endif
}

// "import std; and import std.compat;": std.compat declares the global-namespace C names.
TEST(ImportStd, StdCompatHasGlobalCNames) {
#ifndef CPPDOCS_IMPORT_STD
    GTEST_SKIP() << CPPDOCS_IMPORT_STD_SKIP;
#else
    EXPECT_EQ(c_strlen("abc"), 3u);
#endif
}

// modules.md "Header units": not built here.
TEST(ImportStd, HeaderUnits) {
    GTEST_SKIP() << "header units (import <vector>;): CMake doesn't support them "
                    "(cmake-cxxmodules(7), Limitations); checked by tools/check_snippets.py instead";
}
