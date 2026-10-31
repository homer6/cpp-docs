// Checks claims made by docs/examples/diagnostics/stacktrace.md.
//
// GCC 16 links this area with -lstdc++exp (see tests/CMakeLists.txt); Homebrew
// GCC 15 on macOS and libc++ don't define __cpp_lib_stacktrace, so these skip.
#include <string>
#include <version>

#ifdef __cpp_lib_stacktrace
#include <format>
#include <memory_resource>
#include <sstream>
#include <stacktrace>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

#ifdef __cpp_lib_stacktrace
namespace {
[[gnu::noinline]] std::stacktrace capture_here() { return std::stacktrace::current(); }
}  // namespace
#endif

// "Mental model": current() snapshots the calls leading here; frames report
// function name, file and line when debug info is present (Debug build: -g).
TEST(Stacktrace, FramesHaveNameFileLine) {
    CPPDOCS_REQUIRE(__cpp_lib_stacktrace);
#ifdef __cpp_lib_stacktrace
    auto trace = capture_here();
    ASSERT_FALSE(trace.empty());
    const std::stacktrace_entry& top = trace[0];
    EXPECT_NE(top.description().find("capture_here"), std::string::npos) << top.description();
    EXPECT_NE(top.source_file().find("stacktrace_test.cpp"), std::string::npos) << top.source_file();
    EXPECT_GT(top.source_line(), 0u);
    int n = 0;
    for (const std::stacktrace_entry& f : trace) n += static_cast<bool>(f);
    EXPECT_EQ(n, static_cast<int>(trace.size()));  // it's a container of entries
#endif
}

// "Examples": operator<< pretty-prints all frames; std::format has a formatter.
TEST(Stacktrace, StreamAndFormat) {
    CPPDOCS_REQUIRE(__cpp_lib_stacktrace);
#ifdef __cpp_lib_stacktrace
    auto trace = capture_here();
    std::ostringstream os;
    os << trace;
    EXPECT_NE(os.str().find("capture_here"), std::string::npos);
    EXPECT_EQ(std::format("{}", trace), os.str());
#endif
}

// "Gotchas": std::pmr::stacktrace takes a polymorphic allocator.
TEST(Stacktrace, PmrStacktrace) {
    CPPDOCS_REQUIRE(__cpp_lib_stacktrace);
#ifdef __cpp_lib_stacktrace
    std::pmr::monotonic_buffer_resource arena;
    auto t = std::pmr::stacktrace::current(std::pmr::polymorphic_allocator<std::stacktrace_entry>{&arena});
    EXPECT_FALSE(t.empty());
    EXPECT_EQ(t.get_allocator().resource(), &arena);
#endif
}

// "Declaration": current(skip, max_depth) drops the innermost frames and caps the depth.
TEST(Stacktrace, SkipAndMaxDepth) {
    CPPDOCS_REQUIRE(__cpp_lib_stacktrace);
#ifdef __cpp_lib_stacktrace
    auto full = std::stacktrace::current();
    auto capped = std::stacktrace::current(0, 1);
    EXPECT_EQ(capped.size(), 1u);
    auto skipped = std::stacktrace::current(1);
    EXPECT_EQ(skipped.size() + 1, full.size());
    EXPECT_FALSE(std::to_string(full[0]).empty());
#endif
}

// "In error reporting" + "Gotchas": a trace taken in a catch block shows the
// handler's stack, not the throw site.
#ifdef __cpp_lib_stacktrace
namespace {
[[gnu::noinline]] void thrower_fn() { throw 1; }
}  // namespace
#endif
TEST(Stacktrace, CatchBlockDoesNotShowThrowSite) {
    CPPDOCS_REQUIRE(__cpp_lib_stacktrace);
#ifdef __cpp_lib_stacktrace
    std::string text;
    try {
        thrower_fn();
    } catch (int) {
        text = std::to_string(std::stacktrace::current());
    }
    EXPECT_EQ(text.find("thrower_fn"), std::string::npos);
    EXPECT_NE(text.find("CatchBlockDoesNotShowThrowSite"), std::string::npos);
#endif
}
