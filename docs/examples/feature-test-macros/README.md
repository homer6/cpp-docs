# Feature-test macros

> Standard macros that tell you, at compile time, whether a specific language or
> library feature is available. **Header:** `<version>` (library macros) ·
> **Since:** C++20 (standardized; many predate it)

## Mental model

Instead of guessing from `__cplusplus` ("is this C++20?") and hoping the whole
standard landed at once, each feature has its own macro that's defined to a
**date value** (`YYYYMM`L) when that feature is present. You test the macro, not
the standard version — because compilers ship features incrementally.

```cpp
#include <version>          // pulls in all library feature-test macros

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
    // std::format is available
#else
    // fall back
#endif
```

## Two families

- **Language** macros: `__cpp_concepts`, `__cpp_constexpr`, `__cpp_modules`,
  `__cpp_consteval`, `__cpp_if_constexpr` … (predefined by the compiler, no
  header needed). A macro is only defined while its feature is on: GCC defines
  `__cpp_modules` only with `-fmodules`; Clang 22 and later set it to `1`, not a
  date, while Clang 21 and earlier (and Apple Clang 17 and 21, from Xcode 16.4
  and 26.6) leave it undefined even though they support modules
  ([llvm/llvm-project#161034](https://github.com/llvm/llvm-project/pull/161034)).
- **Library** macros: `__cpp_lib_*`, exposed via `<version>` (or the specific
  feature's own header): `__cpp_lib_ranges`, `__cpp_lib_expected`,
  `__cpp_lib_print`, `__cpp_lib_simd`, `__cpp_lib_linalg`, etc.

## Examples

```cpp
#include <version>

struct Error { int code; };

#if __cpp_lib_expected >= 202202L
    #include <expected>
    using Result = std::expected<int, Error>;
#else
    template<class T, class E> class MyExpectedPolyfill { /* ... */ };
    using Result = MyExpectedPolyfill<int, Error>;
#endif

#if __cpp_lib_ranges >= 201911L
    std::vector<int> data{1, 2, 3, 4};
    auto v = data | std::views::filter([](int x) { return x % 2 == 0; });
#endif
```

The value is a date; compare with `>=` against the value from cppreference's
"Feature-test macro" note on each feature's page.

## Gotchas

- **Test the value, not just `#ifdef`.** Features get *revised*; a later paper
  bumps the macro's date. Use `>= <date>` to require a specific revision.
- **Include `<version>` for library macros.** Don't rely on them leaking in from
  other headers; `<version>` is the portable, cheap way to get them all.
- **Don't lean on `__cplusplus` alone.** It says which standard was *requested*,
  not which features the implementation actually finished. Feature-test macros are
  the precise tool.
- **Each feature's cppreference page lists its macro and value** in a "Feature-test
  macro" box — that's the authoritative number to compare against.

## See also

- [Feature-test macros — cppreference](https://en.cppreference.com/w/cpp/feature_test)
- [`<version>` header — cppreference](https://en.cppreference.com/w/cpp/header/version)
