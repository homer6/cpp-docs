// Checks claims made by docs/examples/diagnostics/stacktrace.md.
//
// libstdc++ needs -lstdc++exp (GCC 14+), which tests/CMakeLists.txt links where a
// try-link shows it's needed. Homebrew GCC 15 on macOS and libc++ don't define
// __cpp_lib_stacktrace, so these skip there.
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

// FreeBSD with pkg GCC 14's libstdc++: the frames are captured, but every entry's
// description, file and line are empty (run 41380889279, #209). The tests that read
// them skip there with this reason.
#if defined(__FreeBSD__) && defined(__GLIBCXX__)
#define CPPDOCS_STACKTRACE_NO_SYMBOLS() \
    GTEST_SKIP() << "FreeBSD, libstdc++ (GCC " << __GNUC__ << "): stacktrace entries carry no " \
                    "description, file or line"
#else
#define CPPDOCS_STACKTRACE_NO_SYMBOLS() (void)0
#endif

// "Mental model": current() snapshots the calls leading here; frames report
// function name, file and line when debug info is present (Debug build: -g).
TEST(Stacktrace, FramesHaveNameFileLine) {
    CPPDOCS_REQUIRE(__cpp_lib_stacktrace);
    CPPDOCS_STACKTRACE_NO_SYMBOLS();
#ifdef __cpp_lib_stacktrace
    auto trace = capture_here();
    ASSERT_FALSE(trace.empty());
    const std::stacktrace_entry& top = trace[0];
    EXPECT_NE(top.description().find("capture_here"), std::string::npos) << top.description();
    EXPECT_NE(top.source_file().find("stacktrace_test.cpp"), std::string::npos) << top.source_file();
    EXPECT_GT(top.source_line(), 0u);
    std::size_t n = 0, empty = 0;
    for (const std::stacktrace_entry& f : trace) {
        ++n;
        empty += !static_cast<bool>(f);
    }
    EXPECT_EQ(n, trace.size());  // it's a container of entries
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 16
    // libstdc++ 14 and 15 end the trace with one empty entry (libbacktrace's
    // outermost pc); GCC 16 drops it. An empty entry is allowed, so tolerate it.
    EXPECT_LE(empty, 1u);
    if (empty == 1) EXPECT_FALSE(static_cast<bool>(trace[trace.size() - 1]));
#else
    EXPECT_EQ(empty, 0u);  // every frame refers to a real location
#endif
#endif
}

// "Examples": operator<< pretty-prints all frames; std::format has a formatter.
TEST(Stacktrace, StreamAndFormat) {
    CPPDOCS_REQUIRE(__cpp_lib_stacktrace);
    CPPDOCS_STACKTRACE_NO_SYMBOLS();
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
    CPPDOCS_STACKTRACE_NO_SYMBOLS();
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
#if defined(_MSC_VER)
    GTEST_SKIP() << "MSVC ABI (MSVC, clang-cl): the catch block runs before the throwing frames' "
                    "stack is popped, so a trace taken there can still show the throw site";
#elif defined(__cpp_lib_stacktrace)
    CPPDOCS_STACKTRACE_NO_SYMBOLS();
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
