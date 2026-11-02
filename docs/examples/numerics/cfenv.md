# Floating-point environment

> Test and clear the IEEE 754 exception flags (divide-by-zero, inexact, …) and
> read or change the dynamic rounding mode. **Header:** `<cfenv>` · **Since:** C++11

## Mental model

The FPU keeps a small piece of thread-local state next to your registers: a set
of **sticky status flags** that floating-point operations raise and never clear,
and **control modes** such as the current rounding direction. `<cfenv>` is the C
interface to that state.

```
   x / 0.0  ──raises──▶  [ FE_DIVBYZERO ][ FE_INEXACT ][ FE_INVALID ] ...   sticky flags
   rint(x)  ◀──reads───  [ rounding mode: FE_TONEAREST | UPWARD | ... ]     control mode
                         └──────────── std::fenv_t (whole environment) ─┘
```

Flags stay raised until you clear them, so the pattern is always
*clear → compute → test*. The environment is per thread. C says a new thread
starts with its creator's environment; glibc does that for `std::thread`, but
on macOS a new thread starts in the default environment (round-to-nearest).

## Declaration

| Name | What it does |
|---|---|
| `fenv_t`, `fexcept_t` | the whole environment; a saved copy of the flags |
| `FE_DIVBYZERO`, `FE_INEXACT`, `FE_INVALID`, `FE_OVERFLOW`, `FE_UNDERFLOW`, `FE_ALL_EXCEPT` | exception flag bits (each defined only if the platform supports it) |
| `FE_TONEAREST`, `FE_UPWARD`, `FE_DOWNWARD`, `FE_TOWARDZERO` | rounding directions (each defined only if supported) |
| `feclearexcept(e)` · `fetestexcept(e)` · `feraiseexcept(e)` | clear, test, raise flags; `fetestexcept` returns the subset of `e` that is set |
| `fegetexceptflag(&f, e)` · `fesetexceptflag(&f, e)` | save and restore flags without raising anything |
| `fegetround()` · `fesetround(mode)` | read or change the rounding direction; `fesetround` returns 0 on success |
| `fegetenv(&env)` · `fesetenv(&env)` | save or restore the whole environment; `FE_DFL_ENV` is the program-start one |
| `feholdexcept(&env)` · `feupdateenv(&env)` | save the environment and clear the flags; restore it and re-raise what was raised in between |

Every function returns `0` on success except `fetestexcept` and `fegetround`.

## When to use it

- **Detecting numeric trouble cheaply**: run a batch of arithmetic, then check
  once whether anything overflowed or produced NaN (`FE_INVALID`), instead of
  testing every result.
- **Interval arithmetic and error bounds**: compute a lower bound with
  `FE_DOWNWARD` and an upper bound with `FE_UPWARD`.
- **Library code that must not leak state**: `feholdexcept` / `feupdateenv`
  hide intermediate flags from the caller.

Anti-fit: error handling for `<cmath>` in portable code. Whether math functions
report errors through `errno`, the FP flags or both is
`math_errhandling`, and that is implementation-defined (see Gotchas).

## Examples

### Clear, compute, test

```cpp
#include <cfenv>
#include <cmath>
#include <cstdio>

#pragma STDC FENV_ACCESS ON   // honoured by Clang; GCC ignores it (see Gotchas)

volatile double zero = 0.0, one = 1.0, three = 3.0;   // stop compile-time folding

void report(const char* what) {
    std::printf("%-10s", what);
    if (std::fetestexcept(FE_DIVBYZERO)) std::printf(" FE_DIVBYZERO");
    if (std::fetestexcept(FE_INEXACT))   std::printf(" FE_INEXACT");
    if (std::fetestexcept(FE_INVALID))   std::printf(" FE_INVALID");
    std::printf("\n");
}

int main() {
    volatile double r;
    std::feclearexcept(FE_ALL_EXCEPT); r = one / zero;       report("1.0/0.0");
    std::feclearexcept(FE_ALL_EXCEPT); r = one / three;      report("1.0/3.0");
    std::feclearexcept(FE_ALL_EXCEPT); r = std::sqrt(-one);  report("sqrt(-1)");
    (void)r;
}
```

