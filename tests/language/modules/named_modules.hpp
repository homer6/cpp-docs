// Skips a test, with the reason, when this build has no named modules.
// modules.cmake defines CPPDOCS_NAMED_MODULES when it builds units/ with
// FILE_SET CXX_MODULES, and CPPDOCS_NAMED_MODULES_SKIP="why not" otherwise.
#pragma once

#include <gtest/gtest.h>

#ifdef CPPDOCS_NAMED_MODULES
#define CPPDOCS_REQUIRE_NAMED_MODULES() static_cast<void>(0)
#else
#ifndef CPPDOCS_NAMED_MODULES_SKIP
#define CPPDOCS_NAMED_MODULES_SKIP "named modules aren't built by this configuration"
#endif
#define CPPDOCS_REQUIRE_NAMED_MODULES() GTEST_SKIP() << CPPDOCS_NAMED_MODULES_SKIP
#endif
