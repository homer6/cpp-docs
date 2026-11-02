# std::complex

> Complex numbers with the usual arithmetic and a full set of math functions.
> **Header:** `<complex>` · **Since:** C++98

## Mental model

`std::complex<T>` stores a real and an imaginary part of type `T` (use `float`,
`double`, or `long double` — those are the specializations with guaranteed
behavior). Operators and `<cmath>`-style functions are overloaded for it.

```
 z = 3 + 4i        imag ▲
                        │   • z        abs(z)  = 5      (length)
                        │  /           arg(z)  ≈ 0.927  (angle, radians)
                        │ /            norm(z) = 25     (length squared)
                        └───────▶ real conj(z) = 3 - 4i (mirror in the real axis)
```

## Declaration

```cpp
template<class T> class complex;
```

| Member / function | Meaning |
|---|---|
| `complex(re = T(), im = T())` | Construct from real and imaginary parts |
| `real()`, `imag()` | Read a part; `real(x)`, `imag(x)` set it |
| `+ - * /`, `==` | Arithmetic with another `complex<T>` or a plain `T` |
| `abs`, `arg`, `norm`, `conj`, `proj`, `polar` | Magnitude, angle, magnitude², conjugate, Riemann projection, construct from (ρ, θ) |
| `exp`, `log`, `pow`, `sqrt`, `sin`, … | Transcendental functions overloaded for `complex` |
| `operator<<`, `operator>>` | Stream as `(re,im)` |
| `i`, `if`, `il` literals | `std::complex_literals` (C++14) |

Most members and operators are `constexpr` since C++20
(`__cpp_lib_constexpr_complex` is `201711L` on GCC 15 and Clang 22).

## When to use it

- **Fits:** signal processing and FFTs, AC circuit and control-system math,
  geometry in the plane (rotation is multiplication by `std::polar(1.0, θ)`).
- **Anti-fits:** integer or fixed-point complex numbers (not supported), and
  SIMD-heavy kernels, where a library with a split real/imaginary layout is
  usually faster.

## Examples

```cpp
#include <complex>
using namespace std::complex_literals;        // the i / if / il literals (C++14)

std::complex<double> a{3.0, 4.0};             // 3 + 4i
std::complex<double> b = 1.0 + 2.0i;          // via literal

void demo() {
    a.real();  a.imag();                      // 3.0, 4.0
    std::abs(a);                              // 5.0  (magnitude)
    std::arg(a);                              // phase angle (radians)
    std::conj(a);                             // 3 - 4i
    std::norm(a);                             // 25.0 (magnitude squared — cheap)

    auto c = a * b + std::exp(1.0i);          // operators + transcendental fns
    auto p = std::polar(2.0, 3.14159);        // from magnitude & angle
}
```

Functions available: `exp`, `log`, `pow`, `sqrt`, `sin`/`cos`/`tan` and their
hyperbolic/inverse variants — all overloaded for `complex`.

### Compile-time arithmetic (C++20)

```cpp
#include <complex>

constexpr std::complex<double> z{1.0, 2.0};
constexpr auto z2 = z * z;                    // (1 + 2i)² = -3 + 4i
static_assert(z2.real() == -3.0 && z2.imag() == 4.0);
```

GCC with libstdc++ and Clang with libc++ accept this. Clang with libstdc++
rejects it (Clang 18 through 23 so far): libstdc++ stores the value as a GNU
`__complex__ double`, and Clang won't constant-evaluate arithmetic on it
([llvm/llvm-project#55370](https://github.com/llvm/llvm-project/issues/55370)).
The same product at run time is fine.

### Square root of a negative number

```cpp
#include <complex>

void roots() {
    std::sqrt(-1.0);                          // NaN — the real overload
    std::sqrt(std::complex<double>{-1.0});    // ≈ (0, 1) — the complex overload
}
```

## Gotchas

- **Only `float`/`double`/`long double` specializations are guaranteed.**
  `std::complex<int>` may compile but isn't supported by the standard's
  guarantees — use a floating type.
- **`std::norm` is magnitude *squared*, not the magnitude.** Use `std::abs` for
  the actual magnitude; `norm` is the cheaper "no sqrt" option for comparisons.
- **Mixing in an `int` doesn't compile.** The operators are templates on `T`, so
  `2 * z` with `z` a `complex<double>` finds no match; write `2.0 * z`.

  ```cpp compile-fail="operator\*|invalid operands"
  #include <complex>
  std::complex<double> z{3.0, 4.0};
  auto doubled = 2 * z;                       // error: int * complex<double>
  ```

- **Layout is compatible with an array of two `T`** (real, then imag) — useful for
  interop, and why C `_Complex` and FFT libraries interoperate with it.
- **The imaginary literals need the namespace** `std::complex_literals` (or
  `std::literals`).

## See also

- [std::complex — cppreference](https://en.cppreference.com/w/cpp/numeric/complex)
- [Complex math functions — cppreference](https://en.cppreference.com/w/cpp/numeric/complex/abs)
- [Numerics](README.md)
