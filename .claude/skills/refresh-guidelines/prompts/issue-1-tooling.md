# Deep research prompt — debugging and correctness tooling (issue #1)

Paste everything below the line into Perplexity Deep Research. Save the report
as `.firecrawl/deep-research-issue-1-tooling-<YYYY-MM-DD>.md`, then ask Claude
to run the `refresh-guidelines` skill in **import** mode on it, targeting
`docs/guidelines/tooling.md` (PR for https://github.com/homer6/cpp-docs/issues/1).

---

I maintain C++ coding guidelines for experienced engineers (targeting C++26,
with C++20/23 as the practical baseline). I'm writing a page on **tooling for
finding and fixing correctness bugs**: crashes, memory corruption, data races,
uninitialized reads, and undefined behaviour. I need a well-sourced research
report that verifies and extends my draft. Today's date is {{DATE}}.

## What I already cover (verify, correct, or add nuance — don't re-explain basics)

- AddressSanitizer as the first tool for segfaults: `-fsanitize=address
  -fno-omit-frame-pointer -g -O1`, ~2× slowdown, reading reports (access /
  free / allocation stacks), `ASAN_OPTIONS` (`detect_leaks`,
  `detect_stack_use_after_return`, `halt_on_error` with
  `-fsanitize-recover=address`, `log_path`), `llvm-symbolizer` /
  `ASAN_SYMBOLIZER_PATH`, LeakSanitizer on by default on Linux
- UBSan with `-fno-sanitize-recover=all` in CI, `UBSAN_OPTIONS=print_stacktrace=1`,
  `-fsanitize-trap=undefined` for production
- ThreadSanitizer (5–15× slowdown, can't combine with ASan/MSan)
- MemorySanitizer (Clang only, 3×, needs everything instrumented incl. an
  instrumented libc++, `-fsanitize-memory-track-origins`) and how C++26
  erroneous behaviour relates to it
- Hardened standard library in debug (libc++ `_LIBCPP_HARDENING_MODE_DEBUG`,
  libstdc++ `_GLIBCXX_ASSERTIONS` vs `_GLIBCXX_DEBUG`)
- gdb/lldb basics, core dumps (`ulimit -c`, `coredumpctl`), `-g`,
  `-gsplit-dwarf`, `std::stacktrace` (C++23)
- Valgrind Memcheck for binaries you can't rebuild
- Committed `.clang-tidy` / `.clang-format`, include-what-you-use,
  `CMAKE_CXX_CLANG_TIDY`, `CMAKE_CXX_INCLUDE_WHAT_YOU_USE`

## Questions to answer

1. **GCC vs Clang vs MSVC sanitizer support today:** which sanitizers each
   supports (ASan, UBSan, TSan, MSan, LSan, HWASan), on which platforms
   (Linux, macOS, Windows), with the minimum versions. What does MSVC's
   `/fsanitize=address` support and not support?
2. **Exact compatibility matrix:** which sanitizers can be combined in one
   build, and which flags conflict (e.g. ASan + TSan, ASan + MSan, UBSan +
   anything, sanitizers + `-static`, sanitizers + `_FORTIFY_SOURCE`).
3. **Current recommended flags and options** from the official docs: anything
   newer or better than my list (e.g. `detect_container_overflow`,
   `strict_string_checks`, `check_initialization_order`,
   `-fsanitize=pointer-compare`, `-fsanitize-address-use-after-scope`
   defaults). Which are on by default now?
4. **HWASan and MTE:** what hardware-assisted ASan and Arm Memory Tagging offer,
   on which platforms, and whether they're used in production (Android etc.).
5. **libstdc++ `std::stacktrace`:** the exact link flag by GCC version
   (`-lstdc++exp` vs `-lstdc++_libbacktrace`), and libc++/MSVC support status.
6. **libc++ hardening modes:** the current names and what each mode checks
   (`_LIBCPP_HARDENING_MODE_NONE/FAST/EXTENSIVE/DEBUG`); libstdc++'s
   equivalent; whether `_GLIBCXX_ASSERTIONS` is enabled by default by any
   distros/toolchains.
7. **Debugging practice:** recommended setup for production crash debugging
   (separate debug info, `debuginfod`, build IDs, symbol servers, minidumps
   with Crashpad/Breakpad), and rr / time-travel debugging — maturity and
   platforms.
8. **Valgrind today:** is it still recommended alongside sanitizers? Its
   current slowdown figures and limits (e.g. AVX-512, macOS support).
9. **clang-tidy and IWYU:** maintained status of include-what-you-use and
   alternatives (e.g. clangd's include cleaner, `misc-include-cleaner`);
   recommended clang-tidy check groups for catching *bugs* (not style).
10. **Anything I'm missing** that experienced C++ teams use to find
    correctness bugs (e.g. `-D_FORTIFY_SOURCE=3`, `-ftrivial-auto-var-init`,
    `-fhardened` in GCC 14+, sanitizer coverage in fuzzing, Dr. Memory on
    Windows).

## Output format

- Organize under the page's headings: **Which tool for which symptom ·
  AddressSanitizer · UndefinedBehaviorSanitizer · ThreadSanitizer ·
  MemorySanitizer · Hardened standard library · Debuggers and core dumps ·
  Valgrind · Static analysis and formatting**.
- For each point: **short imperative rule** → one-line reason → exact
  flag/option/command → toolchain + minimum version → source.
- Mark corrections to my list explicitly ("**Correction:** …").
- **Cite primary sources** — clang.llvm.org, gcc.gnu.org manuals,
  learn.microsoft.com, github.com/google/sanitizers wiki, valgrind.org,
  include-what-you-use.org, official man pages. No unsourced blogs.
- Give a **confidence level and source date** for every version or default.
- **Don't invent flags, options, or environment variables.** If something
  can't be verified, say so.
