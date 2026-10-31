// Checks claims made by docs/examples/io/syncstream.md.
//
// libc++ 22 declares std::osyncstream only with -fexperimental-library, which
// the harness doesn't pass, so these skip on clang-22.
#include <algorithm>
#include <barrier>
#include <cstdio>
#include <ostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <version>

#ifdef __cpp_lib_syncbuf
#include <syncstream>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Compiler support": libstdc++ (GCC 15.2 and 16.2) defines 201803L.
TEST(Syncstream, FeatureMacro) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_syncbuf, 201803L);
    SUCCEED();
}

// "When does the text arrive?": every assert from the page.
TEST(Syncstream, WhenDoesTextArrive) {
    CPPDOCS_REQUIRE(__cpp_lib_syncbuf);
#ifdef __cpp_lib_syncbuf
    std::ostringstream sink;
    {
        std::osyncstream out(sink);
        out << "first ";
        EXPECT_EQ(sink.str(), "");
        out.emit();
        EXPECT_EQ(sink.str(), "first ");
        out << std::emit_on_flush << "second" << std::flush;
        EXPECT_EQ(sink.str(), "first second");
        out << std::noemit_on_flush << " third" << std::flush;
        EXPECT_EQ(sink.str(), "first second");
    }
    EXPECT_EQ(sink.str(), "first second third");
#endif
}

// "Gotchas": endl doesn't emit by default; flush_emit does right away.
TEST(Syncstream, EndlRemembersFlushEmitEmits) {
    CPPDOCS_REQUIRE(__cpp_lib_syncbuf);
#ifdef __cpp_lib_syncbuf
    std::ostringstream sink;
    std::osyncstream out(sink);
    out << "a" << std::endl;
    EXPECT_EQ(sink.str(), "");
    out << "b" << std::flush_emit;
    EXPECT_EQ(sink.str(), "a\nb");
#endif
}

// "Declaration": the ostream& constructor wraps os.rdbuf(), reported by get_wrapped().
TEST(Syncstream, GetWrapped) {
    CPPDOCS_REQUIRE(__cpp_lib_syncbuf);
#ifdef __cpp_lib_syncbuf
    std::ostringstream sink;
    std::osyncstream out(sink);
    EXPECT_EQ(out.get_wrapped(), sink.rdbuf());
#endif
}

// "Whole lines from many threads": each temporary emits its line whole. The
// threads start together on a barrier so they really contend.
TEST(Syncstream, WholeLinesFromManyThreads) {
    CPPDOCS_REQUIRE(__cpp_lib_syncbuf);
#ifdef __cpp_lib_syncbuf
    constexpr int kThreads = 4, kLines = 200;
    std::ostringstream sink;
    std::barrier start{kThreads};
    {
        std::vector<std::jthread> workers;
        for (int i = 0; i < kThreads; ++i)
            workers.emplace_back([&, i] {
                start.arrive_and_wait();
                for (int j = 0; j < kLines; ++j)
                    std::osyncstream(sink) << "thread " << i << " says " << "hi " << j << '\n';
            });
    }
    std::istringstream in{sink.str()};
    std::vector<int> count(kThreads);
    int total = 0;
    for (std::string line; std::getline(in, line); ++total) {
        int t = -1, j = -1;
        ASSERT_EQ(std::sscanf(line.c_str(), "thread %d says hi %d", &t, &j), 2) << line;
        ASSERT_TRUE(t >= 0 && t < kThreads);
        ++count[t];
    }
    EXPECT_EQ(total, kThreads * kLines);
    EXPECT_TRUE(std::all_of(count.begin(), count.end(), [](int c) { return c == kLines; }));
#endif
}
