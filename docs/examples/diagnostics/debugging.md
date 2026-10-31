# Debugging support (`<debugging>`)

> Ask whether a debugger is attached, and stop the program at a breakpoint from
> code. **Header:** `<debugging>` · **Since:** C++26

## Mental model

Three free functions that put the usual `__builtin_debugtrap()` / `DebugBreak()` /
`IsDebuggerPresent()` tricks behind one portable name:

```
std::is_debugger_present()      -> bool   "is someone watching?"   (best effort)
std::breakpoint()               -> stop here, always (a trap with no debugger)
std::breakpoint_if_debugging()  -> stop here only if is_debugger_present()
```

They're for code that wants to drop into the debugger when something surprising
happens — a failed internal check, an "impossible" branch — without crashing a
production run that nobody is debugging.

## Declaration

```cpp nocompile
namespace std {
  void breakpoint() noexcept;
  void breakpoint_if_debugging() noexcept;
  bool is_debugger_present() noexcept;      // replaceable (C++26, P2810)
}
```

| Function | What it does |
|---|---|
| `breakpoint()` | Unconditional breakpoint. The standard leaves the mechanism to the implementation; if no debugger is attached the effect is implementation-defined (typically the process gets `SIGTRAP` and dies). |
| `breakpoint_if_debugging()` | `if (is_debugger_present()) breakpoint();` |
| `is_debugger_present()` | `true` if the implementation can tell a debugger is attached. A `false` answer is not a promise — the detection is best effort. |

`is_debugger_present` is **replaceable**: the program may define its own
`bool std::is_debugger_present() noexcept`, exactly like replacing
`operator new`. That lets you hook it to an environment variable, a test
harness, or a platform API the library doesn't know about.

## Compiler support

| Toolchain | `__cpp_lib_debugging` | Status |
|---|---|---|
| GCC 15.2 (libstdc++) | not defined | no `<debugging>` header |
| GCC 16.2 (libstdc++) | `202403L` | supported with `-std=c++26`; **link with `-lstdc++exp`** |
| Clang 22 + libc++ | not defined | no `<debugging>` header |

`202311L` is the original P2546 feature; `202403L` adds the replaceable
`is_debugger_present` (P2810). libstdc++ ships these functions in its
experimental library, so without `-lstdc++exp` you get
`undefined reference to std::is_debugger_present()`.

On GNU/Linux, libstdc++'s `is_debugger_present()` only answers `true` when the
process is traced by a program it recognises as a debugger (such as `gdb`); on
other POSIX systems any `ptrace` tracer counts (from the header's own comment).

## When to use it

- **Fits:** "this should never happen" paths in debug-and-release builds;
  test harnesses that want to stop in the debugger on the first failure; turning
  a log line into a stop when you are already debugging.
- **Anti-fits:** error handling. A breakpoint is not a substitute for an
  exception, `std::expected` or an assertion that terminates — in production
  nobody is attached, and `breakpoint()` without a debugger usually kills the
  process.
- [Contracts](../language/basic-concepts/contracts.md) (C++26) are the structured way to state preconditions;
  `breakpoint_if_debugging()` is a good thing to call from a custom
  violation handler.

## Examples

### Stop only when someone is watching

```cpp needs=debugging
#include <debugging>
#include <print>

int main() {
    std::println("debugger attached: {}", std::is_debugger_present());
    std::breakpoint_if_debugging();   // no-op when nobody is debugging
    std::println("still running");
}
```

Built with `g++ -std=c++26 dbg.cpp -lstdc++exp` (GCC 16.2) and run without a
debugger, this prints `debugger attached: false` then `still running`.

### A "can't happen" check that traps under the debugger

```cpp needs=debugging
#include <debugging>
#include <print>

void check(bool ok, const char* what) {
    if (!ok) {
        std::println(stderr, "internal check failed: {}", what);
        std::breakpoint_if_debugging();   // stop here, with the stack intact
    }
}
```

### Replacing `is_debugger_present`

```cpp needs=debugging
#include <cstdlib>
#include <debugging>

// The program's definition replaces the library's (C++26, P2810).
bool std::is_debugger_present() noexcept {
    return std::getenv("FORCE_DEBUGGER") != nullptr;
}
```

With this in the program, GCC 16.2 prints `false` for a plain run and `true`
under `FORCE_DEBUGGER=1`. But on GCC 16.2 the replacement only links while the
program calls nothing else from `<debugging>`: `breakpoint()`,
`breakpoint_if_debugging()` and `is_debugger_present()` share one object file in
`libstdc++exp.a`, so calling either breakpoint function pulls in the library's
`is_debugger_present` too and the link fails with
`multiple definition of 'std::is_debugger_present()'`.

## Gotchas

- **`breakpoint()` without a debugger ends the program.** On GCC 16.2 / Linux a
  bare `std::breakpoint()` run outside a debugger dies with
  `Trace/breakpoint trap` (exit status 133 = 128 + `SIGTRAP`). Use
  `breakpoint_if_debugging()` unless you really mean "always stop".
- **`false` doesn't mean "no debugger".** Detection is best effort and platform
  specific; libstdc++ on Linux matches the tracer's name against a list of known
  debuggers.
- **Link with `-lstdc++exp` on GCC 16.** The header compiles fine without it;
  the failure only shows at link time.
- **Replacing `is_debugger_present` doesn't work yet on GCC 16.2** if the
  program also calls `breakpoint()` or `breakpoint_if_debugging()` (see above).

## See also

- [`std::breakpoint` — cppreference](https://en.cppreference.com/w/cpp/utility/breakpoint)
- [`std::breakpoint_if_debugging` — cppreference](https://en.cppreference.com/w/cpp/utility/breakpoint_if_debugging)
- [`std::is_debugger_present` — cppreference](https://en.cppreference.com/w/cpp/utility/is_debugger_present)
- [P2546 — Debugging Support](https://wg21.link/p2546)
- [P2810 — `is_debugger_present` `is_replaceable`](https://wg21.link/p2810)
- [`[debugging]` in the working draft](https://eel.is/c++draft/debugging)
- [Stack traces](stacktrace.md) · [Contracts](../language/basic-concepts/contracts.md) ·
  [Feature-test macros](../feature-test-macros/README.md)
