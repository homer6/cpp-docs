// Checks claims made by docs/examples/datetime/durations.md.
#include <chrono>
#include <type_traits>

#include <gtest/gtest.h>

using namespace std::chrono_literals;
namespace chr = std::chrono;

// "Literals and arithmetic": mixed units yield the finer common type.
TEST(Durations, MixedArithmeticUsesFinerUnit) {
    auto total = 2h + 30min;
    static_assert(std::is_same_v<decltype(total), chr::minutes>);
    EXPECT_EQ(total.count(), 150);

    auto t = 1s + 500ms;
    static_assert(std::is_same_v<decltype(t), chr::milliseconds>);
    EXPECT_EQ(t, 1500ms);
    EXPECT_EQ((500ms).count(), 500);
}

// "Converting between units": duration_cast truncates; floor/round/ceil direct the rounding.
TEST(Durations, CastAndRounding) {
    EXPECT_EQ(chr::duration_cast<chr::milliseconds>(90s).count(), 90000);
    EXPECT_EQ(chr::duration_cast<chr::seconds>(1500ms), 1s);
    EXPECT_EQ(chr::duration_cast<chr::seconds>(-1500ms), -1s);  // toward zero
    EXPECT_EQ(chr::floor<chr::seconds>(1500ms), 1s);
    EXPECT_EQ(chr::round<chr::seconds>(1500ms), 2s);
    EXPECT_EQ(chr::ceil<chr::seconds>(1ms), 1s);
}

// "Floating-point durations": 1500ms converts implicitly to 1.5 seconds.
TEST(Durations, FloatingPointDuration) {
    chr::duration<double> secs = 1500ms;
    EXPECT_DOUBLE_EQ(secs.count(), 1.5);
}

// "Gotchas": lossy implicit conversions don't compile; lossless ones do.
TEST(Durations, LossyImplicitConversionRejected) {
    static_assert(!std::is_convertible_v<chr::milliseconds, chr::seconds>);
    static_assert(std::is_convertible_v<chr::seconds, chr::milliseconds>);
    static_assert(std::is_convertible_v<chr::milliseconds, chr::duration<double>>);
}

// "Mental model": duration = Rep count × Period ratio; days..years are C++20 aliases.
TEST(Durations, AliasPeriods) {
    static_assert(std::is_same_v<chr::milliseconds::period, std::milli>);
    static_assert(chr::hours::period::num == 3600);
    static_assert(chr::days::period::num == 86400);
    static_assert(chr::weeks::period::num == 7 * 86400);
    static_assert(chr::years::period::num == 31556952);  // average Gregorian year
    static_assert(chr::months::period::num * 12 == chr::years::period::num);
    EXPECT_EQ(chr::duration_cast<chr::hours>(chr::days{2}).count(), 48);
}
