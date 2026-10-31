// Checks claims made by docs/examples/datetime/calendar-timezones.md.
//
// Time-zone tests skip, with the reason, when the toolchain has no tzdb support
// or the IANA data can't be loaded.
#include <chrono>
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

using namespace std::chrono;

// "Calendar types": typed fields, ok(), and operator/ in any sensible order.
TEST(CalendarTimezones, CalendarTypes) {
    year_month_day ymd{2026y, May, 26d};
    EXPECT_EQ(ymd.year(), 2026y);
    EXPECT_EQ(ymd.month(), May);
    EXPECT_TRUE(ymd.ok());
    year_month_day d1 = 2026y / May / 26d;
    year_month_day d2 = May / 26d / 2026y;
    EXPECT_EQ(d1, ymd);
    EXPECT_EQ(d2, ymd);
    year_month_day eom = 2026y / February / last;
    EXPECT_EQ(eom.day(), 28d);
    EXPECT_EQ(year_month_day{2028y / February / last}.day(), 29d);
}

// "Gotchas": 2026y/February/30d is constructible but not ok().
TEST(CalendarTimezones, InvalidDateNotOk) {
    year_month_day bad = 2026y / February / 30d;
    EXPECT_FALSE(bad.ok());
}

// "Converting to/from a time point": sys_days round-trips, differences are days,
// weekday of 2026-05-26 is Tuesday, and sys_days is UTC midnight.
TEST(CalendarTimezones, SysDaysBridge) {
    sys_days tp = 2026y / May / 26d;
    EXPECT_EQ(year_month_day{tp}, 2026y / May / 26d);
    auto days_between = sys_days{2026y / May / 26d} - sys_days{2026y / January / 1d};
    EXPECT_EQ(days_between, days{145});
    EXPECT_EQ(weekday{tp}, Tuesday);
    EXPECT_EQ(std::format("{:%Y-%m-%d %H:%M:%S}", sys_seconds{tp}), "2026-05-26 00:00:00");
}

// "Formatting (and parsing)": the date format and parse round-trip.
TEST(CalendarTimezones, FormatAndParse) {
    EXPECT_EQ(std::format("{:%Y-%m-%d}", 2026y / May / 26d), "2026-05-26");
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_chrono, 201907L);
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
    sys_seconds tp;
    std::istringstream in{"2026-05-26 12:00:00"};
    in >> parse("%Y-%m-%d %H:%M:%S", tp);
    ASSERT_FALSE(in.fail());
    EXPECT_EQ(tp, sys_days{2026y / May / 26d} + 12h);
#endif
}

// "Gotchas": %Z on a bare system_clock time point always prints UTC, not the local zone.
TEST(CalendarTimezones, BareSysTimeZoneIsUtc) {
    EXPECT_EQ(std::format("{:%Z %z}", sys_seconds{sys_days{2026y / May / 26d}}), "UTC +0000");
}

// "Time zones": named zones convert one instant, the database applies DST, and
// %Z prints the zone's abbreviation.
TEST(CalendarTimezones, ZonedTime) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_chrono, 201907L);
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
    try {
        (void)get_tzdb();
    } catch (const std::runtime_error& e) {
        GTEST_SKIP() << "IANA time-zone data unavailable: " << e.what();
    }
    sys_seconds summer = sys_days{2026y / May / 26d} + 16h;   // 12:00 EDT
    sys_seconds winter = sys_days{2026y / January / 15d} + 17h;  // 12:00 EST
    zoned_time ny{"America/New_York", summer};
    zoned_time tokyo{"Asia/Tokyo", ny.get_sys_time()};
    EXPECT_EQ(ny.get_sys_time(), tokyo.get_sys_time());  // same instant
    EXPECT_EQ(ny.get_local_time().time_since_epoch(), (sys_days{2026y / May / 26d} + 12h).time_since_epoch());
    EXPECT_EQ(std::format("{:%Y-%m-%d %H:%M:%S %Z}", ny), "2026-05-26 12:00:00 EDT");
    EXPECT_EQ(std::format("{:%H:%M %Z}", tokyo), "01:00 JST");
    EXPECT_EQ(std::format("{:%H:%M %Z}", zoned_time{"America/New_York", winter}), "12:00 EST");
    EXPECT_EQ(locate_zone("Asia/Tokyo")->name(), "Asia/Tokyo");
    EXPECT_THROW((void)locate_zone("Not/AZone"), std::runtime_error);
    try {
        EXPECT_FALSE(current_zone()->name().empty());
    } catch (const std::runtime_error& e) {
        GTEST_SKIP() << "host time zone unknown: " << e.what();
    }
#endif
}
