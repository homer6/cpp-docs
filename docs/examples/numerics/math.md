# Math functions & constants

> The standard math library (`<cmath>`) plus type-safe math constants
> (`<numbers>`, C++20). **Headers:** `<cmath>`, `<numbers>`

## `<cmath>` — the workhorse functions

```cpp
#include <cmath>

std::sqrt(2.0);  std::cbrt(27.0);                 // roots
std::pow(2.0, 10);  std::exp(x);  std::log(x);  std::log2(x);  std::log10(x);
std::sin(x); std::cos(x); std::tan(x);  std::atan2(y, x);       // trig
std::floor(x); std::ceil(x); std::round(x); std::trunc(x);
std::fmod(a, b);  std::remainder(a, b);
std::abs(x);                                       // fabs for floats
std::hypot(x, y);                                  // sqrt(x²+y²) without overflow
std::fma(a, b, c);                                 // a*b + c, single rounding
std::lerp(a, b, t);                                // (C++20) linear interpolation
std::midpoint(a, b);                               // (C++20) overflow-safe average
```

### Classification

```cpp
std::isnan(x);  std::isinf(x);  std::isfinite(x);  std::isnormal(x);
std::signbit(x);
```

## Special functions (C++17)

`<cmath>` gained the mathematical special functions in C++17:

```cpp
std::assoc_legendre, std::beta, std::comp_ellint_1/2/3,
std::cyl_bessel_i/j/k, std::cyl_neumann, std::expint,
std::hermite, std::laguerre, std::legendre, std::riemann_zeta, std::sph_bessel, ...
```

## `<numbers>` — math constants (C++20)

Type-safe, no more `#define M_PI` or hand-typed digits:

```cpp
#include <numbers>
namespace nb = std::numbers;

nb::pi;          // π as double      (also pi_v<float>, pi_v<long double>)
nb::e;           // Euler's number
nb::sqrt2;       // √2
nb::ln2;         // ln 2
nb::phi;         // golden ratio
nb::inv_pi;      // 1/π
double area = nb::pi * r * r;
float  c    = nb::pi_v<float> * d;     // pick the precision explicitly
```

## Gotchas

- **`std::abs` ambiguity.** `<cmath>`'s `std::abs` handles floats; the integer
  `std::abs` lives in `<cstdlib>`. Calling `std::abs(some_int)` with only
  `<cmath>` included can pick the float overload and surprise you — include the
  right header and/or use explicit types.
- **`M_PI` is non-standard** (a POSIX extension, needs `_USE_MATH_DEFINES` on
  MSVC). Use `std::numbers::pi` in C++20+.
- **`std::pow` for integer powers can be slower and imprecise.** `pow(x, 2)` may
  differ from `x*x` by rounding; for small integer exponents, multiply.
- **Use `std::hypot`, not `sqrt(x*x+y*y)`** — `hypot` avoids intermediate
  overflow/underflow. Likewise `std::fma` for accuracy where it matters.

## See also

- [Common mathematical functions — cppreference](https://en.cppreference.com/w/cpp/numeric/math)
- [Mathematical special functions — cppreference](https://en.cppreference.com/w/cpp/numeric/special_functions)
- [std::numbers — cppreference](https://en.cppreference.com/w/cpp/numeric/constants)
