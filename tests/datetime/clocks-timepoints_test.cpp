// Checks claims made by docs/examples/datetime/clocks-timepoints.md.
#include <chrono>
#include <ctime>
#include <format>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace ch = std::chrono;
using namespace std::chrono_literals;

namespace {
template <class A, class B>
concept Subtractable = requires(A a, B b) { a - b; };
template <class A, class B>
concept Comparable = requires(A a, B b) { a < b; };
}  // namespace

// "The clocks": steady_clock is monotonic; system_clock isn't marked steady.
TEST(ClocksTimepoints, SteadyIsMonotonic) {
    static_assert(ch::steady_clock::is_steady);
    static_assert(!ch::system_clock::is_steady);
    auto a = ch::steady_clock::now();
    auto b = ch::steady_clock::now();
    EXPECT_LE(a, b);
}

// "Gotchas": high_resolution_clock is an alias of another clock, and which one
// differs: libstdc++ aliases system_clock (not steady), libc++ steady_clock.
TEST(ClocksTimepoints, HighResolutionIsAnAlias) {
#if defined(_LIBCPP_VERSION)
    static_assert(std::is_same_v<ch::high_resolution_clock, ch::steady_clock>);
#elif defined(__GLIBCXX__)
    static_assert(std::is_same_v<ch::high_resolution_clock, ch::system_clock>);
    static_assert(!ch::high_resolution_clock::is_steady);
#else
    GTEST_SKIP() << "alias recorded only for libstdc++ and libc++";
#endif
}

// "Measuring elapsed time": now() - start is a duration that casts to ms/seconds.
TEST(ClocksTimepoints, ElapsedIsDuration) {
    auto start = ch::steady_clock::now();
    auto elapsed = ch::steady_clock::now() - start;
    static_assert(std::is_same_v<decltype(elapsed), ch::steady_clock::duration>);
    EXPECT_GE(ch::duration_cast<ch::milliseconds>(elapsed).count(), 0);
    EXPECT_GE(ch::duration<double>(elapsed).count(), 0.0);
}

// "time_point arithmetic": time_point + duration and time_point - time_point.
TEST(ClocksTimepoints, TimePointArithmetic) {
    auto tp1 = ch::steady_clock::now();
    auto deadline = tp1 + 5s;
    static_assert(std::is_same_v<decltype(deadline)::clock, ch::steady_clock>);
    EXPECT_EQ(deadline - tp1, 5s);
    EXPECT_LT(tp1, deadline);
}

// "Gotchas": time points from different clocks can't be compared or subtracted.
TEST(ClocksTimepoints, ClockSpecificTimePoints) {
    using S = ch::system_clock::time_point;
    using T = ch::steady_clock::time_point;
    static_assert(!Subtractable<S, T> && !Comparable<S, T>);
    static_assert(Subtractable<T, T>);
}

// "Wall-clock time" + "Gotchas": to_time_t loses sub-second precision; format
// prints a system time point directly.
TEST(ClocksTimepoints, ToTimeTAndFormat) {
    ch::sys_time<ch::milliseconds> tp{1'700'000'000'250ms};
    std::time_t t = ch::system_clock::to_time_t(tp);
    EXPECT_EQ(t, 1'700'000'000);
    EXPECT_EQ(ch::system_clock::from_time_t(t), ch::floor<ch::seconds>(tp));
    EXPECT_EQ(std::format("{:%Y-%m-%d %H:%M:%S}", ch::floor<ch::seconds>(tp)), "2023-11-14 22:13:20");
}

// "The clocks": utc_clock, tai_clock and gps_clock are leap-second-aware (C++20).
TEST(ClocksTimepoints, LeapSecondAwareClocks) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_chrono, 201907L);
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
    using namespace std::chrono;
    try {
        auto utc = clock_cast<utc_clock>(sys_days{2017y / January / 1d});
        // TAI was 37 s ahead of UTC from 2017; GPS is TAI − 19 s.
        auto tai = clock_cast<tai_clock>(utc);
        auto gps = clock_cast<gps_clock>(utc);
        EXPECT_EQ(clock_cast<utc_clock>(tai), utc);
        EXPECT_EQ(tai.time_since_epoch() - gps.time_since_epoch(),
                  (sys_days{1980y / January / 6d} - sys_days{1958y / January / 1d}) + 19s);
    } catch (const std::runtime_error& e) {
        GTEST_SKIP() << "leap-second data unavailable: " << e.what();
    }
#endif
}
