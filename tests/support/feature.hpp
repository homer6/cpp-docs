// Feature gating: skip a test, with the reason, when the standard library or
// compiler doesn't define a feature-test macro (or defines an older value).
//
//   #include <version>
//   #include "support/feature.hpp"
//
//   TEST(FlatMap, KeysAreSorted) {
//       CPPDOCS_REQUIRE(__cpp_lib_flat_map);
//   #ifdef __cpp_lib_flat_map
//       ...code that needs std::flat_map...
//   #endif
//   }
//
// The macro only decides whether to skip; code that would not compile without
// the feature still needs the #ifdef guard. Include <version> (or the feature's
// header) first so library macros are defined. The skip message names the
// macro, e.g. "needs __cpp_lib_flat_map (not defined by this toolchain)", and
// tools/run_tests.sh prints it.
#pragma once

#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace cppdocs::detail {

// A defined feature macro expands to a number such as 202207L; an undefined
// one stays as its own name.
constexpr long feature_value(std::string_view expanded, std::string_view name) {
    if (expanded == name) return 0;
    long v = 0;
    for (char c : expanded) {
        if (c < '0' || c > '9') break;
        v = v * 10 + (c - '0');
    }
    return v;
}

inline std::string feature_skip_reason(std::string_view name, long have, long need) {
    std::string r = "needs " + std::string(name);
    if (need > 1) r += " >= " + std::to_string(need) + "L";
    if (have == 0)
        r += " (not defined by this toolchain)";
    else
        r += " (this toolchain has " + std::to_string(have) + "L)";
    return r;
}

}  // namespace cppdocs::detail

#define CPPDOCS_STRINGIFY_(x) #x
#define CPPDOCS_EXPAND_STRINGIFY_(x) CPPDOCS_STRINGIFY_(x)

// Both public macros stringify their argument themselves: forwarding `macro`
// to another macro would expand it to its value before it could be named.
#define CPPDOCS_REQUIRE_IMPL_(name, expanded, min)                                      \
    do {                                                                                \
        constexpr long cppdocs_have_ = ::cppdocs::detail::feature_value(expanded, name); \
        if (cppdocs_have_ < (min))                                                      \
            GTEST_SKIP() << ::cppdocs::detail::feature_skip_reason(name, cppdocs_have_,  \
                                                                   (min));              \
    } while (false)

// Skip unless feature-test macro `macro` is defined with a value >= min.
#define CPPDOCS_REQUIRE_VERSION(macro, min) \
    CPPDOCS_REQUIRE_IMPL_(#macro, CPPDOCS_EXPAND_STRINGIFY_(macro), min)

// Skip unless feature-test macro `macro` is defined at all.
#define CPPDOCS_REQUIRE(macro) \
    CPPDOCS_REQUIRE_IMPL_(#macro, CPPDOCS_EXPAND_STRINGIFY_(macro), 1L)
