# Tooling — finding and fixing correctness bugs

> What to reach for when a C++ program crashes, corrupts memory, races, or
> behaves differently in a release build. **Sources:** Clang sanitizer docs
> (AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer,
> MemorySanitizer, LeakSanitizer, HWASan), GCC instrumentation options,
> libstdc++ and libc++ manuals, Microsoft STL and MSVC AddressSanitizer docs —
> checked 2026-10-31 · pairs with [safety](safety.md#build-configurations),
> [debugging](debugging.md) and [Valgrind](valgrind.md)

[Safety](safety.md#build-configurations) lists which builds to keep; this page
is how to use them. The short version: **when something crashes, rebuild with a
sanitizer before you open a debugger.** A sanitizer report usually tells you the
bad access, where the memory was allocated, and where it was freed — the three
things a core dump can't.

## Which tool for which symptom

| Symptom | First tool | Then |
|---|---|---|
| Segfault, garbage values, "works in debug, crashes in release" | ASan (+ UBSan) | debugger on the ASan report's stack |
| Leak / memory keeps growing | LeakSanitizer (part of ASan, or [standalone](#leaksanitizer-lsan)) | heap profiler |
| Flaky failures under load, wrong results with threads | TSan | — |
| Value depends on uninitialized memory | MSan (Clang) | [Valgrind](valgrind.md) if you can't rebuild deps |
| Overflow, bad shift, misaligned access, null deref | UBSan | — |
| Bounds error in `vector`/`span`/`string` | Hardened standard library | ASan |
| Crash in production, no repro | [Core dump](debugging.md#core-dumps) + symbols | [`std::stacktrace`](debugging.md#stdstacktrace-c23) logging |
| Crashes or UB on hostile input | [Fuzzing](testing.md#fuzzing) (`-fsanitize=fuzzer`) with ASan + UBSan | — |

No single build runs every sanitizer — several can't be combined (see
[Combining sanitizers](#combining-sanitizers)). Keep one build per
investigation: ASan + UBSan, TSan, and MSan if you use it.

Hardening and sanitizers **complement each other**: library checks catch
invalid container operations (an out-of-range index), while ASan catches bad
pointer and iterator accesses the library can't see (Microsoft STL hardening
notes, 2025).

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
- Same flags on GCC (`g++ -fsanitize=address …`). GCC's
  `-fsanitize=address` already enables `-fsanitize-address-use-after-scope`;
  don't add it separately.
- **MSVC:** `/fsanitize=address` (since Visual Studio 2019 16.9, x86 and
  x64). Turn off `/RTC`, incremental linking and edit-and-continue (`/ZI`);
  Microsoft lists these, OpenMP and coroutines as incompatible.
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
| `detect_invalid_pointer_pairs=2` | Run-time switch for GCC's optional `-fsanitize=pointer-compare` / `-fsanitize=pointer-subtract` (GCC 8+, with ASan only), which flag `<` or `-` between pointers into different objects. Off at run time by default; `=1` checks only when both pointers are non-null |

```bash
ASAN_OPTIONS=detect_leaks=1:log_path=asan.log ./app
```

### Suppressions and runtime options

Each sanitizer reads options from an environment variable (`help=1` lists
them) and takes a suppressions file for reports you can't fix yet: one
`kind:pattern` per line, matching a function, file or module name in the
report's stack (`*` wildcards).

| Sanitizer | Run with | Suppression line |
|---|---|---|
| ASan leaks, standalone LSan | `LSAN_OPTIONS=suppressions=lsan.supp` | `leak:keep_forever` |
| TSan | `TSAN_OPTIONS=suppressions=tsan.supp` | `race:work` (also `deadlock:`, `called_from_lib:`) |
| UBSan | `UBSAN_OPTIONS=suppressions=ubsan.supp` | `signed-integer-overflow:ub.cpp` (a single check, not a group like `undefined`) |

Each row silenced its report (exit code 0) on macOS (Clang 22) and Linux
AArch64 (GCC 16, Clang 19). Prefer suppressions to an ignorelist
(`-fsanitize-ignorelist=`), which skips instrumenting the code entirely.

### Combining sanitizers

| Combination | Allowed? | Source |
|---|---|---|
| ASan + UBSan | ✅ yes — the usual pairing | Clang and GCC accept it |
| ASan + TSan | ❌ no | GCC manual; Clang driver rejects it |
| ASan + MSan | ❌ no | Clang driver rejects it |
| TSan + MSan | ❌ no | Clang driver rejects it |
| TSan + standalone LSan (`-fsanitize=leak`) | ❌ no | GCC manual |
| ASan + HWASan | ❌ no | GCC manual |
| TSan + UBSan | ✅ yes | Clang accepts it |

Clang's rows were checked by running the driver (Apple clang 15,
`-target x86_64-linux-gnu`). GCC's come from its
[instrumentation options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html).

## UndefinedBehaviorSanitizer (UBSan)

Catches signed overflow, invalid shifts, misaligned and null pointer use,
out-of-range enum and `bool` values, bad casts, and more. Cheap enough to
combine with ASan:

```bash
clang++ -g -fsanitize=address,undefined -fno-sanitize-recover=all app.cpp
UBSAN_OPTIONS=print_stacktrace=1 ./app
```

By default UBSan **prints and keeps running**. Pick the failure behaviour
for each build on purpose:

| Build | Flag | Behaviour |
|---|---|---|
| CI | `-fno-sanitize-recover=all` | First finding reports and fails the run |
| Production, crash on failure | `-fsanitize-trap=undefined` | Trap instruction, no runtime library, no message |
| Production, log on failure | `-fsanitize=undefined -fsanitize-minimal-runtime` | Small runtime with basic logging and deduplication; no `vptr` check |

**UBSan suppressions only cover recoverable checks:** under
`-fno-sanitize-recover=all` a suppressed overflow still aborted (Clang 22,
GCC 16), and the minimal runtime doesn't read `UBSAN_OPTIONS` at all. To suppress and still fail CI,
keep the checks recoverable and run with `UBSAN_OPTIONS=halt_on_error=1`.

Don't ship the full UBSan runtime: Clang's docs say it's meant for testing and
may weaken the executable's security, and recommend the minimal runtime or trap
mode for security-sensitive code.

## ThreadSanitizer (TSan)

Finds data races and some deadlocks. Typical slowdown **5–15×**, memory
**5–10×**.

```bash
clang++ -O1 -g -fsanitize=thread app.cpp
```

- **Can't be combined with ASan or MSan** — keep a separate TSan build.
- 64-bit only; platforms are in the [platform support](#platform-support).
- A clean run means no races *on the paths your tests exercised* — not that
  the program is race-free.
- Every library that touches shared data must be built with TSan, or races
  through it go unseen.
- A TSan report shows both conflicting accesses and the stacks that created the
  threads. Fix the design (CP.2, CP.3), not just the line.
- A report makes the process exit with 66 on Linux; on macOS it aborts
  (exit 134), because `abort_on_error` defaults to on for Apple targets —
  `TSAN_OPTIONS=abort_on_error=0` gives 66 there too.

## LeakSanitizer (LSan)

ASan includes LeakSanitizer. For leak checking alone, link with
`-fsanitize=leak` (`clang++ -g -fsanitize=leak app.cpp`); it adds almost no
overhead and reports at exit, with exit code 23. Leaks found by ASan exit with
ASan's code, 1, so don't make CI look for 23. Leak checking doesn't work under
ptrace (a debugger or `strace`): LSan stops with a fatal error instead.

**Homebrew GCC on macOS:** GCC 15.2 ships no sanitizer runtimes — it accepts
`-fsanitize=leak` but links nothing, so nothing is reported, and the others
fail to link (`library 'asan' not found`). The GCC 16.2 arm64 bottle adds
`libasan` and `libubsan` but still has no TSan or LSan runtime. Use Clang
there.

## HWASan — ASan with hardware tags

`-fsanitize=hwaddress` finds ASan's bug classes by tagging the top byte of
each pointer, with much less memory overhead; detection is probabilistic (a
tag can match by chance). It targets **64-bit AArch64** (Linux, Android);
Clang also lists Linux RISC-V 64 and, experimentally, x86-64 with Intel LAM
(GCC needs `-mlam=u48`/`u57`). Linux has disabled LAM since 6.12 (and 6.6.59,
6.11.6) unless the kernel is built without CPU mitigations, so x86-64 HWASan
needs a custom kernel. Can't be combined with ASan or TSan.

## MemorySanitizer (MSan) — Clang only

Reports when a branch, pointer, or system call depends on an uninitialized
value. Typical slowdown **3×**.

```bash
clang++ -O1 -g -fsanitize=memory -fsanitize-memory-track-origins=2 app.cpp
```

- **All code must be instrumented, including the standard library** — in
  practice a self-built instrumented libc++. Uninstrumented code produces false
  reports. (The runtime intercepts common libc functions, so libc itself can
  stay uninstrumented.) This is what makes MSan harder to adopt than ASan.
- Linux, NetBSD and FreeBSD only — not macOS or Windows.
- `-fsanitize-memory-track-origins` reports where the uninitialized value came
  from, at extra cost.
- GCC has no MSan.

**C++26 erroneous behaviour doesn't remove the need.** Reading an
uninitialized variable is now *erroneous* rather than undefined, so the
optimizer can't exploit it — but it's still a bug (ES.20). MSan, compiler
warnings (`-Wuninitialized`), and initializing at declaration are how you find
and avoid it. See [safety](safety.md#c26-erroneous-behaviour).

## Platform support

Where each sanitizer ran here (it reported a seeded bug), and what the Clang
and GCC docs add:

| Sanitizer | Linux AArch64 (GCC 16, Clang 19) | macOS arm64 (Clang 22) | Docs also list |
|---|---|---|---|
| ASan, UBSan | ✅ | ✅ | Linux x86-64, Windows (MSVC ASan) |
| TSan | ✅ | ✅ | Linux x86-64 and PowerPC64, Android, FreeBSD, NetBSD |
| LSan (standalone) | ✅ | ✅ | Android, Fuchsia, NetBSD |
| HWASan | ✅ | ❌ unsupported | Android, Linux RISC-V 64; x86-64 experimental |
| MSan | ✅ (Clang only) | ❌ unsupported | Linux x86-64, FreeBSD, NetBSD |

## Hardened standard library in debug builds

Turn on the strongest library checks in debug and test builds. They're cheaper
than sanitizers and catch out-of-bounds `operator[]`, `front()` on an empty
container, and dereferencing an empty `optional`:

| Library | Debug/test setting | Notes |
|---|---|---|
| libc++ | `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG` | Lighter modes (`_FAST`, `_EXTENSIVE`) are meant for production; the default is whatever the vendor chose (`none` unless configured) |
| libstdc++ | `-D_GLIBCXX_ASSERTIONS` | Light, production-suitable checks. **On by default at `-O0` since GCC 15** (opt out with `-D_GLIBCXX_NO_ASSERTIONS`); off when optimizing |
| libstdc++ | `-D_GLIBCXX_DEBUG` | Heavier debug mode; changes container sizes, so don't pass containers between debug and non-debug code — build everything with it |
| MSVC STL | `/D_MSVC_STL_HARDENING=1` | Since VS 2022 17.14 (partial P3471R4), off by default there; violations call `__fastfail()` because C++26 contracts aren't implemented. Per-class macros (`_MSVC_STL_HARDENING_VECTOR`, …) opt in or out |

Microsoft plans to turn its hardening on by default in a later release; check
your version's release notes rather than assuming.

## Debuggers, core dumps and Valgrind

- **[Debugging](debugging.md)** — gdb and lldb side by side: breakpoints in
  templates, `catch throw`, watchpoints, printing STL containers, attaching,
  core dumps, rr and `std::stacktrace`.
- **[Valgrind](valgrind.md)** — Memcheck on an unmodified binary (about 20–30×
  slower), Helgrind and DRD for races, suppressions, vgdb. Don't run a
  sanitized binary under Valgrind.

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
  [MemorySanitizer](https://clang.llvm.org/docs/MemorySanitizer.html) ·
  [LeakSanitizer](https://clang.llvm.org/docs/LeakSanitizer.html) ·
  [HWASan design](https://clang.llvm.org/docs/HardwareAssistedAddressSanitizerDesign.html) ·
  [TSan suppressions](https://github.com/google/sanitizers/wiki/ThreadSanitizerSuppressions)
- [GCC instrumentation options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html) ·
  [libc++ hardening](https://libcxx.llvm.org/Hardening.html) ·
  [libstdc++ macros](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_macros.html) ·
  [Microsoft STL hardening](https://github.com/microsoft/STL/wiki/STL-Hardening) ·
  [MSVC AddressSanitizer](https://learn.microsoft.com/en-us/cpp/sanitizers/asan)
- [Debugging](debugging.md) · [Valgrind](valgrind.md) · [Testing](testing.md#fuzzing) ·
  [Memory & safety](safety.md) · [Project conventions](project-conventions.md)
