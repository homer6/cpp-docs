# std::complex

> Complex numbers with the usual arithmetic and a full set of math functions.
> **Header:** `<complex>` · **Since:** C++98

## Mental model

`std::complex<T>` stores a real and an imaginary part of type `T` (use `float`,
`double`, or `long double` — those are the specializations with guaranteed
behavior). Operators and `<cmath>`-style functions are overloaded for it.

## Examples

```cpp
#include <complex>
using namespace std::complex_literals;        // the i / if / il literals (C++14)

std::complex<double> a{3.0, 4.0};             // 3 + 4i
std::complex<double> b = 1.0 + 2.0i;          // via literal

a.real();  a.imag();                          // 3.0, 4.0
std::abs(a);                                  // 5.0  (magnitude)
std::arg(a);                                  // phase angle (radians)
std::conj(a);                                 // 3 - 4i
std::norm(a);                                 // 25.0 (magnitude squared — cheap)

auto c = a * b + std::exp(1.0i);              // operators + transcendental fns
auto p = std::polar(2.0, 3.14159);            // from magnitude & angle
```

Functions available: `exp`, `log`, `pow`, `sqrt`, `sin`/`cos`/`tan` and their
hyperbolic/inverse variants — all overloaded for `complex`.

## Gotchas

- **Only `float`/`double`/`long double` specializations are guaranteed.**
  `std::complex<int>` may compile but isn't supported by the standard's
  guarantees — use a floating type.
- **`std::norm` is magnitude *squared*, not the magnitude.** Use `std::abs` for
  the actual magnitude; `norm` is the cheaper "no sqrt" option for comparisons.
- **Layout is compatible with an array of two `T`** (real, then imag) — useful for
  interop, and why C `_Complex` and FFT libraries interoperate with it.
- **The imaginary literals need the namespace** `std::complex_literals` (or
  `std::literals`).

## See also

- [std::complex — cppreference](https://en.cppreference.com/w/cpp/numeric/complex)
- [Complex math functions — cppreference](https://en.cppreference.com/w/cpp/numeric/complex/abs)
