# The preprocessor

> A text-substitution pass that runs *before* the compiler sees your code. Powerful
> but blunt — modern C++ replaces most of its historical uses.

## #include

```cpp nocompile
#include <vector>        // angle brackets: standard/system headers
#include "myheader.h"    // quotes: project headers (searched locally first)
```

Literally pastes the file's text in. Guard headers against double-inclusion:

```cpp
#pragma once             // simplest, widely supported (preferred)
// or the classic include guard:
#ifndef MYHEADER_H
#define MYHEADER_H
// ...
#endif
```

To include a file's *bytes* rather than its text (images, shaders, tables), use
[`#embed`](embed.md) (C++26).

## Macros (#define)

```cpp
#define BUFFER_SIZE 1024            // object-like macro
#define SQUARE(x) ((x) * (x))       // function-like macro — note the parentheses!
#undef BUFFER_SIZE
```

Macros are dumb text replacement with **no type checking and no scope** — which is
exactly why to avoid them for constants and functions.

### Variadic macros and `__VA_OPT__` (C++20)

`...` takes any number of arguments, pasted with `__VA_ARGS__`.
`__VA_OPT__(x)` expands to `x` only when the variadic arguments aren't empty, which
fixes the dangling comma:

```cpp
#define LOG(fmt, ...) std::printf(fmt __VA_OPT__(,) __VA_ARGS__)

void demo() {
    LOG("starting\n");          // std::printf("starting\n")   — no trailing comma
    LOG("%d items\n", 3);       // std::printf("%d items\n", 3)
}
```

## Conditional compilation

```cpp
#if defined(_WIN32)
    // Windows path
#elif defined(__linux__)
    // Linux path
#endif

#ifdef DEBUG
    log("verbose");
#endif

#if __cpp_lib_format >= 201907L     // feature-test macros (the good use)
    #include <format>
#endif
```

`#elifdef X` and `#elifndef X` (C++23) are short for `#elif defined(X)` and
`#elif !defined(X)`. `__has_include(<header>)` (C++17) is true when the header
can be found, so an optional header can be included only if it exists:

```cpp
#ifdef USE_FAST_PATH
    // ...
#elifdef USE_SAFE_PATH          // C++23: #elif defined(USE_SAFE_PATH)
    // ...
#elifndef NDEBUG                // C++23: #elif !defined(NDEBUG)
    // debug build
#endif

#if __has_include(<print>)
    #include <print>            // only if this standard library ships it
#endif
```

A header that exists may still be unusable in this mode, so pair
`__has_include` with the library's feature-test macro when it matters.

This is the one role with no clean language replacement: platform/feature
detection (see [feature-test macros](../../feature-test-macros/README.md)).

## Useful predefined macros

```cpp body
const char* file = __FILE__;    // current file and line —
long line        = __LINE__;    //   prefer std::source_location (C++20)
const char* fn   = __func__;    // current function name (strictly a predefined variable, not a macro)
long version     = __cplusplus; // language version (e.g. 202302L)
assert(line > 0);               // from <cassert>; compiled out when NDEBUG is defined
```

## Prefer language features over macros

| Instead of a macro… | Use |
|---|---|
| `#define PI 3.14159` | `constexpr double pi = ...;` or [`std::numbers::pi`](../../numerics/math.md) |
| `#define MAX(a,b) ...` | a `constexpr`/template function (`std::max`) |
| `#define RED 0` | an `enum class` |
| `#define LOG(x) ...` | a function + [`std::source_location`](../../language-support/source-location.md) |
| header text sharing | [modules](../modules/README.md) (C++20) |

## Gotchas

- **Function-like macros need parentheses everywhere.** `#define SQ(x) x*x` makes
  `SQ(1+2)` expand to `1+2*1+2` = 5. Wrap the whole body and each argument:
  `((x)*(x))` — and even then, double-evaluation (`SQ(i++)`) bites. Use a function.
- **Macros ignore scope and namespaces** and can clobber identifiers across the
  whole TU (`#define max ...` breaks `std::max`). They're a frequent source of
  baffling errors.
- **`#pragma once` vs include guards:** `#pragma once` is simpler and nearly
  universal; include guards are 100% portable. Either is fine — just guard.
- **Don't test for `__VA_OPT__` with a feature macro:** none of GCC 15, GCC 16
  or Clang 22 defines a `__cpp_va_opt`. Test `__cplusplus >= 202002L` instead.
- **Reserve the preprocessor for include guards, conditional/platform
  compilation, and feature-test macros.** For constants, functions, and enums, the
  language does it better and safely.

## See also

- [Preprocessor — cppreference](https://en.cppreference.com/w/cpp/preprocessor)
- [Conditional inclusion — cppreference](https://en.cppreference.com/w/cpp/preprocessor/conditional)
- [Macros, `__VA_OPT__` and predefined macros — cppreference](https://en.cppreference.com/w/cpp/preprocessor/replace)
- [Source file inclusion, `__has_include` — cppreference](https://en.cppreference.com/w/cpp/preprocessor/include)
- [P2334: `#elifdef` and `#elifndef`](https://wg21.link/P2334) ·
  [P0306: `__VA_OPT__`](https://wg21.link/P0306)
