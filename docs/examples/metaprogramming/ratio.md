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
static_assert(half::num == 1);   // static constexpr members
static_assert(half::den == 2);

using r = std::ratio<4, 8>;      // auto-reduced
static_assert(r::num == 1 && r::den == 2);
```

## Declaration

```cpp
template<std::intmax_t Num, std::intmax_t Denom = 1> class ratio;
```

| Piece | Meaning |
|---|---|
| `num`, `den` | The reduced numerator and denominator; the sign is carried by `num`, `den > 0` |
| `type` | `ratio<num, den>` — the canonical (reduced) type |
| `ratio_add`, `ratio_subtract`, `ratio_multiply`, `ratio_divide` | Alias templates for the reduced result |
| `ratio_equal`, `ratio_not_equal`, `ratio_less`, `…_less_equal`, `…_greater`, `…_greater_equal` | `bool_constant` comparisons (`_v` variable templates since C++17) |
| `std::nano`, `std::milli`, `std::kilo`, … | SI-prefix aliases |

`Denom` must not be zero, and neither argument may be `INTMAX_MIN`.

## When to use it

- **Fits:** custom `chrono` duration units, compile-time unit systems and scale
  factors where rounding must never creep in.
- **Anti-fits:** runtime fractions — there are no `ratio` objects with values;
  write a small `struct { long num, den; }` or use a rational-number library.

## Examples

### Arithmetic on types

```cpp
using sum  = std::ratio_add<std::ratio<1,2>, std::ratio<1,3>>;        // 5/6
using diff = std::ratio_subtract<std::ratio<1,2>, std::ratio<1,3>>;   // 1/6
using prod = std::ratio_multiply<std::ratio<2,3>, std::ratio<3,4>>;   // 1/2
using quot = std::ratio_divide<std::ratio<1,2>, std::ratio<1,4>>;     // 2/1

constexpr bool eq = std::ratio_equal_v<std::ratio<1,2>, std::ratio<2,4>>;   // true
constexpr bool lt = std::ratio_less_v<std::ratio<1,3>, std::ratio<1,2>>;    // true
```

### SI prefixes

The header predefines the standard prefixes as ratios:

```cpp
#include <ratio>

static_assert(std::is_same_v<std::milli, std::ratio<1, 1000>>);
static_assert(std::is_same_v<std::micro, std::ratio<1, 1'000'000>>);
static_assert(std::is_same_v<std::kilo,  std::ratio<1000, 1>>);
// also std::mega, std::giga, std::nano, std::pico, ...
```

These are exactly what `std::chrono` uses:

```cpp
using ms = std::chrono::duration<long long, std::milli>;   // a millisecond tick
```

### A custom unit

```cpp
#include <chrono>
#include <ratio>

using frames = std::chrono::duration<long long, std::ratio<1, 60>>;   // 60 fps ticks

static_assert(std::chrono::duration_cast<std::chrono::milliseconds>(frames{3}).count() == 50);
static_assert(frames{std::chrono::seconds{2}}.count() == 120);         // exact: no cast
```

## Gotchas

- **It's all compile-time and type-level.** You don't make `std::ratio` *objects*
  and do runtime math with them; you compose the types. The value is in
  `::num`/`::den`.
- **Unreduced ratios are different types.** `ratio<2, 4>` isn't the same type as
  `ratio<1, 2>` (even though its `num`/`den` are 1 and 2); compare with
  `ratio_equal_v`, or normalize with `::type`. The `ratio_add` family already
  yields reduced types.
- **Overflow is a hard error.** If a `ratio_*` operation would overflow
  `intmax_t`, it's ill-formed — by design, so you never get silently wrong units.
  So is a zero denominator.
- **You'll rarely use it directly** outside building custom `chrono` durations or
  unit systems; that's its reason for existing.

## See also

- [std::ratio — cppreference](https://en.cppreference.com/w/cpp/numeric/ratio/ratio)
- [Compile-time rational arithmetic — cppreference](https://en.cppreference.com/w/cpp/numeric/ratio)
- [std::chrono::duration — cppreference](https://en.cppreference.com/w/cpp/chrono/duration)
