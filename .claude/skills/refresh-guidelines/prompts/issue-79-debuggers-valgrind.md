# Deep research prompt — debuggers, Valgrind, sanitizer options (issue #79)

For https://github.com/homer6/cpp-docs/issues/79. Paste everything below the
line into Perplexity Deep Research. Save the report as
`.firecrawl/reports/issue-79-<YYYY-MM-DD>.md`, then ask Claude to run the
`refresh-guidelines` skill in import mode on it. Target pages:
`docs/guidelines/debugging.md`, `docs/guidelines/valgrind.md`,
`docs/guidelines/tooling.md` (sanitizer sections).

Local tools used to verify every imported claim: LLDB 22.1.8 and Clang 22
(macOS arm64); gdb 16.3, Valgrind 3.24, GCC 16.2 and Clang 19 in the Docker
`gcc:16` image (Linux AArch64). No x86-64 Linux, no rr.

---

I maintain C++ guidelines for experienced engineers, targeting C++26 with
C++20/23 as the practical baseline. I've written pages on **debugging C++ with
gdb and lldb**, **Valgrind**, and **sanitizer runtime options**, and need a
well-sourced report that checks and extends them. Today's date is {{DATE}}.
Use primary sources (GDB manual, LLDB docs, Valgrind manual, Clang and GCC
docs, rr README, kernel docs); give a source and date per fact and say so when
you can't confirm something.

## What I already cover (verified locally — verify, correct or add nuance)

- gdb ↔ lldb map: `rbreak ^f<` / `br set -r`, `break f.cpp:20 if …` /
  `br set -c`, `catch throw [regex]` / `br set -E c++` (LLDB 22 has no C++
  exception type filter), `catch catch` / `-w false -h true` (`-h true` alone
  stops on throw and catch), `watch -l`,
  `watchpoint modify -c`, `gcore` / `process save-core`,
  `process attach -n --waitfor`, `tui enable` / `gui`.
- libstdc++ printers auto-load via `libstdc++.so.*-gdb.py`, blocked outside the
  auto-load safe path; the hook adds `skip -rfu` entries for `std::move`
  etc. since GCC 15 (PR118260; absent in 14); xmethods make `p v[1]` work; LLDB's `step-avoid-regexp` is `^std::`;
  lldb formats both libc++ and libstdc++ containers.
- Valgrind: Memcheck flags (`--leak-check=full`, `--track-origins`,
  `--error-exitcode`, `--errors-for-leak-kinds`), misses stack/global
  overruns; Helgrind/DRD false positives on `std::atomic` handoffs; mangled
  names in suppressions; vgdb with `--vgdb-error=0`; macOS is Intel only, no
  upstream Apple Silicon port; x86-64 ISA support stops at AVX2.
- Sanitizers: suppression formats for LSan/TSan/UBSan; UBSan suppressions
  ignored under `-fno-sanitize-recover`; the minimal runtime doesn't read
  `UBSAN_OPTIONS` at all; standalone LSan exits 23 but ASan leak reports exit
  1; LSan fails under ptrace; TSan exits 66 (Linux) and aborts on macOS
  because `abort_on_error` defaults on for Apple (`abort_on_error=0` gives
  66); sanitizers disable core dumps on 64-bit (`disable_coredump=0`); HWASan
  on AArch64, Clang also lists RISC-V 64 and experimental x86-64 LAM, which
  Linux has disabled since 6.12 (still gated in 7.3 master); Homebrew GCC
  15.2 ships no sanitizer runtimes, the 16.2 arm64 bottle has `libasan` and
  `libubsan` but no TSan or LSan.
- gdb has no native macOS arm64 support (Darwin native config is x86 only).

## Still open (from the 2026-10-31 report, not yet verified or not imported)

- **rr:** CPU/kernel/VM requirements and container flags (`SYS_PTRACE`,
  seccomp), AArch64 LSE-atomics requirement, rr 5.10 changes — not run
  locally (no PMU access in Docker on macOS).
- **Debug info at scale:** DWARF 5 defaults (GCC 11+, Clang 14+, Apple
  targets), `.gdb_index`/`--debug-names`, GDB 18 index break, debuginfod in
  gdb/lldb and distro defaults.
- **Valgrind:** changes in 3.25–3.28 (`--num-callers` minimum 2, GPLv3,
  `--track-fds=bad`); macOS ceiling conflict (3.28 NEWS says Intel up to 13,
  the platforms page says 11); the out-of-tree arm64 macOS port; no
  maintainer preference between Helgrind and DRD; DRD's
  `_GLIBCXX_SYNCHRONIZATION_HAPPENS_*` recipe.
- **Sanitizer options for CI:** `detect_odr_violation`,
  `detect_container_overflow`, TSan `history_size`, `second_deadlock_stack`,
  `report_signal_unsafe`, gdb's ASLR default breaking TSan; MSVC ASan
  `continue_on_error`; LLDB `process save-core --style`.
- **HWASan / MTE:** Android/NDK requirements, MTE on Linux servers, whether a
  released kernel re-enables LAM.
- **New topics:** C++26 `<debugging>` (`std::breakpoint`,
  `std::is_debugger_present`), RealtimeSanitizer, TypeSanitizer, Clang
  lifetime-safety warnings, LLDB on Windows (MSVC STL formatters).

## Questions to answer

1. **rr** — current CPU/kernel requirements (Apple M-series under Linux VMs?
   AArch64 status), and whether it works in containers/CI.
2. **gdb on macOS arm64** — is there native support in current GDB releases?
3. **Debug info at scale** — `-gsplit-dwarf`, `.gdb_index`/`--gdb-index`,
   debuginfod (gdb and lldb support, distro defaults), DWARF 5 defaults in GCC
   and Clang.
4. **Valgrind on x86-64** — current ISA support (AVX-512?), latest release and
   notable changes since 3.24; Helgrind vs DRD guidance from the maintainers.
5. **Sanitizer runtime options** — the TSan/ASan/UBSan options that matter
   most in CI (e.g. `halt_on_error`, `second_deadlock_stack`,
   `report_signal_unsafe`), and MSVC's equivalents.
6. **HWASan / MTE** — status of HWASan on Linux AArch64 distros, and whether
   Arm MTE-based tools replace it.
7. **Anything I'm missing** that a C++ engineer debugging crashes or races
   should know in 2026.

## Output format

Headings matching my pages: *Debugging (gdb/lldb)*, *Core dumps*, *rr*,
*Valgrind*, *Sanitizer options and suppressions*, *HWASan*. Each fact with
source URL, source date and confidence.
