# Sources

Where each guidelines page came from, which edition it reflects, and when it was
last checked. The `refresh-guidelines` skill reads and updates this file — keep
it accurate.

## Tracked sources

| ID | Source | URL | Edition / version tracked | Last checked |
|---|---|---|---|---|
| `ccg` | C++ Core Guidelines (Stroustrup, Sutter) | https://github.com/isocpp/CppCoreGuidelines — submodule at `modules/CppCoreGuidelines` | Jun 14, 2026 (commit `d1dda82`, the submodule pin) | 2026-10-30 |
| `cppref` | cppreference.com — C++26 / C++23 / C++20 pages | https://en.cppreference.com/w/cpp/26 | C++26 (final) | 2026-10-30 |
| `deane-types` | Ben Deane, *Using Types Effectively* | CppCon 2016 | — | 2026-10-30 |
| `deane-misuse` | Ben Deane, *Easy to Use, Hard to Misuse: Declarative Style in C++* | C++Now 2018 | — | 2026-10-30 |
| `deane-composable` | Ben Deane, *Composable C++: Principles and Patterns* | CppCon 2021 | — | 2026-10-30 |
| `automerge-style` | automerge-cpp style guide and architecture plan | https://github.com/homer6/automerge-cpp (`docs/style.md`, `docs/plans/architecture.md`) | commit `10d71bc` and later | 2026-10-30 |
| `google-style` | Google C++ Style Guide | https://google.github.io/styleguide/cppguide.html | living page (targets C++20) | 2026-10-30 |
| `chromium` | Chromium — Modern C++ use in Chromium | https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++-features.md | living page (C++23 support began Jan 2026) | 2026-10-30 |
| `llvm-style` | LLVM Coding Standards | https://llvm.org/docs/CodingStandards.html | living page | 2026-10-30 |
| `abseil-totw` | Abseil Tips of the Week | https://abseil.io/tips/ | index checked; #163, #177, #197, #224 read in full | 2026-10-30 |
| `google-hardening` | Google Security Blog, *Retrofitting spatial safety to hundreds of millions of lines of C++* | https://security.googleblog.com/2024/11/retrofitting-spatial-safety-to-hundreds.html | Nov 15, 2024 | 2026-10-30 |
| `impl-status` | GCC C++ status; libc++ C++26 status | https://gcc.gnu.org/projects/cxx-status.html · https://libcxx.llvm.org/Status/Cxx26.html | live pages | 2026-10-30 |
| `perf-report` | perf-report — Linux performance analysis one-liners and references (homer6) | https://github.com/homer6/perf-report — submodule at `modules/perf-report` (also perf-report.com) | commit `bf0bac0` (Feb 2024) | 2026-10-30 |
| `gbench` | Google Benchmark user guide and tools docs | https://github.com/google/benchmark/blob/main/docs/user_guide.md · https://github.com/google/benchmark/blob/main/docs/tools.md | `main` (flags, `compare.py` U test checked) | 2026-10-31 |
| `perf-man` | perf-record(1) manual (man7 copy of the Linux tree) | https://man7.org/linux/man-pages/man1/perf-record.1.html | taken from the kernel git tree 2026-08-04 | 2026-10-31 |
| `fedora-fp` | Fedora change *fno-omit-frame-pointer* (F38) and its devel-list thread; Red Hat KB 7041624 | https://fedoraproject.org/wiki/Changes/fno-omit-frame-pointer · https://access.redhat.com/solutions/7041624 | change accepted for F38 (measurements on F37, Nov 2022); KB updated 2024-10-09 | 2026-10-31 |
| `valgrind` | Valgrind manual (quick start, Memcheck, Massif, DHAT) and project info | https://valgrind.org/docs/manual/ · https://valgrind.org/info/ | live docs | 2026-10-31 |
| `clang-sanitizers` | Clang docs: AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer, MemorySanitizer | https://clang.llvm.org/docs/AddressSanitizer.html (and siblings) | live pages | 2026-10-31 |
| `gcc-instrumentation` | GCC manual, Program Instrumentation Options (sanitizer flags and conflicts) | https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html | live page | 2026-10-31 |
| `libstdcxx-manual` | libstdc++ manual: macros, linking (`-lstdc++exp`), debug mode | https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_macros.html (and siblings) | live pages; GCC 13/14/15 editions compared | 2026-10-31 |
| `libcxx-hardening` | libc++ hardening modes | https://libcxx.llvm.org/Hardening.html | live page | 2026-10-31 |
| `msvc-stl-hardening` | Microsoft STL hardening wiki and VS 2022 17.14 STL release notes | https://github.com/microsoft/STL/wiki/STL-Hardening · https://github.com/microsoft/STL/releases/tag/vs-2022-17.14 | wiki (2025) · 17.14 release | 2026-10-31 |
| `msvc-asan` | Microsoft Learn, AddressSanitizer and known issues | https://learn.microsoft.com/en-us/cpp/sanitizers/asan | ms.date 2026-05-28 | 2026-10-31 |
| `testing-tools` | GoogleTest, Catch2, doctest, RapidCheck release pages; libFuzzer and llvm-cov docs | https://github.com/google/googletest · https://github.com/catchorg/Catch2 · https://github.com/doctest/doctest · https://github.com/emil-e/rapidcheck · https://llvm.org/docs/LibFuzzer.html | GoogleTest 1.18.0, Catch2 3.16.1, doctest 2.5.3, RapidCheck (no releases; pushed 2026-10-05); P2900R14 for contract testing | 2026-10-31 |

