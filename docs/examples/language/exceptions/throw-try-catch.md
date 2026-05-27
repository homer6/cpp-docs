# throw / try / catch

> Raising an exception unwinds the stack until a matching handler is found,
> running destructors along the way.

## The basics

```cpp
#include <stdexcept>

int parse(std::string_view s) {
    if (s.empty()) throw std::invalid_argument{"empty input"};
    // ...
}

try {
    int n = parse(input);
} catch (const std::invalid_argument& e) {     // most-derived handlers first
    std::cerr << e.what() << '\n';
} catch (const std::exception& e) {             // base-class catch-all
    std::cerr << "error: " << e.what() << '\n';
} catch (...) {                                  // truly everything (rarely needed)
    std::cerr << "unknown error\n";
}
```

## Stack unwinding

When an exception is thrown, the runtime walks up the call stack looking for a
matching `catch`. As each scope is exited, **local objects' destructors run** —
this is what makes RAII exception-safe: locks release, files close, memory frees,
automatically.

```cpp
void f() {
    std::lock_guard lk{m};        // acquired
    risky();                       // if this throws...
}                                  // ...lk's destructor still runs → lock released
```

## Rethrow & nested exceptions

```cpp
catch (const std::exception&) {
    log();
    throw;                         // rethrow the SAME exception (preserves type)
}

// preserve a cause chain:
try { lowLevel(); }
catch (...) { std::throw_with_nested(std::runtime_error{"high-level failed"}); }
// later inspect with std::rethrow_if_nested / std::nested_exception
```

## Catching: throw by value, catch by const reference

```cpp
throw MyError{"msg"};                  // throw a value (a temporary)
catch (const MyError& e) { }           // catch by const reference (no slicing, no copy)
```

## Gotchas

- **Catch by `const&`, throw by value.** Catching by value slices derived
  exceptions to the base and copies; catching by pointer leaks/UB.
- **Order handlers most-derived first.** A `catch (const std::exception&)` before a
  specific type shadows it (and many compilers warn).
- **Never let an exception escape a destructor.** During unwinding, a second
  exception calls `std::terminate`. Destructors should be `noexcept` (they are by
  default) — handle/swallow internally.
- **`throw;` (bare) rethrows the active exception** preserving its dynamic type;
  `throw e;` re-throws a *copy* (and can slice). Use bare `throw;`.
- **Exceptions cross `noexcept` boundaries fatally** — throwing out of a
  `noexcept` function calls `std::terminate`. See
  [noexcept & safety](noexcept-and-safety.md).
- **Don't use exceptions for ordinary control flow** — for expected failures,
  [`std::expected`](../../utilities/expected.md) (C++23) is clearer and cheaper.

## See also

- [Throwing exceptions — cppreference](https://en.cppreference.com/w/cpp/language/throw)
- [try-catch block — cppreference](https://en.cppreference.com/w/cpp/language/try_catch)
- [std::nested_exception — cppreference](https://en.cppreference.com/w/cpp/error/nested_exception)
