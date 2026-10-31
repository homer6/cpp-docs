# Print functions (`std::print`, `std::println`)

> Formatted output straight to a `FILE*` or an `std::ostream`, using
> `std::format` syntax — one call, one unit of output, Unicode-aware.
> **Header:** `<print>` (`FILE*` overloads), `<ostream>` (stream overloads) ·
> **Since:** C++23

## Mental model

`std::print(fmt, args...)` is `std::format(fmt, args...)` written straight to the
destination, without building a `std::string` for you to hand on:

```
std::println(stderr, "{} of {} done", i, n);
     │         │       └─ format string: checked at compile time
     │         └─ destination: FILE* (default stdout) or std::ostream&
     └─ print = no newline added · println = adds '\n'
```

This page is about the **output side** — destinations, the `vprint_*` family,
terminals and Unicode. The format-spec mini-language (`{:>8.2f}`, `{:#x}`,
custom formatters) lives on [`std::format` and `std::print`](../text-processing/formatting.md).

## Declaration

```cpp nocompile
// <print>
template <class... Args> void print  (format_string<Args...> fmt, Args&&... args);
template <class... Args> void print  (FILE* stream, format_string<Args...> fmt, Args&&... args);
template <class... Args> void println(format_string<Args...> fmt, Args&&... args);
template <class... Args> void println(FILE* stream, format_string<Args...> fmt, Args&&... args);
void println();                     // just "\n" to stdout   (P3142)
void println(FILE* stream);         // just "\n" to stream   (P3142)

void vprint_unicode   (string_view fmt, format_args args);
void vprint_unicode   (FILE* stream, string_view fmt, format_args args);
void vprint_nonunicode(string_view fmt, format_args args);
void vprint_nonunicode(FILE* stream, string_view fmt, format_args args);

// C++26 (P3107): the same, but format into a buffer first
void vprint_unicode_buffered   (FILE* stream, string_view fmt, format_args args);
void vprint_nonunicode_buffered(FILE* stream, string_view fmt, format_args args);
// (and overloads without the FILE*, writing to stdout)

// <ostream>
template <class... Args> void print  (ostream& os, format_string<Args...> fmt, Args&&... args);
template <class... Args> void println(ostream& os, format_string<Args...> fmt, Args&&... args);
void println(ostream& os);          // P3142
void vprint_unicode   (ostream& os, string_view fmt, format_args args);
void vprint_nonunicode(ostream& os, string_view fmt, format_args args);
```

| Piece | Meaning |
|---|---|
| `format_string<Args...>` | The format string is checked against the argument types **at compile time**; a mismatch is a compile error. |
| `print` → `vprint_unicode` or `vprint_nonunicode` | `print` forwards to `vprint_unicode` when the **ordinary literal encoding is UTF-8**, otherwise to `vprint_nonunicode`. |
| `vprint_unicode` | If the destination is a terminal that can display Unicode (the Windows console), write through the native Unicode API instead of as bytes. Invalid UTF-8 should be replaced with U+FFFD. |
| `vprint_nonunicode` | Write the formatted bytes as-is. |
| `v…` functions | Take a runtime `string_view` + type-erased `std::format_args` — the building block for your own wrappers. Errors in the format string throw `std::format_error`. |

**Errors:** a bad runtime format string throws `std::format_error`; a failed write
to a `FILE*` throws `std::system_error`; the stream overloads are formatted
output functions and set `badbit` like `operator<<` does.

## Compiler support

| Toolchain | `__cpp_lib_print` | Notes |
|---|---|---|
| GCC 15.2 (libstdc++) | `202211L` | `print`/`println` (incl. no-argument `println()`), `vprint_*`; no `*_buffered` |
| GCC 16.2 (libstdc++) | `202406L` | adds `vprint_*_buffered` and `enable_nonlocking_formatter_optimization` |
| Clang 22 + libc++ | `202207L` | `print`/`println` (incl. `println()`), `vprint_*`; no `*_buffered` |

`202207L` is the C++23 feature (P2093); `202403L` permits the efficient
implementation and adds the `*_buffered` functions (P3107); `202406L` extends that
to more standard types (P3235). All three accept the no-argument `println()` in
`-std=c++23` mode too.

## When to use it

- **Fits:** almost all console output in new code — it's type-safe, short,
  compile-time-checked, and avoids `std::cout`'s sticky format state
  (`std::hex` leaking into the next line).
- **Fits:** writing to `stderr` or any `FILE*`, and to any `std::ostream`
  (files, string streams) through the `<ostream>` overloads.
- **Anti-fit:** when you need the text as a `std::string` — call `std::format`.
- **Compared with `std::cout <<`:** same destination, but one call formats the
  whole message, so it doesn't get split across `<<` operators. For
  cross-thread line integrity on streams, see [`std::osyncstream`](syncstream.md).
- **Compared with `printf`:** same `FILE*` model, but type-checked and
  extensible with `std::formatter`.

## Examples

### Destinations