## Page → sources

| Page | Sources |
|---|---|
| [core-guidelines.md](core-guidelines.md) | `ccg`, `cppref` |
| [safety.md](safety.md) | `ccg`, `cppref`, `google-hardening`, `abseil-totw` |
| [modern-idioms.md](modern-idioms.md) | `cppref`, `impl-status`, `google-style`, `chromium` |
| [design-principles.md](design-principles.md) | `deane-*`, `automerge-style`, `abseil-totw`, `chromium` |
| [project-conventions.md](project-conventions.md) | `automerge-style`, `ccg` (F.16, NL), `google-style`, `chromium` |
| [performance-tooling.md](performance-tooling.md) | `ccg` (Per, SL.regex), `perf-report`, `gbench`, `perf-man`, `fedora-fp`, `valgrind`, `abseil-totw` (#197) |
| [industry-practice.md](industry-practice.md) | `google-style`, `chromium`, `llvm-style`, `abseil-totw` |
| [tooling.md](tooling.md) | `clang-sanitizers`, `gcc-instrumentation`, `libstdcxx-manual`, `libcxx-hardening`, `msvc-stl-hardening`, `msvc-asan`, `valgrind`, `impl-status` |
| [testing.md](testing.md) | `testing-tools`, `cppref` (contracts), `deane-*` (laws) |

## Candidate sources (not yet imported)

Notable references worth considering for a future import. Move a row up to
"Tracked sources" when it's imported.

| Source | Why it might be worth it |
|---|---|
| SEI CERT C++ Coding Standard | Security-focused rules (deep-research report couldn't establish its positions — needs a direct read) |
| Mozilla — Using C++ in Mozilla code | Another large-codebase feature subset (unverified in the report) |
| Bloomberg BDE | Component-contract conventions (report found no normative style policy) |
| Clang / MSVC conformance pages | Fill in the compiler table in modern-idioms.md |
| WG21 papers on `std::execution::task` design (P4007, P4089, P4123) | Open design concerns, per the report — read before recommending `task` broadly |
| Jason Turner, *C++ Best Practices* | Tooling, warnings, sanitizers |
| Sean Parent, *C++ Seasoning* / "no raw loops" | Origin of several design-principles ideas |

## Change log

| Date | Change |
|---|---|
| 2026-10-30 | Section created. Imported the `cpp-modern` skill (clustera-bridge, Dec 2025; CCG Jul 8 2025) and the automerge-cpp style guide. Updated to CCG Jun 14 2026 (new C.13, removed T.46, F.16 threshold 4 words, ES.23 slicing note) and C++26 (contracts, reflection, hardening, erroneous behaviour, `optional<T&>`, …). Fixed errors in the originals: invented `std::lazy`, invalid "defaulted lambda" example, `if consteval` labelled C++20 (it's C++23), wrong profile rule names (Type.2, Lifetime.2–4), backwards F.15 comment, `constexpr std::vector` overreach, and `std::reduce` needing commutativity. |
| 2026-10-30 | Core Guidelines added as a submodule at `modules/CppCoreGuidelines`, tracking `master`. Pulled upstream: still at `d1dda82` (Jun 14, 2026 edition), so no page changes. |
| 2026-10-30 | Full catch-up check. The original notes' copy of the Guidelines is upstream commit `cb798c4` (Dec 3, 2025; its header still says Jul 8, 2025). Reviewed all 16 commits `cb798c4..d1dda82`: C.13, T.46, F.16 and ES.23 were already covered; added the SL.regex performance note plus a short SL section. The rest were typo, CI or example-only fixes (C.48, ES.49, T.21, an added `const`, the GSL link). automerge-cpp `docs/style.md` and `docs/plans/architecture.md` are unchanged since import. |
| 2026-10-30 | Imported a Perplexity deep-research report (`.firecrawl/deep-research-2026-10-30.md`, used as a map only). Verified against primary sources before importing: libc++ status confirms `std::execution::task` (P3552, Sofia 2025) is C++26 and trivial relocation (P2786) was reverted by P3920 (Kona 2025); GCC 16 support for contracts, reflection (`-freflection`), expansion statements, erroneous behaviour; Google's 0.30% hardening overhead; Google/Chromium/LLVM policies; all 25 Abseil tip titles. Added `industry-practice.md`; hardening-in-practice and build configurations (safety); C++26 adoption section, `task`, relocation note (modern idioms); `const` data members, pipeline caveats, optional parameters, failure kinds (design principles); feature matrix, warnings, clang-tidy, modules (project conventions). Left out as unverified: Mozilla/BDE/CERT positions, Clang/MSVC support, the talk catalogue. |
| 2026-10-30 | Added `performance-tooling.md` (issue #2): workflow, benchmarking (Google Benchmark flags checked against its user guide), `perf`, flame graphs, off-CPU tracing and memory via perf-report links, reading generated code, common C++ costs. Linked from Per.6 in the digest. Deep-research prompt: `.claude/skills/refresh-guidelines/prompts/issue-2-performance-tooling.md`. |
| 2026-10-31 | Imported the issue #2 deep-research report (`.firecrawl/reports/issue-2-2026-10-31.md`, a map only). Verified and added: `compare.py` runs a Mann–Whitney U test and its docs ask for at least 9 repetitions (`gbench`); perf `fp`/`dwarf`/`lbr` unwinders, `fp` default, 8192-byte DWARF stack dump, LBR limits (`perf-man`); Fedora's frame-pointer measurements and F38 default, Red Hat's position (`fedora-fp`); Massif and DHAT (`valgrind`). Added a reproducibility checklist and flame-graph labelling advice as house recommendations. Left out as unverified: benchmark-library comparison, profiler licence/platform matrix, current Fedora/Ubuntu frame-pointer policy, continuous-profiling support, LTO/PGO/BOLT speedups. |
| 2026-10-30 | Added `tooling.md` (issue #1): symptom→tool table, ASan/UBSan/TSan/MSan with flags and slowdowns from the Clang docs, hardened library in debug, gdb/lldb and core dumps, `std::stacktrace`, Valgrind, clang-tidy/clang-format/IWYU. Deep-research prompt: `.claude/skills/refresh-guidelines/prompts/issue-1-tooling.md`. |
| 2026-10-31 | Imported the issue #1 deep-research report into `tooling.md` (only claims verified against primary sources): sanitizer combination table (GCC manual + Clang/GCC driver tests), GCC ASan enables use-after-scope, GCC pointer-pair checks, UBSan minimal runtime and the full runtime's production warning, TSan/MSan platforms, libstdc++ assertions on at `-O0` since GCC 15, MSVC STL hardening (17.14), MSVC `/fsanitize=address`, `std::stacktrace` link flags by GCC version, Memcheck 20–30×. Report: `.firecrawl/reports/issue-1-2026-10-31.md`. |
| 2026-10-30 | Added `testing.md` (issue #7): framework comparison, property-based tests, fuzzing, testing contracts, coverage, compile-time tests. Versions from GitHub release APIs. Deep-research prompt: `.claude/skills/refresh-guidelines/prompts/issue-7-testing.md` — import its report before treating the page as final. |
| 2026-10-31 | Imported the issue #7 deep-research report (`.firecrawl/reports/issue-7-2026-10-31.md`) into `testing.md`. Release versions re-checked against GitHub tags. Throwing contract-violation handler semantics added from P2900R14 §3.6.6 and [basic.contract.handler]. Noted GCC 13 `-fcontracts` is the pre-P2900 design and TSan can't be combined with ASan. Fixed the handler signature in `examples/language/basic-concepts/contracts.md` (`const contract_violation&`). Most of the report was unverified; remaining gaps are listed under "Still open" in the prompt. |
