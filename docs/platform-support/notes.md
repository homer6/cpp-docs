# Platform support notes

<!-- notes-run: 41707574171 -->

A high-level reading of the results: how far each platform, architecture and
compiler supports what this reference documents. Written by hand from the
v0.5.0 release runs and the local runs of the same day, and updated with every
full run:

- [41657155620](https://github.com/homer6/cpp-docs/actions/runs/41657155620)
  (every platform, every area; `dev` at 3c85772);
- [41689101415](https://github.com/homer6/cpp-docs/actions/runs/41689101415)
  (its 43 failed jobs again, after the fixes; 3835b98);
- [41707574171](https://github.com/homer6/cpp-docs/actions/runs/41707574171)
  (the 4 jobs still failing; 86f6c76).

Each job's result comes from the newest run that ran it, so a few jobs were
measured on an earlier commit than others. The per-toolchain detail is in the
toolchain pages ([`README.md`](README.md)) and in
[`../platform-support.md`](../platform-support.md).

## In one paragraph

Everything documented that a toolchain implements works: **0 test failures
in all 59 CI toolchain jobs and on the 3 local toolchains**. That includes
the suite under AddressSanitizer + UBSan (GCC 16, Clang 22, LLVM 23),
ThreadSanitizer (Clang 22 with libstdc++) and the hardened builds (GCC 16,
Clang 22 with libc++, LLVM 23, MSVC 19.51). Toolchains differ in how much they
implement, which shows up as skipped tests, each with its reason. **The
standard library and compiler version decide the level of support. The OS and
the CPU barely matter:** the same toolchain gives the same results on x86_64
and arm64, on Linux, macOS, Windows and FreeBSD alike, apart from what the
pages record per platform. **GCC 16 is the most complete**: it is the only
compiler with reflection, contracts, expansion statements and constexpr
exceptions. Clang with libstdc++ follows the version of libstdc++. MSVC lacks a
C++26 mode and several C++23/26 language features.

## By compiler

Tests passed out of 1544 (1541 on macOS); the rest are skipped, never failed.

| Compiler | Passed | Characterization |
|---|---|---|
| GCC 16 (Linux, macOS, MinGW) | 1485–1505 | The most complete. The only one with reflection, contracts, expansion statements, constexpr exceptions and erroneous behaviour, and nearly all of the C++26 library tested (`inplace_vector`, `function_ref`, `indirect`/`polymorphic`, `<debugging>`, `submdspan`). Missing: hive, linalg, senders, simd, RCU and hazard pointers (no one has them), and the private module fragment. |
| GCC 15 | 1423–1442 | Complete C++23 and early C++26 (`#embed`, pack indexing, `= delete("reason")`, placeholder `_`) and named modules; no reflection or contracts, and little of the C++26 library. |
| GCC 14 | 1365–1385 | C++23 with gaps: no `flat_map`/`flat_set`, `#embed` or pack indexing (`import std` isn't measured: its runners have CMake 3.31). FreeBSD 15.1's pkg GCC 14.2 can't build named modules (compiler crash, [#219](https://github.com/homer6/cpp-docs/issues/219)). |
| GCC 13 | 1323 | The oldest supported. No C++26 mode (built as C++23), no `<print>`, and CMake 3.25 on Debian 12 can't build modules. |
| Clang with libstdc++ | 1361–1448 | Follows libstdc++: Clang 23 with libstdc++ 16 (MSYS2 UCRT64) 1448, Clang 22 with libstdc++ 15 (Ubuntu 26.04) 1433–1435, Clang 18–23 with libstdc++ 14 (Ubuntu 24.04) 1361–1398. With libstdc++ 16, Clang gets most of GCC 16's C++26 library (`inplace_vector`, `function_ref`, `indirect`/`polymorphic`, `submdspan`). Known Clang/libstdc++ mismatches: constexpr `std::complex`, dynamic format widths with libstdc++ 15 and 16, and `ranges::to` and `bind_front` with libstdc++ 14 (Clang 20–23). |
| Clang 22–23 with libc++ | 1394–1406 | Strong C++23, named modules and `#embed`; no reflection or contracts. libc++ 22 lacks `std::generator`, several C++23 views (`cartesian_product`, `concat`, `cache_latest`), `spanstream` and `std::text_encoding`; libc++ 23 adds `views::concat` and `std::text_encoding`. Neither defines `__cpp_lib_syncbuf`, so the `osyncstream` tests skip. |
| Clang 18–21 with libc++ | 1325–1388 | Each version adds C++23 library pieces. Clang 18–19 lack deducing `this`, libc++ 18–19 lack `std::jthread`, libc++ before 22 lacks `views::zip`, libc++ 19–20 has a `ranges::contains` bug ([#218](https://github.com/homer6/cpp-docs/issues/218)), and libc++ 18 rejects a formatter whose `format` takes only `std::format_context` ([#384](https://github.com/homer6/cpp-docs/issues/384)). |
| Apple Clang 17 / 21 | 1335–1338 / 1373–1375 | Apple Clang 17 ships libc++ 19 and scores like Clang 18–19 (and has no `std::jthread`); Apple Clang 21 scores like Clang 20. No named modules: Xcode ships no `clang-scan-deps`, so CMake can't build them. |
| MSVC 19.51 (VS 2026) | 1372 | No C++26 mode (CMake builds it as C++23, `/std:c++latest`). Missing `#embed`, pack indexing, `= delete("reason")`, the placeholder `_`, `auto(x)`, labels at the end of a block, `<stdckdint.h>` and `<stdbit.h>`; `[[assume]]` is recognized. A P2718 range-for bug ([#222](https://github.com/homer6/cpp-docs/issues/222)). Same results on x64 and arm64. |
| MSVC 19.44 (VS 2022) | 1342 | As 19.51, plus no `[[assume]]`, deducing `this`, implicit move, `flat_map`/`flat_set` or `start_lifetime_as`, and it rejects constexpr virtual calls and `if consteval` with a parameter ([#223](https://github.com/homer6/cpp-docs/issues/223)). `import std` isn't measured (CMake 3.31 on its runner). |
| clang-cl 22 | 1393 | Clang's language support with the MS STL. Named modules work, but a module-linkage mangling bug (llvm/llvm-project#174064) limits one test, and CMake 4.4.5 doesn't build `import std` for clang-cl. |

## By platform

- **Linux** (Debian, Ubuntu; x86_64 and arm64): the reference platform, with
  the widest range of compilers (GCC 13–16, Clang 18–23 with both libraries).
  Most implementation-defined values in the pages are recorded for glibc.
- **macOS** (15 and 26; arm64 and Intel): Homebrew GCC 16 and LLVM match
  Linux. Homebrew GCC has two platform gaps: `nested_exception`
  ([#221](https://github.com/homer6/cpp-docs/issues/221)) and no
  `<stacktrace>`. The SDK has no `<uchar.h>` and no `quick_exit`. A
  `call_once` whose callable throws can't be retried with libstdc++ outside
  Linux (Homebrew GCC, FreeBSD's GCC 14, MSYS2; GCC PR 66146).
- **Windows**: MSVC, clang-cl and MSYS2 (GCC 16, Clang 23 with libstdc++ or
  libc++) all pass. LLP64 (32-bit `long`), 16-bit `wchar_t` and the MSVC ABI
  change some results, and 28 tests per MSVC toolchain skip because a page
  records only the GCC/Clang, glibc, macOS or Itanium-ABI value (bit-field
  layout, `MB_CUR_MAX`, `source_location` columns, …): these are gaps in what
  the pages record, not in MSVC.
- **FreeBSD** (14.5 and 15.1, x86_64): the base Clang (19, 21) and pkg GCC 14
  match their Linux versions, apart from:
  - libc details: no `mbrtoc8`, and, with GCC 14, no `<stdbit.h>` and a
    C-only `<stdckdint.h>`;
  - what the pages record only for glibc, and the Linux-only security tests;
  - GCC 14's stacktraces, which carry no descriptions, and its `call_once`;
  - the GCC 14.2 module crash on 15.1.

  The base libc++ runs the library-hardening death tests in a plain build,
  which Linux's libc++ doesn't.

## Architecture

x86_64 and arm64 give identical results for every toolchain tested on both
(Linux, macOS, MSVC). The only difference is two tests whose page records
`char` signedness and `WCHAR_MAX` for arm64 only. (Two pairs also differ in
`SecurityIntegers.StdckdintAvailability`, because their x86_64 and arm64 jobs
ran on commits before and after #373 changed that test.)

## Library hardening

The library-hardening death tests (an out-of-range `operator[]`, `front()` on
an empty vector, dereferencing an empty `optional` or an `expected` holding
an error) pass in every hardened job: GCC 16, Clang 22 with libc++, LLVM 23
and MSVC 19.51. Without a hardened build they also run where the library
turns its checks on by default: libstdc++ 15 and 16 in unoptimized builds
(GCC 15–16, and Clang on Ubuntu 26.04 and MSYS2 UCRT64), and FreeBSD's base
libc++. Elsewhere they skip.

## Not supported anywhere yet

No toolchain has hive, linalg, senders (`std::execution`), simd, RCU or hazard
pointers. Header units (CMake can't build them) and freestanding builds aren't
tested. The private module fragment builds with Clang 18–23, clang-cl and
MSVC 19.44 and 19.51; GCC 14–16 lack it, and Apple Clang can't build modules.

## Limits of these results

- The sanitizer and hardened jobs run the tests only, not the snippets.
  ThreadSanitizer runs with libstdc++ only: packaged libc++ isn't
  TSan-instrumented.
- Snippets are enforced only for GCC 15–16 and Clang 22 with libc++. On the
  other toolchains their failures are reported, not gated.
- `import std` is skipped where the runner's CMake is older than 4.3 (the
  apt.llvm.org Ubuntu 24.04 jobs, the `gcc` images, FreeBSD and MSVC 19.44's
  runner) and for clang-cl: a build tool limit, not a compiler one. Where CMake
  allows it (Homebrew GCC and LLVM on macOS, MSYS2, MSVC 19.51, Ubuntu 26.04),
  it builds.
- The jobs come from three runs on three commits of `dev` (see the top). A job
  that passed wasn't re-run for a fix that couldn't affect it.
- No GCC or Clang on Windows arm64, no FreeBSD arm64, and no emulated
  architectures (riscv64, s390x, ppc64le).
