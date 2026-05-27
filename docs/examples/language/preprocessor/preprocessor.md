# The preprocessor

> A text-substitution pass that runs *before* the compiler sees your code. Powerful
> but blunt — modern C++ replaces most of its historical uses.

## #include

```cpp
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

## Macros (#define)

```cpp
#define BUFFER_SIZE 1024            // object-like macro
#define SQUARE(x) ((x) * (x))       // function-like macro — note the parentheses!
#undef BUFFER_SIZE
```

Macros are dumb text replacement with **no type checking and no scope** — which is
exactly why to avoid them for constants and functions.

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

This is the one role with no clean language replacement: platform/feature
detection (see [feature-test macros](../../feature-test-macros/README.md)).

## Useful predefined macros

```cpp
__FILE__   __LINE__              // prefer std::source_location (C++20)
__func__                        // current function name
__cplusplus                     // language version (e.g. 202302L)
assert(cond)                    // from <cassert>; compiled out when NDEBUG is defined
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
- **Reserve the preprocessor for include guards, conditional/platform
  compilation, and feature-test macros.** For constants, functions, and enums, the
  language does it better and safely.

## See also

- [Preprocessor — cppreference](https://en.cppreference.com/w/cpp/preprocessor)
- [Conditional inclusion — cppreference](https://en.cppreference.com/w/cpp/preprocessor/conditional)
- [Predefined macros — cppreference](https://en.cppreference.com/w/cpp/preprocessor/replace)
