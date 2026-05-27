# std::numeric_limits

> Compile-time facts about arithmetic types: min/max values, precision, special
> values. **Header:** `<limits>` · **Since:** C++98

## Mental model

`std::numeric_limits<T>` is a trait full of `constexpr` members describing `T`'s
representation — the portable replacement for C's `INT_MAX`, `DBL_EPSILON`, etc.

## Examples

```cpp
#include <limits>

std::numeric_limits<int>::max();          // largest int
std::numeric_limits<int>::min();          // most negative int
std::numeric_limits<unsigned>::max();     // all bits set

std::numeric_limits<double>::max();
std::numeric_limits<double>::lowest();    // most negative finite (NOT min!)
std::numeric_limits<double>::min();       // smallest POSITIVE normal value
std::numeric_limits<double>::epsilon();   // 1 ulp at 1.0 — precision yardstick
std::numeric_limits<double>::infinity();
std::numeric_limits<double>::quiet_NaN();

std::numeric_limits<int>::digits;         // value bits (31 for int)
std::numeric_limits<T>::is_signed;
std::numeric_limits<T>::is_integer;
```

### Common uses

```cpp
// initialize a running max to the smallest possible value:
double best = std::numeric_limits<double>::lowest();

// float comparison with a tolerance:
bool close = std::abs(a - b) <= std::numeric_limits<double>::epsilon() * scale;
```

## Gotchas

- **`min()` means different things for int vs. float.** For integers it's the most
  negative value; for floating point it's the smallest *positive normal* value.
  For "most negative finite float," use **`lowest()`** (added in C++11 precisely
  because `min()` confused everyone).
- **`epsilon()` is the gap at 1.0**, not a universal tolerance. Scale it by the
  magnitude of the values you're comparing.
- **It's all `constexpr`** — usable in `static_assert`, array sizes, templates.
- **Prefer it over C macros** (`INT_MAX`, `FLT_EPSILON`): type-safe and works in
  generic code (`numeric_limits<T>`).

## See also

- [std::numeric_limits — cppreference](https://en.cppreference.com/w/cpp/types/numeric_limits)
