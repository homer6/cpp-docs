# std::chrono durations

> A time span with its unit encoded in the type — `seconds` and `milliseconds`
> are *different types*, and conversions are explicit. **Header:** `<chrono>` ·
> **Since:** C++11

## Mental model

`std::chrono::duration<Rep, Period>` = a count (`Rep`, e.g. `long long`) times a
tick `Period` (a [`std::ratio`](../metaprogramming/ratio.md) of seconds). The
named aliases cover the usual units:

```cpp
std::chrono::nanoseconds, microseconds, milliseconds,
std::chrono::seconds, minutes, hours,
std::chrono::days, weeks, months, years          // (C++20)
```

## Examples

### Literals and arithmetic

```cpp
#include <chrono>
using namespace std::chrono_literals;        // 1s, 100ms, 2h, ...

auto timeout = 500ms;
auto total   = 2h + 30min;                    // common_type → minutes
auto t       = 1s + 500ms;                     // → 1500ms (finer unit wins)

timeout.count();                               // 500 (the raw number of ticks)
```

Adding durations of different units yields their common (finer) type
automatically — no silent unit loss.

### Converting between units

```cpp
std::chrono::seconds s = 90s;
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(s);   // 90000ms

// duration_cast TRUNCATES toward zero. For other roundings (C++17):
std::chrono::floor<std::chrono::seconds>(1500ms);    // 1s
std::chrono::round<std::chrono::seconds>(1500ms);    // 2s
std::chrono::ceil<std::chrono::seconds>(1ms);        // 1s
```

### Floating-point durations

```cpp
std::chrono::duration<double> secs = 1500ms;   // 1.5  (implicit: no precision loss)
double fractional = secs.count();              // 1.5
```

Integer→finer (`s`→`ms`) is implicit (lossless); coarser or lossy conversions
need an explicit `duration_cast`/`floor`/`round`/`ceil`.

## Gotchas

- **`duration_cast` truncates toward zero**, not rounds. Use `chrono::round` if
  you want nearest, `floor`/`ceil` for directed rounding (all C++17).
- **Lossy implicit conversions don't compile.** `chrono::seconds s = 1500ms;`
  is an error (would lose the .5s) — that's the safety. Use a cast or a
  floating-point duration.
- **`.count()` strips the unit** — only call it at the boundary (printing, a C
  API). Keep durations typed for as long as possible.
- **The literals need `using namespace std::chrono_literals`** (or
  `std::literals`).

## See also

- [std::chrono::duration — cppreference](https://en.cppreference.com/w/cpp/chrono/duration)
- [std::chrono::duration_cast — cppreference](https://en.cppreference.com/w/cpp/chrono/duration/duration_cast)
