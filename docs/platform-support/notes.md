# Platform support notes

<!-- notes-run: 41618636506 -->

A high-level reading of the results: how far each platform, architecture and
compiler supports what this reference documents. Written by hand from the
results of run
[41618636506](https://github.com/homer6/cpp-docs/actions/runs/41618636506)
(2026-11-03, every platform, every area) and the local runs of the same day,
and updated with every full run. The per-toolchain detail is in the toolchain
pages ([`README.md`](README.md)) and in [`../platform-support.md`](../platform-support.md).

## In one paragraph

Everything documented that a toolchain implements works: **0 test failures
in all 55 CI jobs and on the 3 local toolchains**, including the suite under
AddressSanitizer + UBSan (GCC 16, Clang 22, LLVM 23) and ThreadSanitizer
(Clang 22 with libstdc++). Toolchains differ in how much they implement, which
shows up as skipped tests, each with its reason. **The standard library and
compiler version decide the level of support. The OS and the CPU barely
matter:** the same toolchain gives the same results on x86_64 and arm64, on
Linux, macOS, Windows and FreeBSD alike. **GCC 16 is the most complete**: it
is the only compiler with reflection, contracts, expansion statements and
constexpr exceptions. Clang with libstdc++ follows the version of libstdc++.
MSVC lacks a C++26 mode and several C++23/26 language features.

## By compiler

Tests passed out of 1464 (1461 on macOS); the rest are skipped, never failed.

| Compiler | Passed | Characterization |
|---|---|---|
| GCC 16 (Linux, macOS, MinGW) | 1411–1428 | The most complete. The only one with reflection, contracts, expansion statements, constexpr exceptions and erroneous behaviour, and nearly all of the C++26 library tested (`inplace_vector`, `function_ref`, `indirect`/`polymorphic`, `<debugging>`, `submdspan`). Missing: hive, linalg, senders, simd, RCU and hazard pointers (no one has them), and the private module fragment. |
| GCC 15 | 1358–1367 | Complete C++23 and early C++26 (`#embed`, pack indexing, `= delete("reason")`, placeholder `_`) and named modules; no reflection or contracts, and little of the C++26 library. |
| GCC 14 | 1302–1319 | C++23 with gaps: no `flat_map`/`flat_set`, `#embed`, pack indexing or std module. FreeBSD 15.1's pkg GCC 14.2 can't build named modules (compiler crash, [#219](https://github.com/homer6/cpp-docs/issues/219)). |
| GCC 13 | 1258 | The oldest supported. No C++26 mode (built as C++23), no `<print>`, and CMake 3.25 on Debian 12 can't build modules. |
| Clang with libstdc++ | 1295–1375 | Follows libstdc++: Clang 23 with libstdc++ 16 (MSYS2 UCRT64) 1375, Clang 22 with libstdc++ 15 (Ubuntu 26.04) 1357–1359, Clang 18–23 with libstdc++ 14 (Ubuntu 24.04) 1295–1331. Known Clang/libstdc++ mismatches: constexpr `std::complex`, dynamic format widths, `ranges::to` and `bind_front` with libstdc++ 14 (Clang 20–23). |
| Clang 22–23 with libc++ | 1331–1341 | Strong C++23, named modules and `#embed`; no reflection or contracts. libc++ 22 lacks `std::generator`, several C++23 views (`cartesian_product`, `concat`, `cache_latest`), `spanstream` and `std::text_encoding`; parallel algorithms and `osyncstream` need `-fexperimental-library`. |
| Clang 18–21 with libc++ | 1261–1322 | Each version adds C++23 library pieces. Clang 18–19 lack deducing `this`, libc++ before 22 lacks `views::zip`, and libc++ 19–20 has a `ranges::contains` bug ([#218](https://github.com/homer6/cpp-docs/issues/218)). |
| Apple Clang 17 / 21 | 1273–1275 / 1309–1311 | Apple Clang 17 ships libc++ 19 and scores like Clang 18–19 (and has no `std::jthread`); Apple Clang 21 scores like Clang 20. No named modules: Xcode ships no `clang-scan-deps`, so CMake can't build them. |
| MSVC 19.51 (VS 2026) | 1309 | No C++26 mode (CMake builds it as C++23, `/std:c++latest`). Missing `#embed`, pack indexing, `= delete("reason")`, `[[assume]]`, `auto(x)`, labels at the end of a block, `<stdckdint.h>`. A P2718 range-for bug ([#222](https://github.com/homer6/cpp-docs/issues/222)). Same results on x64 and arm64. |
| MSVC 19.44 (VS 2022) | 1279 | As 19.51, plus no deducing `this`, implicit move or constexpr virtual calls ([#223](https://github.com/homer6/cpp-docs/issues/223)). |
| clang-cl 22 | 1330 | Clang's language support with the MS STL. Named modules work, but a module-linkage mangling bug (llvm/llvm-project#174064) limits one test. |

## By platform

- **Linux** (Debian, Ubuntu; x86_64 and arm64): the reference platform, with
  the widest range of compilers (GCC 13–16, Clang 18–23 with both libraries).
  Most implementation-defined values in the pages are recorded for glibc.
- **macOS** (15 and 26; arm64 and Intel): Homebrew GCC 16 and LLVM match
  Linux. Homebrew GCC has three platform gaps: `call_once` with a throwing
  callable, `nested_exception` ([#221](https://github.com/homer6/cpp-docs/issues/221))
  and no `<stacktrace>`. The SDK has no `<uchar.h>` and no `quick_exit`.
- **Windows**: MSVC, clang-cl and MSYS2 (GCC 16, Clang 23 with libstdc++ or
  libc++) all pass. LLP64 (32-bit `long`), 16-bit `wchar_t` and the MSVC ABI
  change some results, and about 25 tests per MSVC toolchain skip because a page
  records only the GCC/Clang or Itanium-ABI value: these are gaps in what the
  pages record, not in MSVC.
- **FreeBSD** (14.5 and 15.1, x86_64): the base Clang (19, 21) and pkg GCC 14
  match their Linux versions, apart from libc details (no `mbrtoc8`) and the
  GCC 14.2 module crash on 15.1.

## Architecture

x86_64 and arm64 give identical results for every toolchain tested on both
(Linux, macOS, MSVC). The only difference is two tests whose page records
`char` signedness and `WCHAR_MAX` for arm64 only.

## Not supported anywhere yet

No toolchain has hive, linalg, senders (`std::execution`), simd, RCU or hazard
pointers. Header units (CMake can't build them) and freestanding builds aren't
tested. The private module fragment is tested with Clang only (GCC 15/16 lack
it; MSVC isn't tested yet).

## Limits of these results

- The sanitizer jobs run the tests only, not the snippets. ThreadSanitizer runs
  with libstdc++ only: packaged libc++ isn't TSan-instrumented.
- Snippets are enforced only for GCC 15–16 and Clang 22 with libc++. On the
  other toolchains their failures are reported, not gated.
- `import std` is skipped where the runner's CMake is older than 4.3 (most
  Linux and FreeBSD jobs): a build tool limit, not a compiler one. Where CMake
  allows it (macOS, MSYS2, MSVC 19.51, Ubuntu 26.04), it builds.
- No GCC or Clang on Windows arm64, no FreeBSD arm64, and no emulated
  architectures (riscv64, s390x, ppc64le).