Output with GCC 15 and Clang 22 on arm64 macOS, and GCC 16 on arm64 Linux (glibc):

```
1.0/0.0    FE_DIVBYZERO
1.0/3.0    FE_INEXACT
sqrt(-1)   FE_INVALID
```

### Changing the rounding mode

```cpp
#include <cfenv>
#include <cmath>
#include <cstdio>

#pragma STDC FENV_ACCESS ON

volatile double one = 1.0, three = 3.0;

int main() {
    const int saved = std::fegetround();
    const struct { int mode; const char* name; } modes[] = {
        {FE_TONEAREST, "FE_TONEAREST"}, {FE_UPWARD, "FE_UPWARD"},
        {FE_DOWNWARD, "FE_DOWNWARD"},   {FE_TOWARDZERO, "FE_TOWARDZERO"},
    };
    for (auto [mode, name] : modes) {
        std::fesetround(mode);
        std::printf("%-14s rint(2.5)=%4.1f rint(-2.5)=%4.1f 1/3=%.20f\n",
                    name, std::rint(2.5), std::rint(-2.5), one / three);
    }
    std::fesetround(saved);       // always put it back
}
```

```
FE_TONEAREST   rint(2.5)= 2.0 rint(-2.5)=-2.0 1/3=0.33333333333333331483
FE_UPWARD      rint(2.5)= 3.0 rint(-2.5)=-2.0 1/3=0.33333333333333337035
FE_DOWNWARD    rint(2.5)= 2.0 rint(-2.5)=-3.0 1/3=0.33333333333333331482
FE_TOWARDZERO  rint(2.5)= 2.0 rint(-2.5)=-2.0 1/3=0.33333333333333331482
```

