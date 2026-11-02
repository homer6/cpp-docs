# Saturation arithmetic

> Integer `+ - * /` and conversions that **clamp to the type's range** instead of
> wrapping or overflowing. **Header:** `<numeric>` · **Since:** C++26

## Mental model

Ordinary integer arithmetic either wraps (unsigned) or is undefined behaviour
(signed overflow). A saturating operation computes the mathematically exact
result and then clamps it into `[numeric_limits<T>::min(), numeric_limits<T>::max()]`:

```
 uint8_t:   200 + 100 = 300  → stored back in a uint8_t: 44 (wrapped)
                             → saturating_add: 255  (clamped to max)
 uint8_t:    10 -  20 = -10  → saturating_sub: 0    (clamped to min)
```

That's what you want for pixel values, audio samples, counters and any quantity
where "stuck at the limit" beats "wrapped to nonsense".

## Declaration

```cpp nocompile
namespace std {
    template<class T> constexpr T saturating_add(T x, T y) noexcept;
    template<class T> constexpr T saturating_sub(T x, T y) noexcept;
    template<class T> constexpr T saturating_mul(T x, T y) noexcept;
    template<class T> constexpr T saturating_div(T x, T y) noexcept;
    template<class R, class T> constexpr R saturating_cast(T x) noexcept;
}
```

- `T` and `R` must be signed or unsigned **integer** types: no `bool`, no
  character types, no floating point.
- Both operands have the **same type** `T`: there are no usual arithmetic
  conversions, so `saturating_add(1, 2u)` doesn't deduce.
- `saturating_cast<R>(x)` converts `x` to `R`, clamping when it doesn't fit.
- Feature-test macro `__cpp_lib_saturation_arithmetic`: `202603L` for these
  names. The header is freestanding.

## Examples

### Clamp instead of wrap

```cpp needs=saturating
#include <climits>
#include <cstdint>
#include <numeric>

int main() {
    std::saturating_add<std::uint8_t>(200, 100);   // 255, not 44
    std::saturating_sub<std::uint8_t>(10, 20);     // 0, not 246
    std::saturating_sub(3u, 5u);                   // 0u
    std::saturating_mul<std::int8_t>(-100, 2);     // -128 (int8_t's min)
    std::saturating_div(INT_MIN, -1);              // INT_MAX: the one overflowing division
    std::saturating_cast<std::uint8_t>(300);       // 255
    std::saturating_cast<std::int8_t>(-1000);      // -128
}
```

### The pre-rename names (`add_sat`, `saturate_cast`)

C++26 first shipped these as `add_sat`, `sub_sat`, `mul_sat`, `div_sat` and
`saturate_cast` (P0543, macro `202311L`). An NB comment renamed them before
C++26 was finished (P4052R0, adopted in 2026), and the macro went up to
`202603L`. **GCC 16 has only the new names; GCC 15 and Clang 22 (libc++) have
only the old ones.** Until every compiler you target has caught up, switch on
the macro:

```cpp
#include <cstdint>
#include <numeric>

std::uint8_t brighten(std::uint8_t px, std::uint8_t by) {
#if __cpp_lib_saturation_arithmetic >= 202603L
    return std::saturating_add(px, by);
#else
    return std::add_sat(px, by);    // GCC 15, Clang 22
#endif
}
```

## Gotchas

- **Two spellings in the wild.** Code written against GCC 15 or Clang 22
  (`std::add_sat`) stops compiling on GCC 16, and vice versa. Test the macro's
  *value*: `>= 202311L` alone doesn't promise the new names.
- **`saturating_div(x, 0)` is still undefined.** Division by zero is a
  precondition violation, not a saturation case; in a constant expression it's
  a compile error.
- **Integers only, same type only.** `std::saturating_add(1, 2u)` and
  `std::saturating_add(true, false)` don't compile. Spell the type
  (`saturating_add<std::uint8_t>(200, 100)`) or cast the operands.
- **Not a checked operation.** You can't tell afterwards whether it clamped. To
  detect overflow, use `ckd_add` (`<stdckdint.h>`, C++26), which returns
  `true` on overflow. From C++ it needs a C++-aware header: libstdc++'s
  (GCC 15, also GCC 14.4; GCC 16 defines `__cpp_lib_stdckdint_h`) or Clang's.
  GCC 14.2's own `<stdckdint.h>` is the C one, and `ckd_add` fails with
  "`_Bool` was not declared in this scope".

## See also

- [Saturation arithmetic — cppreference](https://en.cppreference.com/w/cpp/numeric#Saturation_arithmetic)
  (still under the old names: [std::add_sat](https://en.cppreference.com/w/cpp/numeric/add_sat))
- [P4052R0: Renaming saturation arithmetic functions](https://wg21.link/P4052R0)
- [P0543R3: Saturation arithmetic](https://wg21.link/P0543R3)
- [[numeric.sat] in the draft](https://eel.is/c++draft/numeric.sat)
- [Numeric algorithms (`<numeric>`)](../algorithms/numeric.md)
