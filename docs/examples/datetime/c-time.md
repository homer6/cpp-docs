# C date and time (`<ctime>`)

> The C library's clock: a `std::time_t` count of seconds, broken down into a
> `std::tm` by `localtime` / `gmtime` and printed with `strftime`.
> **Header:** `<ctime>` · **Since:** C++98 (inherited from C)

## Mental model

```
std::time(nullptr) ─▶ time_t (seconds since the epoch, usually 1970-01-01 UTC)
                        │ gmtime / localtime            ▲ mktime (local) / timegm (UTC, C++26)
                        ▼                               │
                      std::tm { tm_year = years since 1900, tm_mon = 0..11, tm_mday = 1..31, ... }
                        │ strftime(buf, size, "%Y-%m-%d", &tm)
                        ▼
                      "1971-01-01"
```

`time_t` is an arithmetic type of unspecified width (64-bit on all three local
toolchains). `std::tm` is a plain struct with no time zone in it: `gmtime`
fills it as UTC, `localtime` as the process's local zone (`TZ`).

## Declaration

| Function | What it does |
|---|---|
| `time_t time(time_t* out)` | current calendar time; pass `nullptr` |
| `tm* gmtime(const time_t*)` / `tm* localtime(const time_t*)` | break down as UTC / local time, into a **static** `tm` |
| `time_t mktime(tm*)` | local `tm` → `time_t`, normalising out-of-range fields |
| `double difftime(time_t end, time_t start)` | seconds between two times |
| `size_t strftime(char* buf, size_t max, const char* fmt, const tm*)` | format; returns the length, or **0** if it didn't fit |
| `int timespec_get(timespec*, TIME_UTC)` | seconds + nanoseconds (C++17) |
| `tm* gmtime_r(const time_t*, tm* buf)`, `localtime_r`, `time_t timegm(tm*)` | C++26 (from C23): fill a caller's `tm`; no local toolchain declares them in `std::` yet |

`asctime` and `ctime` (fixed-format text in a static buffer) are deprecated in
C++26, following C23.

## When to use it

- **Talking to C APIs** that take or return `time_t` / `tm` (file times in old
  APIs, `stat`, log formats).
- **Everything else: use [`<chrono>`](clocks-timepoints.md).** It has the units
  in the type, sub-second precision, a monotonic clock for intervals,
  thread-safe time zones and `std::format` support
  ([calendar & time zones](calendar-timezones.md)).
  `system_clock::to_time_t` / `from_time_t` bridge the two.

## Examples

### Now, broken down and formatted

```cpp
#include <cstdio>
#include <ctime>

int main() {
    std::time_t t = 365 * 86400;                       // one year after the epoch
    std::tm utc = *std::gmtime(&t);                    // copy out of the static buffer
    char buf[32];
    std::size_t n = std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &utc);
    std::printf("%s (%zu chars)\n", buf, n);           // 1971-01-01 00:00:00 (19 chars)
    std::printf("%d %d\n", utc.tm_year, utc.tm_mon);   // 71 0  (years since 1900; January is 0)

    char tiny[4];
    std::printf("%zu\n", std::strftime(tiny, sizeof tiny, "%Y-%m-%d", &utc)); // 0: didn't fit
    std::printf("%d\n", std::difftime(std::time(nullptr), t) > 0);            // 1: now is later
}
```

### The same thing with `<chrono>`

```cpp
#include <chrono>
#include <ctime>
#include <format>
#include <string>

std::string iso_date(std::time_t t) {
    auto tp = std::chrono::system_clock::from_time_t(t);           // bridge in
    return std::format("{:%Y-%m-%d}", std::chrono::floor<std::chrono::days>(tp));
}   // iso_date(365 * 86400) == "1971-01-01"; no static buffer, no tm_year + 1900
```

## Gotchas

- **`localtime` and `gmtime` return a pointer to static storage.** The next
  call overwrites it, and two threads calling them race: the standard says they
  "are not required to avoid data races". Copy the `tm` out at once
  (`std::tm t = *std::gmtime(&now);`). Whether `gmtime` and `localtime` share
  one buffer varies: glibc (GCC 16 image) shares it, macOS doesn't.
- **The thread-safe versions aren't portable yet.** C++26 adds
  `std::localtime_r` / `std::gmtime_r` (from C23, P3348), but GCC 15, GCC 16
  and Clang 22 don't declare them in `std::` yet. POSIX `::localtime_r` (global
  namespace) works on Linux and macOS; MSVC has `localtime_s` with the
  arguments the other way round, which is C11 Annex K, not standard C++. In new
  code use `std::chrono::zoned_time` instead.
- **`tm_year` counts from 1900 and `tm_mon` from 0.** The classic off-by-1900
  and off-by-one bugs.
- **`strftime` returns 0 when the buffer is too small** (and the buffer's
  contents are then indeterminate). Check the result.
- **`mktime` treats its input as local time**; for UTC there's only POSIX
  `::timegm` until C++26's `std::timegm` arrives.
- **`time_t` has one-second resolution** and an unspecified epoch in the C
  standard (POSIX and every common platform use 1970-01-01 UTC). A 32-bit
  `time_t` overflows in January 2038.

## See also

- [Date and time utilities (C) — cppreference](https://en.cppreference.com/w/cpp/chrono/c)
- [std::localtime — cppreference](https://en.cppreference.com/w/cpp/chrono/c/localtime)
- [std::strftime — cppreference](https://en.cppreference.com/w/cpp/chrono/c/strftime)
- [\[ctime.syn\] — C++ draft](https://eel.is/c++draft/ctime.syn)
- [P3348R4 — C++26 should refer to C23 not C17](https://wg21.link/P3348R4)
- [Clocks & time points](clocks-timepoints.md) · [Calendar & time zones](calendar-timezones.md)