```cpp
#include <cstdio>
#include <iostream>
#include <print>
#include <sstream>

void destinations() {
    std::print("a={} ", 1);                    // stdout, no newline
    std::println("b={}", 2);                   // stdout, newline
    std::println(stderr, "warning: {}", 3);    // any FILE*
    std::println();                            // blank line

    std::ostringstream os;
    std::println(os, "into a stream {}", 4);   // <ostream> overload
    std::print(std::cout, "{}", os.str());
}
```

Output on all three toolchains: `a=1 b=2`, a blank line, `into a stream 4`, with
`warning: 3` on stderr.

### Your own `print`-like function

Take a `std::format_string` for compile-time checking, then forward to the
type-erased `vprint_*`:

```cpp
#include <cstdio>
#include <format>
#include <print>
#include <string_view>

void log_impl(std::string_view fmt, std::format_args args) {
    std::fputs("[log] ", stderr);
    std::vprint_unicode(stderr, fmt, args);
    std::fputc('\n', stderr);
}

template <class... Args>
void log_line(std::format_string<Args...> fmt, Args&&... args) {
    log_impl(fmt.get(), std::make_format_args(args...));
}

// log_line("{} items in {} ms", 42, 3.5);      // ok
// log_line("{} items", "x", 1);                // compile error: too many args
```

### Unicode text

```cpp
#include <print>

void greet() {
    std::println("héllo ✓ — {}", "naïve");     // UTF-8 source, UTF-8 output
}
```

On macOS/Linux terminals the bytes go out unchanged. On Windows the
`vprint_unicode` path writes to the console through the native Unicode API, so
this prints correctly regardless of the console code page — **provided the
ordinary literal encoding is UTF-8** (MSVC: compile with `/utf-8`); otherwise
`print` uses `vprint_nonunicode` and you get the code page's interpretation of
the bytes.

### Buffered variants (C++26)

```cpp needs=print_buffered
#include <cstdio>
#include <format>
#include <print>

void buffered() {
    // Format the whole message first, then write it under one lock.
    std::vprint_unicode_buffered(stdout, "{} {}\n", std::make_format_args("all", "at once"));
}
```

C++26 (P3107) lets `print` write *while* formatting, holding the `FILE*` lock,
for types whose formatter is marked safe with
`std::enable_nonlocking_formatter_optimization`; the `*_buffered` functions
keep the old format-then-write behaviour. Compiled and run with GCC 16.2.

## Gotchas

- **`print` doesn't add a newline; `println` does.** `std::print("x")` followed
  by a crash may never show `x` — `stdout` is usually line-buffered on a
  terminal, fully buffered when redirected.
- **Mixing with `std::cout` is fine by default.** `std::cout` is synchronised
  with C `stdio` unless you call `std::ios::sync_with_stdio(false)`; after that,
  `std::print(...)` (to `stdout`) and `std::cout << …` can come out of order.
- **Runtime format strings need `std::vprint_*`.** `std::print(user_string)`
  won't compile — the format string must be a constant expression. C++26 adds a
  wrapper for this, spelled `std::runtime_format` in GCC 15.2 and
  `std::dynamic_format` in GCC 16.2 (`__cpp_lib_format` `202603L`); libc++ 22
  has neither. See [formatting](../text-processing/formatting.md).
- **Arguments to `make_format_args` must be lvalues.** `std::format_args` only
  refers to them; pass variables, not temporaries you computed inline.
- **The Windows Unicode path depends on the build's literal encoding**, not on
  the console. Without `/utf-8` (MSVC) the non-Unicode path is used.
- **Writing a custom formatter that touches the same `FILE*`?** Don't opt it
  into `enable_nonlocking_formatter_optimization` — under the C++26 direct-write
  path, the stream is locked while your formatter runs.

## See also

- [`std::print` — cppreference](https://en.cppreference.com/w/cpp/io/print)
- [`std::println` — cppreference](https://en.cppreference.com/w/cpp/io/println)
- [`std::vprint_unicode` — cppreference](https://en.cppreference.com/w/cpp/io/vprint_unicode)
- [`std::vprint_nonunicode` — cppreference](https://en.cppreference.com/w/cpp/io/vprint_nonunicode)
- [`std::print(std::ostream&)` — cppreference](https://en.cppreference.com/w/cpp/io/basic_ostream/print)
- [`std::enable_nonlocking_formatter_optimization` — cppreference](https://en.cppreference.com/w/cpp/utility/format/enable_nonlocking_formatter_optimization)
- [P2093 — Formatted output](https://wg21.link/p2093)
- [P3107 — Permit an efficient implementation of `std::print`](https://wg21.link/p3107)
- [P3142 — Printing blank lines with `std::println()`](https://wg21.link/p3142)
- [P3235 — `std::print` more types faster with less memory](https://wg21.link/p3235)
- [`std::format` and `std::print`](../text-processing/formatting.md) ·
  [`std::osyncstream`](syncstream.md) · [Streams & state](streams.md)
