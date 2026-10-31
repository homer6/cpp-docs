# Tooling — finding and fixing correctness bugs

> What to reach for when a C++ program crashes, corrupts memory, races, or
> behaves differently in a release build. **Sources:** Clang sanitizer docs
> (AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer,
> MemorySanitizer), checked 2026-10-30 · pairs with
> [safety](safety.md#build-configurations)

[Safety](safety.md#build-configurations) lists which builds to keep; this page
is how to use them. The short version: **when something crashes, rebuild with a
sanitizer before you open a debugger.** A sanitizer report usually tells you the
bad access, where the memory was allocated, and where it was freed — the three
things a core dump can't.

## Which tool for which symptom

| Symptom | First tool | Then |
|---|---|---|
| Segfault, garbage values, "works in debug, crashes in release" | ASan (+ UBSan) | debugger on the ASan report's stack |
| Leak / memory keeps growing | LeakSanitizer (part of ASan on Linux) | heap profiler |
| Flaky failures under load, wrong results with threads | TSan | — |
| Value depends on uninitialized memory | MSan (Clang) | Valgrind if you can't rebuild deps |
| Overflow, bad shift, misaligned access, null deref | UBSan | — |
| Bounds error in `vector`/`span`/`string` | Hardened standard library | ASan |
| Crash in production, no repro | Core dump + symbols | `std::stacktrace` logging |

## AddressSanitizer (ASan) — start here for segfaults

Detects use-after-free, heap/stack/global buffer overflows, use-after-return,
double free, and (on Linux) leaks. Typical slowdown is about **2×**, so it's
practical for the whole test suite.

```bash
clang++ -O1 -g -fsanitize=address -fno-omit-frame-pointer app.cpp -o app
./app
```

- `-g` and `-fno-omit-frame-pointer` give readable stack traces.
- `-O1` keeps it fast while preserving useful line information.
- Same flags on GCC (`g++ -fsanitize=address …`).
- Sanitize **every** object file and link with the same flag — mixing
  instrumented and uninstrumented code hides bugs.

### Reading an ASan report

```text
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x602000000010
READ of size 4 at 0x602000000010 thread T0
    #0 0x4f1c3a in Widget::id() const widget.cpp:12      ← the bad access
    #1 0x4f2a10 in main main.cpp:30
freed by thread T0 here:
    #0 ... in operator delete(void*)
    #1 0x4f2981 in main main.cpp:27                      ← where it was freed
previously allocated by thread T0 here:
    #0 ... in operator new(unsigned long)
    #1 0x4f2950 in main main.cpp:25                      ← where it was allocated
```

The first line names the bug class (`heap-use-after-free`,
`heap-buffer-overflow`, `stack-use-after-return`, `stack-buffer-overflow`,
`double-free`, …). Read the three stacks top-down: access, free, allocation.
That's usually enough to see the lifetime mistake.

If frames show only addresses, ASan couldn't find a symbolizer: install
`llvm-symbolizer` or point `ASAN_SYMBOLIZER_PATH` at it.

### Useful options

Set at run time with `ASAN_OPTIONS` (colon-separated):

| Option | Effect |
|---|---|
| `detect_leaks=1` | LeakSanitizer — on by default on Linux; turn on explicitly on macOS |
| `detect_stack_use_after_return=1` | Catch use of a returned function's stack frame (already on by default on Linux) |
| `halt_on_error=0` | Keep going after the first error (needs `-fsanitize-recover=address`) |
| `log_path=asan.log` | Write reports to a file (per-process suffix) |

```bash
ASAN_OPTIONS=detect_leaks=1:log_path=asan.log ./app
```

## UndefinedBehaviorSanitizer (UBSan)

Catches signed overflow, invalid shifts, misaligned and null pointer use,
out-of-range enum and `bool` values, bad casts, and more. Cheap enough to
combine with ASan:

```bash
clang++ -g -fsanitize=address,undefined -fno-sanitize-recover=all app.cpp
UBSAN_OPTIONS=print_stacktrace=1 ./app
```

By default UBSan **prints and keeps running**. In CI, use
`-fno-sanitize-recover=all` so the first finding fails the run.
`-fsanitize-trap=undefined` turns checks into a trap instruction with no runtime
library — small enough to consider for hardened production builds.

## ThreadSanitizer (TSan)

Finds data races and some deadlocks. Typical slowdown **5–15×**, memory
**5–10×**.

```bash
clang++ -O1 -g -fsanitize=thread app.cpp
```

- **Can't be combined with ASan or MSan** — keep a separate TSan build.
- Every library that touches shared data must be built with TSan, or races
  through it go unseen.
- A TSan report shows both conflicting accesses and the stacks that created the
  threads. Fix the design (CP.2, CP.3), not just the line.

## MemorySanitizer (MSan) — Clang only

Reports when a branch, pointer, or system call depends on an uninitialized
value. Typical slowdown **3×**.

```bash
clang++ -O1 -g -fsanitize=memory -fsanitize-memory-track-origins=2 app.cpp
```

- **All code must be instrumented, including the standard library** — in
  practice a self-built instrumented libc++. Uninstrumented code produces false
  reports. This is what makes MSan harder to adopt than ASan.
- `-fsanitize-memory-track-origins` reports where the uninitialized value came
  from, at extra cost.
- GCC has no MSan.

**C++26 erroneous behaviour doesn't remove the need.** Reading an
uninitialized variable is now *erroneous* rather than undefined, so the
optimizer can't exploit it — but it's still a bug (ES.20). MSan, compiler
warnings (`-Wuninitialized`), and initializing at declaration are how you find
and avoid it. See [safety](safety.md#c26-erroneous-behaviour).

## Hardened standard library in debug builds

Turn on the strongest library checks in debug and test builds. They're cheaper
than sanitizers and catch out-of-bounds `operator[]`, `front()` on an empty
container, and dereferencing an empty `optional`:

| Library | Debug/test setting | Notes |
|---|---|---|
| libc++ | `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG` | Lighter modes (`_FAST`, `_EXTENSIVE`) are meant for production |
| libstdc++ | `-D_GLIBCXX_ASSERTIONS` | Light, production-suitable checks |
| libstdc++ | `-D_GLIBCXX_DEBUG` | Heavier debug mode; changes the ABI, so build everything with it |

## Debuggers and core dumps

Use the debugger once a sanitizer has told you *what* went wrong, or for logic
bugs that aren't memory errors.

| Task | gdb | lldb |
|---|---|---|
| Run | `gdb --args ./app arg1` | `lldb -- ./app arg1` |
| Backtrace | `bt` | `bt` |
| All threads | `thread apply all bt` | `bt all` |
| Break on a function | `break Widget::id` | `b Widget::id` |
| Inspect | `print w`, `frame 2` | `p w`, `frame select 2` |

**Debug info:** build with `-g` even for release builds and strip into a
separate file if size matters. `-gsplit-dwarf` speeds up linking large
projects.

**Core dumps** let you debug a crash after the fact:

```bash
ulimit -c unlimited                 # allow core files in this shell
./app                               # crashes → core file
gdb ./app core                      # or: coredumpctl gdb   (systemd)
```

Keep the exact binary and its debug symbols for every build you ship, or the
dump is unreadable.

**`std::stacktrace` (C++23)** captures a stack trace in-process — useful for
logging where an unexpected error came from in production:

```cpp
#include <stacktrace>
#include <print>

void on_fatal_error() {
    std::println(stderr, "{}", std::stacktrace::current());
}
```

Toolchain support varies; libstdc++ needs an extra library at link time.

## Valgrind — when you can't rebuild

Valgrind's Memcheck finds memory errors and leaks in an **unmodified binary**
— no recompile, works with third-party libraries you can't instrument. The
cost: programs run many times slower than with ASan, so it's for targeted runs,
not the whole suite. Don't run a sanitized binary under Valgrind.

## Static analysis and formatting

Catch bugs before running anything, and keep the configuration in the repo so
everyone (and CI) runs the same checks.

- **Compiler warnings** — the baseline in
  [project conventions](project-conventions.md#warnings-and-static-analysis).
- **clang-tidy** — commit a `.clang-tidy` (starting point in
  [project conventions](project-conventions.md#warnings-and-static-analysis)).
  Run it in CI and in the editor; with CMake, set `CMAKE_CXX_CLANG_TIDY`.
- **clang-format** — commit a `.clang-format` and format on save, so reviews
  never discuss whitespace:

  ```yaml
  BasedOnStyle: LLVM
  ColumnLimit: 100
  ```

- **include-what-you-use** — keeps each file's `#include`s matching what it
  uses, which cuts rebuild times and stops headers compiling by accident. With
  CMake, set `CMAKE_CXX_INCLUDE_WHAT_YOU_USE`.

## See also

- [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) ·
  [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) ·
  [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html) ·
  [MemorySanitizer](https://clang.llvm.org/docs/MemorySanitizer.html)
- [Memory & safety](safety.md) · [Project conventions](project-conventions.md)
