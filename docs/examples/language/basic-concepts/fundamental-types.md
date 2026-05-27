# Fundamental types

> The built-in types every program starts from, and the fixed-width aliases that
> make sizes portable.

## The categories

```
 void
 std::nullptr_t              (the type of nullptr)
 arithmetic ─┬─ integral ─┬─ bool
             │            ├─ character (char, wchar_t, char8_t/16_t/32_t)
             │            └─ signed/unsigned (short, int, long, long long)
             └─ floating  ─── float, double, long double
                              (+ float16_t/32_t/64_t/128_t, bfloat16_t — C++23)
```

## Sizes are minimums, not fixed

The standard guarantees only *ranges* (e.g. `int` ≥ 16 bits, in practice 32),
not exact sizes. For portable, exact widths use `<cstdint>`:

```cpp
#include <cstdint>

std::int32_t  a;     // exactly 32 bits, signed
std::uint64_t b;     // exactly 64 bits, unsigned
std::int_fast16_t f; // fastest type with ≥16 bits
std::intptr_t  p;    // integer big enough to hold a pointer
std::size_t    n;    // unsigned, result of sizeof / container .size()
std::ptrdiff_t d;    // signed, result of pointer subtraction
```

## Initialization defaults & the bool/char notes

```cpp
int x;            // uninitialized if local (indeterminate value!) — always initialize
int y{};          // value-initialized to 0
bool b = true;    // 1 byte, but only true/false
char c = 'A';     // may be signed OR unsigned — use signed char / unsigned char when it matters
auto big = 5'000'000;   // digit separators (C++14)
```

## Type sizes and properties at compile time

```cpp
sizeof(int);                         // bytes
alignof(double);                     // alignment
std::numeric_limits<int>::max();     // see Language support → numeric_limits
```

## Gotchas

- **`int` is not guaranteed 32 bits** (just ≥16). Don't assume widths — use
  `<cstdint>` aliases when the exact size matters (file formats, protocols).
- **Plain `char`'s signedness is implementation-defined.** Use `signed char` /
  `unsigned char` for arithmetic on bytes; use `std::byte` (C++17) for "raw
  memory, not a number."
- **Uninitialized locals hold garbage.** Reading them is UB — always initialize
  (`int x{};`).
- **Signed overflow is UB; unsigned wraps.** Mixing signed/unsigned in comparisons
  is a classic bug — see [`std::cmp_less`](../../utilities/utility.md).
- **`char8_t` (C++20) is a distinct type** from `char` — UTF-8 string literals
  (`u8"..."`) are now `const char8_t[]`.

## See also

- [Fundamental types — cppreference](https://en.cppreference.com/w/cpp/language/types)
- [Fixed-width integers `<cstdint>` — cppreference](https://en.cppreference.com/w/cpp/types/integer)
