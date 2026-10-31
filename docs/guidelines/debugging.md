# Debugging — gdb and lldb for C++

> The debugger commands that matter for C++, side by side for gdb and lldb.
> **Sources:** GDB manual, LLDB docs (GDB-to-LLDB command map), libstdc++
> pretty-printers, rr-project.org — checked 2026-10-31 · pairs with
> [tooling](tooling.md)

**Rebuild with a sanitizer first** ([tooling](tooling.md)): a sanitizer report
names the bad access and its allocation and free stacks. Open a debugger once
you know *what* went wrong, for logic bugs, or when all you have is a core
dump. Every command below was run with gdb 16.3 (Linux, `gcc:16` image) and
LLDB 22 (macOS); the transcripts are trimmed.

**Build for debugging:** `-g` always (release builds too; strip into a separate
file if size matters), `-Og` or `-O0` when you need to step, since optimized
code shows variables as `<optimized out>` and jumps between lines.
`-gsplit-dwarf` writes debug info to `.dwo` files and speeds up linking large
projects. On macOS the debug info lives in a `.dSYM` bundle next to the
binary: copy the binary without it and `file:line` breakpoints don't resolve.
On macOS, use lldb: gdb has no native Apple Silicon support (its Darwin
port is x86 only, and Homebrew builds the arm64 gdb for an x86-64 target),
and on Intel Macs it needs codesigning.

## Command map

| Task | gdb | lldb |
|---|---|---|
| Start with arguments | `gdb --args ./app a b` | `lldb -- ./app a b` |
| Break on a function | `break Widget::id` | `b Widget::id` |
| Break at a line, conditionally | `break app.cpp:20 if i == 5` | `br set -f app.cpp -l 20 -c 'i == 5'` |
| One template instantiation | `break scale<double>` | `br set -n 'scale<double>'` |
| Every instantiation (regex) | `rbreak ^scale<` | `br set -r '^scale<'` |
| One-shot breakpoint | `tbreak check` | `br set -n check -o true` |
| Stop when an exception is thrown | `catch throw` | `br set -E c++` |
| … when it's caught | `catch catch` | `br set -E c++ -w false -h true` |
| Stop when a variable changes | `watch total` | `watchpoint set variable total` |
| … only past a value | `watch total if total > 150` | `watchpoint modify -c 'total > 150' 1` |
| Backtrace, all threads | `bt`, `thread apply all bt` | `bt`, `bt all` |
| Select a frame | `frame 2`, `up`, `down` | `frame select 2`, `up`, `down` |
| Run to the end of this function | `finish` | `finish` |
| Locals | `info locals` | `frame variable` (`v`) |
| Print, with and without formatters | `p v`, `p/r v` | `p v`, `v --raw v` |
| Attach | `gdb -p PID` | `lldb -p PID` |
| Attach when it starts | — | `process attach -n app --waitfor` |
| Open a core file | `gdb ./app core` | `lldb -c core ./app` |
| Write a core of the live process | `gcore app.core` | `process save-core app.core` |
| Text UI | `tui enable`, `layout split` | `gui` |

More in LLDB's [GDB-to-LLDB command map](https://lldb.llvm.org/use/map.html).

## Breakpoints in templates and overloads

A template has one function per instantiation. `break scale<double>` stops in
one; `rbreak ^scale<` (gdb) or `br set -r '^scale<'` (lldb) sets a location in
each and prints the instantiations it matched. A plain `break scale` also sets
one location per instantiation or overload (gdb: `scale. (2 locations)`).

## Exceptions

`catch throw` / `br set -E c++` stop inside `__cxa_throw`, before the stack
unwinds — the backtrace still shows who threw. Select the thrower's frame to
see its locals:

```text
Catchpoint 2 (exception thrown), 0x0000fffff7cb2f3c in __cxa_throw () from libstdc++.so.6
(gdb) bt
#0  0x0000fffff7cb2f3c in __cxa_throw () from libstdc++.so.6
#1  0x0000000000401c54 in check (i=7) at dbg.cpp:13
#2  0x0000000000401df4 in main () at dbg.cpp:22
```

gdb takes a regex for the type: `catch throw std::runtime_error`. LLDB 22 has
no type filter for C++ exceptions; stop on every throw and read the backtrace.
In lldb, `-h true` alone stops on the throw *and* the catch
(`__cxa_begin_catch`); add `-w false` to stop only where it's caught.

**Watchpoints** stop on the write that changes a value — the fastest way to
find who corrupts a field. They're hardware-backed and few (LLDB reports 4 on
Apple arm64). A watch on a local ends when it goes out of scope; gdb's
`watch -l v[1]` watches the address instead of the expression.

## Printing STL containers

Both debuggers show standard containers as their elements, not their
internals, and gdb evaluates `v[1]` and `m.size()` through libstdc++'s
xmethods:

```text
(gdb) p v
$2 = std::vector of length 3, capacity 3 = {1, 2, 3}
(gdb) p m
$3 = std::map with 2 elements = {["a"] = 1, ["b"] = 2}
```

