# std::stacktrace

> Capture and print the current call stack — portable stack traces, finally in the
> standard. **Header:** `<stacktrace>` · **Since:** C++23

## Mental model

`std::stacktrace::current()` snapshots the chain of calls that led to this point.
Each frame (`std::stacktrace_entry`) can report its function name, source file,
and line (when debug info is available). It's a container of entries — iterate it,
or stream the whole thing.

```
 main() → outer() → inner() → current()

 [0] inner() at app.cpp:6      ← frame 0 is the caller of current()
 [1] outer() at app.cpp:7
 [2] main    at app.cpp:10
 [3] __libc_start_main …       ← runtime frames below main
```

## Declaration

```cpp
template<class Allocator> class basic_stacktrace;
class stacktrace_entry;
```

| Piece | Meaning |
|---|---|
| `std::stacktrace` | `basic_stacktrace<std::allocator<stacktrace_entry>>` |
| `current()`, `current(skip)`, `current(skip, max_depth)` | Capture now, dropping the `skip` innermost frames and keeping at most `max_depth` |
| `size()`, `empty()`, `operator[]`, `begin()`/`end()` | Read-only container of entries |
| `entry.description()`, `source_file()`, `source_line()` | Function name, file, line — empty / 0 when unknown |
| `std::to_string(trace)`, `operator<<`, `std::format("{}", trace)` | Render one entry or the whole trace |

## Compiler support

| Toolchain | `__cpp_lib_stacktrace` | Notes |
|---|---|---|
| GCC 15.2 (Homebrew, macOS) | not defined | — |
| GCC 16.2 (Docker `gcc:16`, Linux) | `202011L` | link with `-lstdc++exp`; build with `-g` for files and lines |
| Clang 22 + libc++ | not defined | — |

## When to use it

- **Fits:** crash and fatal-error reports, "how did we get here" logging on rare
  error paths, attaching a trace to a custom exception at the throw site.
- **Anti-fits:** hot paths (capture and symbolization are slow), and the
  immediate caller only — [`std::source_location`](../language-support/source-location.md)
  is free by comparison.

## Examples

```cpp needs=stacktrace
#include <stacktrace>
#include <iostream>

void inner() {
    auto trace = std::stacktrace::current();
    std::cout << trace;                 // pretty-prints all frames
}

// inspect frames individually:
void frames() {
    for (const std::stacktrace_entry& f : std::stacktrace::current()) {
        f.description();                // function name
        f.source_file();                // file
        f.source_line();                // line
    }
}
```

### In error reporting

```cpp needs=stacktrace
#include <stacktrace>

void risky();

void report() {
    try {
        risky();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n at:\n" << std::stacktrace::current();
    }
}
```

(`std::format("{}", trace)` also works — there's a formatter.)

Note that the catch block captures the stack *of the handler*, not of the
`throw`: `risky()`'s frames are already gone. To keep them, capture the trace
where you throw and store it in the exception object.

## Gotchas

- **Needs debug/symbol info to be useful.** Without it (release builds stripped of
  symbols), entries may lack names, files, and lines — you get addresses only.
- **Capturing a trace is relatively expensive.** Don't grab one on a hot path;
  reserve it for error/diagnostic paths.
- **Support and quality vary.** It's C++23 and depends on a platform backend; some
  toolchains need a linker flag or a backtrace library to populate symbols
  (GCC 14+ on Linux: link with `-lstdc++exp`). Homebrew GCC 15 on macOS ships
  without it (`__cpp_lib_stacktrace` undefined); the examples here were checked
  with GCC 16.
- **A trace taken in a `catch` block doesn't show the throw site** (see above).
- **`std::pmr::stacktrace`** exists if you want to control the allocation of the
  frame storage.

## See also

- [std::stacktrace — cppreference](https://en.cppreference.com/w/cpp/utility/basic_stacktrace)
- [std::stacktrace_entry — cppreference](https://en.cppreference.com/w/cpp/utility/stacktrace_entry)
- [P0881 — A Proposal to add stacktrace library](https://wg21.link/p0881)
- [std::source_location](../language-support/source-location.md)
