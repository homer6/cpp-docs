// Checks claims made by docs/examples/language/modules/modules.md.
//
// The module units live in units/ and are built by modules.cmake with CMake's
// FILE_SET CXX_MODULES. This file is scanned for imports (CXX_SCAN_FOR_MODULES),
// so CMake compiles the modules first. Without named-module support (CMake
// older than 3.28, a non-Ninja generator, or a compiler CMake can't scan with)
// the tests skip and say why.
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "language/modules/named_modules.hpp"
#include "support/feature.hpp"

#ifdef CPPDOCS_NAMED_MODULES
import math;
import shapes;
import config;
import geo;
import counter;
import token;
import wrapper;
#ifdef CPPDOCS_PRIVATE_MODULE_FRAGMENT
import tally;
#endif
#endif

// building.md "import std; and import std.compat;": the library advertises the std module with __cpp_lib_modules (P2465, 202207L).
TEST(Modules, ImportStdIsAdvertisedByLibrary) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_modules, 202207L);
    SUCCEED() << "this standard library ships `import std;`";
}

// "Interface and implementation units": exported functions, one defined in math.cpp.
TEST(Modules, InterfaceAndImplementationUnit) {
    CPPDOCS_REQUIRE_NAMED_MODULES();
#ifdef CPPDOCS_NAMED_MODULES
    EXPECT_EQ(add(2, 3), 5);
    EXPECT_EQ(add_twice(1, 1), 4);   // calls the non-exported twice() inside the module
    EXPECT_EQ(add(2, 3) + add_twice(1, 1) - 9, 0);
#endif
}

// "Export forms": a class, a function, an export block, an exported namespace, an exported using-declaration.
TEST(Modules, ExportForms) {
    CPPDOCS_REQUIRE_NAMED_MODULES();
#ifdef CPPDOCS_NAMED_MODULES
    Point p{3, 4};
    EXPECT_DOUBLE_EQ(length(p), 5.0);
    EXPECT_DOUBLE_EQ(area(1.0), pi);
    EXPECT_GT(pi, 3.14);
    EXPECT_EQ(geo_ns::square(3), 9);
    EXPECT_DOUBLE_EQ(sqrt(16.0), 4.0);   // the re-exported std::sqrt, found unqualified
#endif
}

// "The global and private module fragments": the module's #define doesn't reach the importer; its value does.
TEST(Modules, GlobalModuleFragmentMacrosDontLeak) {
    CPPDOCS_REQUIRE_NAMED_MODULES();
#ifdef CPPDOCS_NAMED_MODULES
#ifdef CONFIG_BUFFER_SIZE
    ADD_FAILURE() << "CONFIG_BUFFER_SIZE leaked through `import config;`";
#endif
    EXPECT_EQ(buffer_size, 64u);
#endif
}

// "The global and private module fragments": importers call next(); its state lives after `module :private;`.
TEST(Modules, PrivateModuleFragment) {
    CPPDOCS_REQUIRE_NAMED_MODULES();
#ifndef CPPDOCS_PRIVATE_MODULE_FRAGMENT
    GTEST_SKIP() << "private module fragment: GCC 15/16 say \"sorry, unimplemented\"";
#else
    EXPECT_EQ(next(), 1);
    EXPECT_EQ(next(), 2);
#endif
}

// "Partitions": `import geo;` sees what the primary interface re-exports from :types and :ops.
TEST(ModulePartitions, ReexportedInterfacePartitions) {
    CPPDOCS_REQUIRE_NAMED_MODULES();
#ifdef CPPDOCS_NAMED_MODULES
    geo::point p{3, -4};                      // from geo:types
    EXPECT_EQ(geo::manhattan(p), 7);          // declared in geo:ops, defined in geo.cpp via geo:detail
    EXPECT_EQ(geo::manhattan({3, -4}) == 7 ? 0 : 1, 0);
#endif
}

// "Module linkage, visibility and reachability": a global-module step() of our own doesn't clash with counter's module-linkage step().
// clang-cl can't link it: Clang's Microsoft-ABI mangling leaves out module
// attachment (llvm/llvm-project#174064), so both are ?step@@YAHXZ.
#if defined(_MSC_VER) && defined(__clang__)
#define CPPDOCS_MODULE_LINKAGE_MANGLED 0
#else
#define CPPDOCS_MODULE_LINKAGE_MANGLED 1
int step() { return 100; }
#endif

// "Module linkage, visibility and reachability": module linkage — the program links, and each step() is its own function.
TEST(ModuleLinkage, ModuleLinkageNamesDontClash) {
    CPPDOCS_REQUIRE_NAMED_MODULES();
#if !CPPDOCS_MODULE_LINKAGE_MANGLED
    GTEST_SKIP() << "clang-cl's Microsoft-ABI mangling leaves out module attachment "
                    "(llvm/llvm-project#174064): a global step() would clash with counter's";
#elif defined(CPPDOCS_NAMED_MODULES)
    EXPECT_EQ(next(1), 3);                    // counter.cpp uses counter's step() == 2
    EXPECT_EQ(step(), 100);                   // ours
    EXPECT_EQ(next(1) + step(), 103);
#endif
}

// "Module linkage, visibility and reachability": the non-exported Token is usable through auto and decltype.
TEST(ModuleLinkage, ReachableButNotVisible) {
    CPPDOCS_REQUIRE_NAMED_MODULES();
#ifdef CPPDOCS_NAMED_MODULES
    auto t = make_token(21);
    decltype(t) copy = t;
    EXPECT_EQ(t.twice(), 42);
    EXPECT_EQ(copy.id, 21);
    static_assert(std::is_aggregate_v<decltype(t)>);
#endif
}

// "Module linkage, visibility and reachability" (visibility): wrapper uses base internally; we only call wrapped().
TEST(ModuleLinkage, NonExportedImportStillWorksInside) {
    CPPDOCS_REQUIRE_NAMED_MODULES();
#ifdef CPPDOCS_NAMED_MODULES
    EXPECT_EQ(wrapped(), 2);
#endif
}

// "Module linkage, visibility and reachability" (reachability): an exported template can use a non-exported helper when instantiated here.
TEST(ModuleLinkage, ExportedTemplateUsesNonExportedHelper) {
    CPPDOCS_REQUIRE_NAMED_MODULES();
#ifdef CPPDOCS_NAMED_MODULES
    EXPECT_EQ(scaled(4), 40);
    EXPECT_EQ(scaled(4L), 40L);
#endif
}
