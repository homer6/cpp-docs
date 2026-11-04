// Checks claims made by docs/security/language-versions.md.
#include <cstddef>
#include <utility>
#include <vector>  // pulls in the library's configuration (c++config.h, __config)
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Language version versus toolchain": libstdc++ 15+ defines _GLIBCXX_ASSERTIONS by
// default only in unoptimized builds; optimized builds get no library checks unless asked.
TEST(SecurityLanguageVersions, LibstdcxxAssertionsDefaultOnlyWhenUnoptimized) {
#if !defined(__GLIBCXX__)
    GTEST_SKIP() << "libstdc++ only (this build uses another standard library)";
#elif !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE < 15
    GTEST_SKIP() << "the unoptimized-build default is new in libstdc++ 15 (GCC 15 release notes)";
#elif defined(_GLIBCXX_NO_ASSERTIONS)
    GTEST_SKIP() << "_GLIBCXX_NO_ASSERTIONS turns the default off in this build";
#elif defined(__OPTIMIZE__)
    GTEST_SKIP() << "optimized build: the default doesn't apply (assertions only if the build defines them)";
#else
#  if defined(_GLIBCXX_ASSERTIONS)
    SUCCEED();
#  else
    ADD_FAILURE() << "libstdc++ 15+ should define _GLIBCXX_ASSERTIONS in an unoptimized build";
#  endif
#endif
}

// "What each standard adds": before C++20 a signed/unsigned comparison converts the
// negative value, so it passes a size check; std::cmp_less and std::in_range compare values.
TEST(SecurityLanguageVersions, IntegerComparisonCatchesNegativeLength) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_integer_comparison_functions, 202002L);
#if defined(__cpp_lib_integer_comparison_functions)
    int requested = -1;
    std::size_t limit = 4096;
    EXPECT_FALSE(static_cast<std::size_t>(requested) <= limit);  // -1 became SIZE_MAX
    EXPECT_TRUE(std::cmp_less(requested, 0));
    EXPECT_TRUE(std::cmp_less_equal(requested, limit));
    EXPECT_FALSE(std::in_range<std::size_t>(requested));
#endif
}

// "What each standard adds": the C++17 facilities and the values the table gives.
TEST(SecurityLanguageVersions, Cxx17FacilitiesAndMacros) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_optional, 201606L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_variant, 201606L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_string_view, 201606L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_to_chars, 201611L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_filesystem, 201703L);
    SUCCEED();
}

// "What each standard adds": the C++20 facilities and the values the table gives.
TEST(SecurityLanguageVersions, Cxx20FacilitiesAndMacros) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_span, 202002L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_ranges, 201911L);
    CPPDOCS_REQUIRE_VERSION(__cpp_concepts, 201907L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_jthread, 201911L);
    CPPDOCS_REQUIRE_VERSION(__cpp_impl_coroutine, 201902L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_integer_comparison_functions, 202002L);
    SUCCEED();
}

// "What each standard adds": the C++23 facilities and the values the table gives.
TEST(SecurityLanguageVersions, Cxx23FacilitiesAndMacros) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_expected, 202202L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_mdspan, 202207L);
    SUCCEED();
}

// "C++26: contracts and library hardening": the hardened-library macros (P3471) aren't
// defined by the libraries we test yet; this records which toolchains have them.
TEST(SecurityLanguageVersions, Cxx26HardenedLibraryMacros) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_hardened_array, 202502L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_hardened_vector, 202502L);
    SUCCEED();
}

// "C++26: contracts and library hardening": contract assertions (P2900), __cpp_contracts.
TEST(SecurityLanguageVersions, Cxx26ContractsMacro) {
    CPPDOCS_REQUIRE_VERSION(__cpp_contracts, 202502L);
    SUCCEED();
}
