# Date and time library (`<chrono>`)

> Type-safe time: durations carry their units in the type, clocks produce time
> points, and (C++20) a full calendar and time-zone system sits on top.
> **Header:** `<chrono>` · **Namespace:** `std::chrono`

Mirrors [cppreference's chrono library](https://en.cppreference.com/w/cpp/chrono).

| Page | Since | What |
|---|---|---|
| [Durations](durations.md) | C++11 | `seconds`, `milliseconds`, `duration_cast`, literals |
| [Clocks & time points](clocks-timepoints.md) | C++11 | `steady_clock`, `system_clock`, measuring elapsed time |
| [Calendar & time zones](calendar-timezones.md) | C++20 | `year_month_day`, `zoned_time`, formatting |

## Why chrono instead of `time_t`/`int`

Units live in the type, so the compiler stops you mixing seconds and
milliseconds, and conversions are explicit (`duration_cast`). A `steady_clock`
time point can't be accidentally compared to a `system_clock` one. It's the
difference between "an int that's probably milliseconds" and "milliseconds,
guaranteed."
