// Checks claims made by docs/examples/language/coroutines/coroutines.md.
#include <coroutine>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <version>

#if __has_include(<generator>)
#include <generator>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
// A minimal hand-written coroutine type: what C++20 makes you write yourself.
struct Counter {
    struct promise_type {
        int current = 0;
        Counter get_return_object() { return Counter{Handle::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int v) noexcept { current = v; return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { throw; }
    };
    using Handle = std::coroutine_handle<promise_type>;
    explicit Counter(Handle h) : h_(h) {}
    Counter(Counter&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~Counter() { if (h_) h_.destroy(); }
    bool next() { h_.resume(); return !h_.done(); }
    int value() const { return h_.promise().current; }
private:
    Handle h_;
};

std::string coro_trace;
Counter count_to(int n) {  // co_yield makes this a coroutine
    for (int i = 1; i <= n; ++i) {
        coro_trace += "y" + std::to_string(i) + ' ';
        co_yield i;  // suspend; `i` survives in the frame
    }
}
}  // namespace

#ifdef __cpp_lib_generator
namespace {
std::generator<int> fibonacci() {  // the page's example
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        std::tie(a, b) = std::tuple{b, a + b};
    }
}

int produced = 0;
std::generator<int> counting() {
    for (int i = 0;; ++i) {
        ++produced;
        co_yield i;
    }
}
}  // namespace
#endif

// "std::generator (C++23)": the first ten Fibonacci numbers are 0 1 1 2 3 5 8 13 21 34.
TEST(Coroutines, GeneratorFibonacciFirstTen) {
    CPPDOCS_REQUIRE(__cpp_lib_generator);
#ifdef __cpp_lib_generator
    std::vector<int> got;
    for (int x : fibonacci() | std::views::take(10)) got.push_back(x);
    EXPECT_EQ(got, (std::vector<int>{0, 1, 1, 2, 3, 5, 8, 13, 21, 34}));
#endif
}

// "std::generator (C++23)": lazy — values are computed on demand. take(n) stops the
// loop after n values, but its counted_iterator's last ++ resumes the coroutine
// once more, so n + 1 values are produced (and the n + 1th is never seen).
TEST(Coroutines, GeneratorIsLazy) {
    CPPDOCS_REQUIRE(__cpp_lib_generator);
#ifdef __cpp_lib_generator
    produced = 0;
    auto g = counting();
    EXPECT_EQ(produced, 0);  // nothing runs until iteration starts
    int n = 0;
    for (int x : std::move(g) | std::views::take(3)) n += x;  // a generator is move-only
    EXPECT_EQ(n, 0 + 1 + 2);
    EXPECT_EQ(produced, 4);
#endif
}

// "Gotchas": std::generator is single-pass (an input range, not a forward range).
TEST(Coroutines, GeneratorIsSinglePass) {
    CPPDOCS_REQUIRE(__cpp_lib_generator);
#ifdef __cpp_lib_generator
    static_assert(std::ranges::input_range<std::generator<int>>);
    static_assert(!std::ranges::forward_range<std::generator<int>>);
#endif
}

// "Mental model" / "The catch": with a hand-written promise_type, co_yield suspends,
// hands control back, and resumes with locals preserved.
TEST(Coroutines, HandWrittenPromiseSuspendsAndResumes) {
    coro_trace.clear();
    Counter c = count_to(3);
    EXPECT_EQ(coro_trace, "");  // initial_suspend: nothing ran yet
    std::string got;
    while (c.next()) {
        got += std::to_string(c.value()) + ' ';
        coro_trace += "| ";
    }
    EXPECT_EQ(got, "1 2 3 ");
    EXPECT_EQ(coro_trace, "y1 | y2 | y3 | ");  // caller and coroutine interleave
}

// "std::generator (C++23)": GCC 15's libstdc++ ships <generator>; Clang 22's libc++ doesn't yet.
TEST(Coroutines, GeneratorAvailabilityPerLibrary) {
#if defined(_LIBCPP_VERSION)
#ifdef __cpp_lib_generator
    ADD_FAILURE() << "libc++ now defines __cpp_lib_generator; update the page";
#endif
#elif defined(__GLIBCXX__) && defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 15
#ifndef __cpp_lib_generator
    ADD_FAILURE() << "expected libstdc++ (GCC 15+) to ship <generator>";
#endif
#else
    GTEST_SKIP() << "the page only states availability for GCC 15 libstdc++ and Clang 22 libc++";
#endif
}
