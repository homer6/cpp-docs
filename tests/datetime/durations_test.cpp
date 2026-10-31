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
