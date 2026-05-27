# std::source_location

> Capture the current file, line, column, and function name as a value — the
> type-safe replacement for `__FILE__`/`__LINE__`. **Header:** `<source_location>`
> · **Since:** C++20

## Mental model

`std::source_location::current()` records where it's *called*. Its real power: as
a **default argument**, it captures the **call site**, not the function
definition — so a logging helper learns where it was invoked without macros.

## Examples

### Logging that knows its caller

```cpp
#include <source_location>
#include <iostream>

void log(std::string_view msg,
         std::source_location loc = std::source_location::current()) {
    std::clog << loc.file_name() << ':' << loc.line()
              << " (" << loc.function_name() << "): " << msg << '\n';
}

void compute() {
    log("starting");      // prints THIS file/line/function, not log()'s
}
```

Because the default argument is evaluated at the call site, `loc` reflects
`compute()`'s location.

### What it exposes

```cpp
auto loc = std::source_location::current();
loc.file_name();        // const char*
loc.line();             // uint_least32_t
loc.column();           // uint_least32_t
loc.function_name();    // const char*
```

## Gotchas

- **Put `source_location::current()` as a default argument**, not inside the
  function body — otherwise it captures the helper's own location, defeating the
  purpose.
- **It replaces the `__FILE__`/`__LINE__` macros** with a first-class value that
  flows through normal function calls (macros can't).
- **`function_name()` formatting is implementation-defined** (may or may not
  include the signature) — don't parse it for logic.
- **C++23's `std::stacktrace`** is the tool when you need the *whole* call chain,
  not just the immediate caller.

## See also

- [std::source_location — cppreference](https://en.cppreference.com/w/cpp/utility/source_location)