On glibc, macOS, FreeBSD and the Windows UCRT, `printf` also rounds its
decimal conversion in the current mode, so the last digit of the `1/3` column
is partly printf's: `FE_TONEAREST`, `FE_DOWNWARD` and `FE_TOWARDZERO` compute
the same `double` (the nearest one is below 1/3), and only `FE_UPWARD`
produces a different one. MinGW-w64's own `printf` (which libstdc++ selects
with `__USE_MINGW_ANSI_STDIO`) ignores the mode and printed `…31483`,
`…37034`, `…31483`, `…31483` (MSYS2 UCRT64 GCC 16 in
[run 41368084884](https://github.com/homer6/cpp-docs/actions/runs/41368084884)).

`std::rint` and `std::nearbyint` follow the current mode (`nearbyint` never
raises `FE_INEXACT`); `std::round` always rounds halves away from zero and
`std::lround` ignores the mode too.

### Hiding intermediate flags from the caller

```cpp
#include <cfenv>
#include <cstdio>

#pragma STDC FENV_ACCESS ON

volatile double zero = 0.0;

double quiet_reciprocal(double x) {
    std::fenv_t env;
    std::feholdexcept(&env);                     // save + clear flags
    volatile double r = 1.0 / x;                 // volatile: GCC may otherwise move the
                                                 // division past fetestexcept (see Gotchas)
    bool divbyzero = std::fetestexcept(FE_DIVBYZERO);
    std::feclearexcept(FE_DIVBYZERO);            // swallow this one...
    std::feupdateenv(&env);                      // ...restore, re-raise the rest
    return divbyzero ? 0.0 : r;
}

int main() {
    std::feclearexcept(FE_ALL_EXCEPT);
    double r = quiet_reciprocal(zero);
    std::printf("r=%g caller sees FE_DIVBYZERO: %d\n", r,
                std::fetestexcept(FE_DIVBYZERO) != 0);   // r=0 caller sees FE_DIVBYZERO: 0
}
```

### How does `<cmath>` report errors here?

```cpp
#include <cmath>
#include <cstdio>

int main() {
    // MATH_ERRNO: domain/range errors set errno; MATH_ERREXCEPT: they raise FP flags
    std::printf("math_errhandling=%d errno:%d flags:%d\n", math_errhandling,
                (math_errhandling & MATH_ERRNO) != 0,
                (math_errhandling & MATH_ERREXCEPT) != 0);
}
```

On macOS (both compilers) `math_errhandling` is `2` (flags only — `errno` is
never set by `sqrt(-1)`); glibc reports `3` (both). Code that checks `errno`
after `std::log(0.0)` is therefore not portable.

## Gotchas

- **`#pragma STDC FENV_ACCESS` support is implementation-defined, and GCC does
  not implement it.** GCC 15 warns `ignoring '#pragma STDC FENV_ACCESS'
  [-Wunknown-pragmas]` and keeps optimizing as if the default mode were in
  effect. Its documented substitute, `-frounding-math`, is marked experimental
  and still does not stop every transformation: the program below printed
  "folded" for `1.0 / 3.0` under `FE_UPWARD` with GCC 15 at `-O0`, `-O2` and
  `-O2 -frounding-math`. Clang 22 honours the pragma (or `-frounding-math` /
  `-ffp-model=strict` without it) and printed the upward-rounded value.
  Without the pragma, Clang folded too. GCC can also **move** an operation past
  `fetestexcept`: in the `quiet_reciprocal` example above, with a plain
  `double r`, GCC 15 at `-O2` performed the division after the test and the
  caller saw `FE_DIVBYZERO`. Use `volatile` operands and results (as these
  examples do) when the code must work on GCC.

  ```cpp
  #include <cfenv>
  #include <cstdio>
  #pragma STDC FENV_ACCESS ON
  int main() {
      std::fesetround(FE_UPWARD);
      double a = 1.0 / 3.0;              // may be folded at compile time
      std::fesetround(FE_TONEAREST);
      std::puts(a == 1.0 / 3.0 ? "folded" : "rounded upward");
  }
  ```
- **Which flags and modes exist is implementation-defined.** Each `FE_*` macro
  is defined only if the platform supports it; guard with `#ifdef FE_UPWARD`
  on unusual targets. Whether raising a flag traps is also platform-specific
  (and trapping is not part of `<cfenv>`).
- **Changing modes without `FENV_ACCESS ON` (where the compiler honours it) is
  undefined behaviour** per C. Compile-time evaluation (`constexpr`) always uses
  round-to-nearest and never sets runtime flags. It bites even functions that
  "ignore the mode": on x86-64 with SSE4.1 (the macOS default), Clang expands
  `std::round` inline as `trunc(x + copysign(0.49999999999999994, x))`, and
  that addition rounds in the current mode. Without the pragma,
  `std::round(-2.5)` under `FE_UPWARD` gave `-2` (Apple Clang 15 under
  Rosetta, and Apple Clang 17/21 and Homebrew LLVM 22 on the macOS x86-64
  runners); with the pragma, `-frounding-math` or `-ffp-model=strict`, Clang
  calls the library's `round` and gets `-3`. arm64 has an exact instruction
  (`frinta`), and on Linux x86-64 the default target lacks SSE4.1, so `round`
  is a library call there.
- **Flags are sticky.** Always `feclearexcept` before the code you want to test.
- **Restore the rounding mode.** Other code, including the C library's
  `printf`, assumes round-to-nearest.
- **The environment is per thread.** Setting the mode in one thread does not
  affect a pool of workers already running, and a new thread may not inherit
  it (it doesn't on macOS) — set the mode inside the thread that computes.

## See also

- [Floating-point environment — cppreference](https://en.cppreference.com/w/cpp/numeric/fenv)
- [`math_errhandling` — cppreference](https://en.cppreference.com/w/cpp/numeric/math/math_errhandling)
- [`std::rint` / `nearbyint` — cppreference](https://en.cppreference.com/w/cpp/numeric/math/rint)
- [GCC: floating-point implementation-defined behaviour (FENV_ACCESS "not implemented")](https://gcc.gnu.org/onlinedocs/gcc/Floating-point-implementation.html)
- [GCC bug 34678 — `-frounding-math` does not stop all folding](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=34678)
- [Clang: controlling floating-point behaviour](https://clang.llvm.org/docs/UsersManual.html#controlling-floating-point-behavior)
- [Math functions](math.md) — the `<cmath>` functions these flags come from