- **gdb** uses the Python pretty-printers that ship with libstdc++. They load
  automatically from a `libstdc++.so.*-gdb.py` script next to the library — if
  that library isn't under gdb's *auto-load safe path* (a GCC installed under
  `/usr/local`, as in the `gcc:16` image), gdb prints a warning at startup and
  you get `_M_impl` and `_M_start` instead. Add
  `add-auto-load-safe-path <dir>` to your gdbinit. `info pretty-printer` lists
  what loaded; `p/r` prints the raw object.
- **lldb** has built-in formatters for libc++ and libstdc++ (both checked on
  macOS, Clang 22 and GCC 15 binaries). `v --raw` turns them off.
- **Stepping:** `step` into `v[i]` lands in library code. LLDB skips functions
  matching `target.process.thread.step-avoid-regexp` (default `^std::`).
  libstdc++'s gdb script adds `skip` entries for `std::move`, `std::forward`,
  smart-pointer accessors and container `begin`/`size`/`operator[]` (since
  GCC 15, PR118260; present in 15.2 and 16, absent in 14; `info skip` lists
  them). Add your own with
  `skip -gfi '/usr/include/c++/*/bits/*'`.

## Attaching, startup files, TUI

`gdb -p PID` or `lldb -p PID` stops a running process; `detach` lets it
continue. On Linux, the Yama security module may forbid attaching to a process
that isn't your child — see `kernel.yama.ptrace_scope` in the
[Yama docs](https://www.kernel.org/doc/html/latest/admin-guide/LSM/Yama.html).

gdb reads `~/.config/gdb/gdbinit` (or `~/.gdbinit`); lldb reads `~/.lldbinit`.
Good first lines: `set print pretty on` and `add-auto-load-safe-path <dir>`
for gdb, `command alias bfl breakpoint set -f %1 -l %2` style aliases for
lldb. A project-local `.gdbinit` runs only if its directory is on the
auto-load safe path, and lldb ignores `./.lldbinit` unless
`settings set target.load-cwd-lldbinit true` — both warn instead, because the
file can run arbitrary commands.

**Text UI:** gdb's `tui enable` (or Ctrl-x a) shows source above the command
line; `layout split` adds disassembly. lldb's `gui` is a curses view of source,
threads and variables.

## Core dumps

A core dump is the process's memory at the crash, so you can debug it later:

```console
$ ulimit -c unlimited       # allow core files in this shell
$ ./app                     # Segmentation fault (core dumped)
$ gdb ./app core            # or: coredumpctl gdb  (systemd)
(gdb) bt
```

Where the file goes depends on `/proc/sys/kernel/core_pattern` (`core` in the
`gcc:16` image; systemd distros pipe it to `coredumpctl`). Pretty-printers work
on cores too. `gcore` (gdb) and `process save-core` (lldb) write a core of a
live process without killing it — handy for a hung service. On macOS a crash
left no core here even with `ulimit -c unlimited`; `process save-core` worked.
Sanitizers turn core dumps off on 64-bit targets (`disable_coredump=1`); to
get one from an ASan failure, run with
`ASAN_OPTIONS=disable_coredump=0:abort_on_error=1`.

Keep the exact binary and its debug symbols for every build you ship, or the
dump is unreadable.

## Record and replay (rr)

[rr](https://rr-project.org/) records a run and replays it deterministically
under gdb, including backwards: `rr record ./app`, then `rr replay` and
`watch -l x` + `reverse-continue` to find the last write. It needs Linux and
CPU performance counters (recent x86 and some ARM CPUs), so it doesn't run in
most containers or under Docker on macOS — not verified here.

## `std::stacktrace` (C++23)

Captures a stack trace in-process — useful for logging where an unexpected
error came from in production:

```cpp needs=stacktrace
#include <stacktrace>
#include <print>

void on_fatal_error() {
    std::println(stderr, "{}", std::stacktrace::current());
}
```

**Linking with libstdc++ depends on the version:** GCC 13 needs
`-lstdc++_libbacktrace`; GCC 14 and later use `-lstdc++exp`. Support also
depends on how the toolchain was built, so check with a compile-**and-link**
test in your build, not by testing that `<stacktrace>` exists. (libc++ and MSVC
STL linking not covered here yet.)

## See also

- GDB manual: [catchpoints](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Set-Catchpoints.html) · [pretty-printing](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Pretty-Printing.html) ·
  [auto-load safe path](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Auto_002dloading-safe-path.html) ·
  [skipping functions](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Skipping-Over-Functions-and-Files.html) ·
  [TUI](https://sourceware.org/gdb/current/onlinedocs/gdb.html/TUI.html) ·
  [core files](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Core-File-Generation.html)
- LLDB: [GDB-to-LLDB command map](https://lldb.llvm.org/use/map.html) ·
  [variable formatting](https://lldb.llvm.org/use/variable.html)
- [rr](https://rr-project.org/) · [rr README](https://github.com/rr-debugger/rr)
- [std::stacktrace](https://en.cppreference.com/w/cpp/utility/basic_stacktrace)
- [Tooling](tooling.md) · [Valgrind](valgrind.md) · [Performance tooling](performance-tooling.md)
