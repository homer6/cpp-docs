# std::numeric_limits

> Compile-time facts about arithmetic types: min/max values, precision, special
> values. **Header:** `<limits>` · **Since:** C++98

## Mental model

`std::numeric_limits<T>` is a trait full of `constexpr` members describing `T`'s
representation — the portable replacement for C's `INT_MAX`, `DBL_EPSILON`, etc.

## Declaration

```cpp
template<class T> class numeric_limits;
```

Specialized for every arithmetic type (and their `const`/`volatile` versions).
The members that matter most:

| Member | Integers | Floating point |
|---|---|---|
| `min()` | Most negative value | Smallest **positive normal** value |
| `lowest()` (C++11) | Same as `min()` | Most negative finite value |
| `max()` | Largest value | Largest finite value |
| `digits` | Value bits (excluding sign) | Mantissa bits |
| `digits10` | Decimal digits representable without change (9 for 32-bit `int`) | Decimal digits that survive text → `T` → text (15 for `double`) |
| `max_digits10` (C++11) | 0 | Decimal digits needed to print a value and read it back exactly (17 for `double`) |
| `epsilon()` | 0 | Gap between 1.0 and the next value |
| `infinity()`, `quiet_NaN()`, `denorm_min()` | 0 | Special values (check `has_infinity`, `has_quiet_NaN`) |
| `is_specialized`, `is_signed`, `is_integer`, `is_exact` | Classification flags | |

## When to use it

- **Fits:** sentinels and initial values (`lowest()` for a running max), overflow
  checks before arithmetic, tolerances in floating-point comparisons,
  `static_assert`s on type properties in generic code.
- **Anti-fits:** testing whether a value *is* NaN or infinite — use
  `std::isnan` / `std::isinf`. Safe mixed-sign comparisons — use C++20
  `std::cmp_less` and friends from `<utility>`.

## Examples

```cpp
#include <limits>

template<class T>
void limits_tour() {
    std::numeric_limits<int>::max();          // largest int
    std::numeric_limits<int>::min();          // most negative int
    std::numeric_limits<unsigned>::max();     // all bits set

    std::numeric_limits<double>::max();
    std::numeric_limits<double>::lowest();    // most negative finite (NOT min!)
    std::numeric_limits<double>::min();       // smallest POSITIVE normal value
    std::numeric_limits<double>::epsilon();   // 1 ulp at 1.0 — precision yardstick
    std::numeric_limits<double>::infinity();
    std::numeric_limits<double>::quiet_NaN();

    std::numeric_limits<int>::digits;         // value bits (31 for 32-bit int)
    std::numeric_limits<T>::is_signed;
    std::numeric_limits<T>::is_integer;
}
```

### Common uses

```cpp
// initialize a running max to the smallest possible value:
double best = std::numeric_limits<double>::lowest();

// float comparison with a tolerance:
bool nearly_equal(double a, double b, double scale) {
    return std::abs(a - b) <= std::numeric_limits<double>::epsilon() * scale;
}
```

### Check before you overflow

```cpp
#include <limits>
#include <optional>

constexpr std::optional<int> checked_add(int a, int b) {
    if (b > 0 && a > std::numeric_limits<int>::max() - b) return std::nullopt;
    if (b < 0 && a < std::numeric_limits<int>::min() - b) return std::nullopt;
    return a + b;
}
static_assert(!checked_add(std::numeric_limits<int>::max(), 1));
static_assert(*checked_add(40, 2) == 42);
```

## Gotchas

- **`min()` means different things for int vs. float.** For integers it's the most
  negative value; for floating point it's the smallest *positive normal* value.
  For "most negative finite float," use **`lowest()`** (added in C++11 precisely
  because `min()` confused everyone).
- **`epsilon()` is the gap at 1.0**, not a universal tolerance. Scale it by the
  magnitude of the values you're comparing.
- **Unspecialized types compile silently.** For a type with no specialization
  (your own `struct`), every member is `T()` or `0/false` and `is_specialized` is
  `false` — check it in generic code.
- **`char`-sized results print as characters.** `std::cout <<
  numeric_limits<unsigned char>::max()` writes byte 255, not `"255"`; promote with
  unary `+` first.
- **It's all `constexpr`** — usable in `static_assert`, array sizes, templates.
- **Prefer it over C macros** (`INT_MAX`, `FLT_EPSILON`): type-safe and works in
  generic code (`numeric_limits<T>`).

## See also

- [std::numeric_limits — cppreference](https://en.cppreference.com/w/cpp/types/numeric_limits)
- [std::numeric_limits::lowest — cppreference](https://en.cppreference.com/w/cpp/types/numeric_limits/lowest)
- [std::cmp_less — cppreference](https://en.cppreference.com/w/cpp/utility/intcmp)
