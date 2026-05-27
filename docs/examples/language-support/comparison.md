# Three-way comparison (`operator<=>`)

> The "spaceship" operator: define ordering once and get all six relational
> operators for free. **Header:** `<compare>` · **Since:** C++20

## Mental model

`a <=> b` returns an ordering object (not a bool) describing whether `a` is less,
equal, or greater. From a single `operator<=>` (plus `==`), the compiler
**synthesizes** `<`, `>`, `<=`, `>=`, `!=`. Defaulting it gives memberwise
lexicographic comparison — no more writing six boilerplate operators.

## The three ordering types

| Type | When | Meaning |
|---|---|---|
| `std::strong_ordering` | most types | equal values are interchangeable |
| `std::weak_ordering` | e.g. case-insensitive strings | equivalent but not identical |
| `std::partial_ordering` | floating point | some pairs are *unordered* (NaN) |

Values: `less`, `equal`/`equivalent`, `greater` (and `unordered` for partial).

## Examples

### Default it — the common case

```cpp
#include <compare>

struct Point {
    int x, y;
    auto operator<=>(const Point&) const = default;   // lexicographic on x, then y
    bool operator==(const Point&) const = default;     // (also defaulted)
};

Point{1,2} < Point{1,3};      // true — all six operators now work
std::sort(points.begin(), points.end());               // sortable for free
```

### Custom ordering

```cpp
struct Version {
    int major, minor, patch;
    std::strong_ordering operator<=>(const Version& o) const {
        if (auto c = major <=> o.major; c != 0) return c;
        if (auto c = minor <=> o.minor; c != 0) return c;
        return patch <=> o.patch;
    }
    bool operator==(const Version&) const = default;
};
```

### Using the result

```cpp
auto c = a <=> b;
if (c < 0)      { /* a < b */ }
else if (c > 0) { /* a > b */ }
else            { /* equal/equivalent */ }
if (c == std::strong_ordering::equal) { /* ... */ }
```

## Gotchas

- **You usually still need `operator==` too.** `<=>` synthesizes the relational
  ops but **not** `==`/`!=`; default or define `==` separately (often
  `= default` alongside).
- **Floating-point gives `partial_ordering`** — NaN compares `unordered`, so don't
  assume `!(a < b)` implies `a >= b` for floats.
- **`<=>` returns an ordering, not a bool.** Comparing the result to `0` (or to a
  named ordering value) is how you branch; `if (a <=> b)` is wrong.
- **Defaulted `<=>` compares bases then members in declaration order** — make sure
  that order matches the ordering you intend.

## See also

- [Default comparisons — cppreference](https://en.cppreference.com/w/cpp/language/default_comparisons)
- [std::strong_ordering / weak / partial — cppreference](https://en.cppreference.com/w/cpp/utility/compare/strong_ordering)
