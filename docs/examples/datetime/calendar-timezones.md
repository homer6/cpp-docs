# Calendar & time zones (C++20)

> C++20 turned `<chrono>` into a full date library: civil calendar types,
> time-zone conversions, and `std::format` integration. **Header:** `<chrono>`

## Calendar types

Compose dates from typed fields (no more "is month 0-based?" guessing):

```cpp
#include <chrono>
using namespace std::chrono;

year_month_day ymd{2026y, May, 26d};        // typed literals: 2026y, 26d
auto y = ymd.year();
auto m = ymd.month();                        // May
bool valid = ymd.ok();                        // is it a real date?

// fluent construction with operator/ :
year_month_day d1 = 2026y / May / 26d;
year_month_day d2 = May / 26d / 2026y;        // any sensible field order
year_month_day last = 2026y / February / last;// last day of Feb (the 28th/29th)
```

## Converting to/from a time point

`sys_days` is a `system_clock` time point with day precision — the bridge between
calendar and clock:

```cpp
sys_days tp = 2026y / May / 26d;             // midnight UTC on that date
year_month_day back = year_month_day{tp};     // time point → calendar

auto days_between = sys_days{2026y/May/26d} - sys_days{2026y/Jan/1d};   // a duration in days

weekday wd{tp};                               // Tuesday — day of week
```

## Time zones

```cpp
auto now = system_clock::now();
zoned_time local{current_zone(), now};                 // system's zone
zoned_time ny{"America/New_York", now};                 // a named zone
zoned_time tokyo{"Asia/Tokyo", ny.get_sys_time()};      // convert between zones

auto local_time = ny.get_local_time();
```

The zone database handles DST and historical offsets. `current_zone()` is the
host's zone; `locate_zone("...")` looks one up by IANA name.

## Formatting (and parsing)

```cpp
std::format("{:%Y-%m-%d}", 2026y/May/26d);             // "2026-05-26"
std::format("{:%Y-%m-%d %H:%M:%S %Z}", ny);             // with zone abbreviation

// parse text back into chrono types:
std::chrono::sys_seconds tp;
std::istringstream in{"2026-05-26 12:00:00"};
in >> std::chrono::parse("%Y-%m-%d %H:%M:%S", tp);
```

## Gotchas

- **Library/OS support varies.** Calendar types are widely available; the
  **time-zone database** (`zoned_time`, `current_zone`) shipped later in some
  standard libraries and needs the IANA tz data present on the system. Check your
  toolchain.
- **`ok()` matters.** Constructing `2026y/February/30d` is allowed but `.ok()` is
  false; validate before trusting a date you built from parts.
- **`sys_days` is UTC midnight**, not local — converting a civil date to an
  instant goes through a time zone for local wall-clock time.
- **`%Z`/`%z` need a zoned time** (or known offset); formatting a bare
  `system_clock` time point as UTC won't print a meaningful zone name.

## See also

- [Calendar (year_month_day) — cppreference](https://en.cppreference.com/w/cpp/chrono/year_month_day)
- [std::chrono::zoned_time — cppreference](https://en.cppreference.com/w/cpp/chrono/zoned_time)
- [std::chrono::parse — cppreference](https://en.cppreference.com/w/cpp/chrono/parse)
