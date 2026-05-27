# std::ratio

> Exact rational numbers as **types**, computed at compile time — no rounding, no
> runtime cost. **Header:** `<ratio>` · **Since:** C++11

## Mental model

`std::ratio<Num, Den>` encodes a fraction in the type system, automatically
reduced to lowest terms. Arithmetic happens between *types* and yields new ratio
types. Its main job is giving `std::chrono` durations their units (seconds vs.
milliseconds vs. frames).

```cpp
#include <ratio>

using half  = std::ratio<1, 2>;
half::num;                       // 1   (static constexpr)
half::den;                       // 2

using r = std::ratio<4, 8>;      // auto-reduced
r::num; r::den;                  // 1, 2
```

## Arithmetic on types

```cpp
using sum  = std::ratio_add<std::ratio<1,2>, std::ratio<1,3>>;        // 5/6
using diff = std::ratio_subtract<std::ratio<1,2>, std::ratio<1,3>>;   // 1/6
using prod = std::ratio_multiply<std::ratio<2,3>, std::ratio<3,4>>;   // 1/2
using quot = std::ratio_divide<std::ratio<1,2>, std::ratio<1,4>>;     // 2/1

constexpr bool eq = std::ratio_equal_v<std::ratio<1,2>, std::ratio<2,4>>;   // true
constexpr bool lt = std::ratio_less_v<std::ratio<1,3>, std::ratio<1,2>>;    // true
```

## SI prefixes

The header predefines the standard prefixes as ratios:

```cpp
std::milli   // ratio<1, 1000>
std::micro   // ratio<1, 1'000'000>
std::kilo    // ratio<1000, 1>
std::mega, std::giga, std::nano, std::pico, ...
```

These are exactly what `std::chrono` uses:

```cpp
using ms = std::chrono::duration<long long, std::milli>;   // a millisecond tick
```

## Gotchas

- **It's all compile-time and type-level.** You don't make `std::ratio` *objects*
  and do runtime math with them; you compose the types. The value is in
  `::num`/`::den`.
- **Overflow is a hard error.** If a `ratio_*` operation would overflow
  `intmax_t`, it's ill-formed — by design, so you never get silently wrong units.
- **You'll rarely use it directly** outside building custom `chrono` durations or
  unit systems; that's its reason for existing.

## See also

- [std::ratio — cppreference](https://en.cppreference.com/w/cpp/numeric/ratio/ratio)
- [std::chrono::duration — cppreference](https://en.cppreference.com/w/cpp/chrono/duration)
