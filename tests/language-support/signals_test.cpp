// Checks claims made by docs/examples/language-support/signals.md.
// Handlers only touch volatile sig_atomic_t and lock-free atomics, and every
// test restores the previous disposition.
#include <atomic>
#include <csignal>

#include <gtest/gtest.h>

namespace {
volatile std::sig_atomic_t g_signal = 0;
std::atomic<int> g_count{0};
static_assert(std::atomic<int>::is_always_lock_free);

extern "C" void on_signal(int sig) {
    g_signal = sig;
    g_count.fetch_add(1, std::memory_order_relaxed);
}

struct Restore {
    int sig;
    void (*prev)(int);
    ~Restore() { std::signal(sig, prev); }
};
}  // namespace

// "Set a flag, react outside the handler": the handler runs before raise returns.
TEST(Signals, HandlerRunsBeforeRaiseReturns) {
    g_signal = 0;
    g_count = 0;
    auto prev = std::signal(SIGINT, on_signal);
    ASSERT_NE(prev, SIG_ERR);
    Restore r{SIGINT, prev};
    EXPECT_EQ(std::raise(SIGINT), 0);
    EXPECT_EQ(g_signal, SIGINT);
    EXPECT_EQ(g_count, 1);
    EXPECT_EQ(SIGINT, 2);  // the printed "got signal 2"
}

// "Set a flag": SIG_IGN ignores SIGTERM; the previous handler was SIG_DFL.
TEST(Signals, IgnoreAndPreviousDefault) {
    auto prev = std::signal(SIGTERM, SIG_IGN);
    Restore r{SIGTERM, prev};
    EXPECT_EQ(prev, SIG_DFL);
    EXPECT_EQ(std::raise(SIGTERM), 0);  // still running
}

// "Gotchas": macOS and glibc keep the handler installed after it runs.
TEST(Signals, HandlerStaysInstalled) {
#if defined(__APPLE__) || defined(__GLIBC__)
    g_count = 0;
    Restore r{SIGINT, std::signal(SIGINT, on_signal)};
    std::raise(SIGINT);
    std::raise(SIGINT);
    EXPECT_EQ(g_count, 2);
    EXPECT_EQ(std::signal(SIGINT, on_signal), on_signal);  // still ours
#else
    GTEST_SKIP() << "whether the handler is reset is implementation-defined";
#endif
}

// "Declaration": the six standard signals are distinct.
TEST(Signals, SixStandardSignals) {
    int sigs[] = {SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM};
    for (int i = 0; i < 6; ++i)
        for (int j = i + 1; j < 6; ++j) EXPECT_NE(sigs[i], sigs[j]);
}

// "Gotchas": destructors don't run when the default action terminates the process.
TEST(SignalsDeathTest, DefaultActionTerminates) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
#if defined(_WIN32)
    // The UCRT's default action ends the program with exit code 3 for every signal,
    // and GoogleTest has no KilledBySignal on Windows.
    EXPECT_EXIT(
        {
            std::signal(SIGTERM, SIG_DFL);
            std::raise(SIGTERM);
        },
        testing::ExitedWithCode(3), "");
#else
    EXPECT_EXIT(
        {
            std::signal(SIGTERM, SIG_DFL);
            std::raise(SIGTERM);
        },
        testing::KilledBySignal(SIGTERM), "");
#endif
}
