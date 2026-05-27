# Clocks & time points

> Clocks produce **time points** (an instant); subtracting two gives a
> [duration](durations.md). Pick the right clock for the job.
> **Header:** `<chrono>` · **Since:** C++11

## The clocks

| Clock | Property | Use for |
|---|---|---|
| `std::chrono::steady_clock` | monotonic — never goes backward | **measuring elapsed time** |
| `std::chrono::system_clock` | wall-clock; can jump (NTP, DST) | timestamps, calendar dates |
| `std::chrono::high_resolution_clock` | finest tick (often an alias) | usually just use `steady_clock` |
| `std::chrono::utc_clock`, `tai_clock`, `gps_clock` (C++20) | leap-second-aware variants | precise time standards |

Each clock has a `::now()` returning a `time_point` tied to *that* clock — time
points from different clocks aren't comparable.

## Measuring elapsed time (the canonical snippet)

```cpp
#include <chrono>
namespace ch = std::chrono;

auto start = ch::steady_clock::now();
do_work();
auto elapsed = ch::steady_clock::now() - start;       // a duration

auto ms = ch::duration_cast<ch::milliseconds>(elapsed).count();
double secs = ch::duration<double>(elapsed).count();
```

**Always use `steady_clock` for intervals** — `system_clock` can leap forward or
backward and produce negative or absurd elapsed times.

## Wall-clock time

```cpp
auto now = ch::system_clock::now();                   // a time_point
std::time_t t = ch::system_clock::to_time_t(now);     // bridge to C time

// C++20: format a system time point directly
std::string s = std::format("{:%Y-%m-%d %H:%M:%S}", now);
```

## time_point arithmetic

```cpp
auto deadline = ch::steady_clock::now() + 5s;          // time_point + duration
while (ch::steady_clock::now() < deadline) { /* ... */ }

auto diff = tp2 - tp1;                                  // time_point - time_point = duration
```

This is exactly what `std::this_thread::sleep_until` and timed waits consume.

## Gotchas

- **`steady_clock` for durations, `system_clock` for dates.** Timing a benchmark
  with `system_clock` is a classic bug — a clock adjustment mid-measurement gives
  garbage.
- **`high_resolution_clock` is often just an alias** for one of the others and
  may not be steady. Prefer naming `steady_clock` explicitly.
- **Time points are clock-specific.** You can't compare or subtract a
  `system_clock` time point and a `steady_clock` one.
- **`to_time_t` loses sub-second precision** and ties you to C's `time_t` — for
  formatting prefer C++20 `std::format` on the chrono type directly.

## See also

- [std::chrono::steady_clock — cppreference](https://en.cppreference.com/w/cpp/chrono/steady_clock)
- [std::chrono::system_clock — cppreference](https://en.cppreference.com/w/cpp/chrono/system_clock)
- [std::chrono::time_point — cppreference](https://en.cppreference.com/w/cpp/chrono/time_point)
