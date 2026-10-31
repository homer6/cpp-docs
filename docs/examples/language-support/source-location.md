# std::source_location

> Capture the current file, line, column, and function name as a value — the
> type-safe replacement for `__FILE__`/`__LINE__`. **Header:** `<source_location>`
> · **Since:** C++20

## Mental model

`std::source_location::current()` records where it's *called*. Its real power: as
a **default argument**, it captures the **call site**, not the function
definition — so a logging helper learns where it was invoked without macros.

```
 void log(msg, loc = source_location::current());   ← evaluated per call…
 void compute() {
     log("starting");                               ← …here: loc = compute()'s line
 }
```

## Declaration

```cpp
struct source_location;
```

| Member | Meaning |
|---|---|
| `static consteval source_location current() noexcept` | The location of the call (or of the call site, when used as a default argument) |
| `source_location()` | An "unknown" location: empty strings, line and column 0 |
| `constexpr std::uint_least32_t line()`, `column()` | 1-based line; column is implementation-defined (0 if unknown) |
| `constexpr const char* file_name()`, `function_name()` | File as the compiler saw it; function name in an implementation-defined format |

It's small and trivially copyable — pass it by value.

## When to use it

- **Fits:** logging, assertion and `expect`-style helpers, test frameworks,
  tracing wrappers — anything that reports "where was I called from".
- **Anti-fits:** the whole call chain → [`std::stacktrace`](../diagnostics/stacktrace.md);
  token pasting or stringizing an expression (`#expr`) still needs a macro.

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
#include <source_location>

void where() {
    auto loc = std::source_location::current();
    loc.file_name();        // const char*
    loc.line();             // uint_least32_t
    loc.column();           // uint_least32_t
    loc.function_name();    // const char*
}
```

### With a variadic function

`template<class... Ts> void logf(Ts&&..., std::source_location = …)` doesn't
work: a pack that isn't last is deduced empty, so `logf("x", 1)` finds no
match. Put the location in a small wrapper type that is built from the format
string instead:

```cpp
#include <format>
#include <iostream>
#include <source_location>

struct fmt_loc {
    std::string_view fmt;
    std::source_location loc;
    fmt_loc(const char* f, std::source_location l = std::source_location::current())
        : fmt{f}, loc{l} {}
};

template<class... Args>
void logf(fmt_loc f, const Args&... args) {
    std::clog << f.loc.line() << ": "
              << std::vformat(f.fmt, std::make_format_args(args...)) << '\n';
}

void run() { logf("x = {}", 42); }   // reports run()'s line
```

## Gotchas

- **Put `source_location::current()` as a default argument**, not inside the
  function body — otherwise it captures the helper's own location, defeating the
  purpose.
- **It replaces the `__FILE__`/`__LINE__` macros** with a first-class value that
  flows through normal function calls (macros can't).
- **`function_name()` formatting is implementation-defined** (may or may not
  include the signature) — don't parse it for logic.
- **`column()` differs between compilers.** For the same call `log("hi")`, GCC 15
  reports the column of the `(` and Clang 22 the column of `log` — don't
  compare columns across toolchains.
- **C++23's `std::stacktrace`** is the tool when you need the *whole* call chain,
  not just the immediate caller.

## See also

- [std::source_location — cppreference](https://en.cppreference.com/w/cpp/utility/source_location)
- [std::source_location::current — cppreference](https://en.cppreference.com/w/cpp/utility/source_location/current)
- [std::stacktrace](../diagnostics/stacktrace.md)
