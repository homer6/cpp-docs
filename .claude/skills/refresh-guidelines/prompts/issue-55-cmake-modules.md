# Deep research prompt — CMake's C++ modules support (issue #55)

For https://github.com/homer6/cpp-docs/issues/55. Paste everything below the
line into Perplexity Deep Research. Save the report as
`.firecrawl/reports/issue-55-<YYYY-MM-DD>.md`, then ask Claude to run the
`refresh-guidelines` skill in import mode on it. Target pages:
`docs/guidelines/build-and-packaging.md` §3 (Modules),
`docs/examples/language/modules/`, and the test harness `tests/` (build
named modules with CMake).

Local toolchains used to verify every imported claim: CMake 4.3.4, Ninja,
Homebrew GCC 15.2, Homebrew Clang 22.1 + libc++, GCC 16.2 (Docker `gcc:16`,
Linux arm64). macOS arm64 host.

---

I maintain C++ guidelines and a reference for experienced engineers, targeting
C++26 with C++20/23 as the practical baseline. I'm rewriting my coverage of
**C++20 modules as built with CMake** — the build-system side in depth, not just
the language. CMake's modules support is extensive and I want all of it,
precisely, with the version each piece arrived in. Today's date is {{DATE}}.

## What I already cover or have verified (against CMake's `Help/` at the release tags, and built locally — don't re-explain)

The page keeps only the essentials (FILE_SET example, floor, `import std` status,
install/export, known issues); the rest below was verified and kept out of the
page on purpose.

- Named modules supported since **3.28** (3.25–3.27 experimental with per-release
  `CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API` UUIDs; 3.25/3.26 also needed
  `CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP`). Providers (interface units and both
  kinds of partition) go in `FILE_SET CXX_MODULES`; implementation units are
  ordinary sources; `module M:part;` is an internal partition (CMake errors if
  it's outside the file set); PRIVATE-module-from-PUBLIC-module rule.
- Floor: Ninja 1.11+ / Ninja Multi-Config, VS 17 2022 and VS 18 2026; MSVC
  14.34+, Clang 16+, clang-cl 19.1+ (4.4, not VS generators), GCC 14+. No
  header units, no Makefiles, no Xcode.
- Scanning decision order, CMP0155 (unset = OLD, no warning), file-set
  `CXX_SCAN_FOR_MODULES` (4.4), `set(CMAKE_CXX_SCAN_FOR_MODULES 0)` as a
  non-cache variable, unity-build exclusion.
- Install/export: `FILE_SET CXX_MODULES DESTINATION`, `CXX_MODULES_DIRECTORY`
  (required), `CXX_MODULES_BMI` (installed but unused by consumers),
  `IMPORTED_CXX_MODULES_<CONFIG>`; consumers rebuild BMIs in synthetic targets;
  before 4.4 those use the provider's settings (dialect/extensions mismatches
  fail — tested with 3.28.6 and 4.3.4), 4.4+ takes `COMPILE_FEATURES` /
  `COMPILE_OPTIONS` from the consumer (tested with 4.4.5, 4.5.0-rc2); 4.5
  `install_prep/*` targets.
- `import std`: 3.30 experimental (Clang/libc++, MSVC 14.36+); 4.1 adds GCC 15+
  and Clang+libstdc++; 4.2 adds `CMAKE_CXX_STDLIB_MODULES_JSON`; 4.3.0-rc1
  shipped ungated and 4.3.0 restored the gate; gate UUIDs for 3.30, 4.3, 4.4
  (`f35a9ac6-…`); 4.5.0-rc2 ungated for all scanned C++20+ sources, removed
  `CXX_MODULE_STD` / `CMAKE_CXX_MODULE_STD` / `CMAKE_CXX_COMPILER_IMPORT_STD`,
  GCC 16+ on macOS (GCC 15 + 4.5-rc2 on macOS fails to link "initializer for
  module std" — tested); Ninja only; Ubuntu < 26.04 broken manifest; Homebrew
  LLVM 22 needs `CMAKE_CXX_STDLIB_MODULES_JSON`.
- Build database (3.31, still gated in 4.5-rc2), CMP0154, CMP0211, 4.4
  file-set properties, 4.5 `FILE_SET_TYPES` and AUTOMOC (Qt 6.13+); the
  compiler command templates in `Modules/Compiler/*-CXX.cmake`; ccache doesn't
  support C++20 modules.
- Google and Chromium still ban modules; the pilot checklist.

## Still open (answer these; the rest is settled)

- **sccache and distributed builds** with CMake's module rules (GCC, Clang,
  MSVC): what's cached, what bypasses, minimum versions — from sccache's own
  docs and release notes.
- **Editor and analysis tools**: clangd (`--experimental-modules-support`,
  current limitations), clang-tidy, CLion, VS Code, Visual Studio — what works
  with a CMake/Ninja module build as of the date, with versions.
- **CPS packages with modules**: does `install(PACKAGE_INFO … CXX_MODULES_DIRECTORY)`
  produce a package a CMake consumer can import modules from, since which
  version, and is there a worked example?
- **Patch-level fixes** in 4.4.1–4.4.5 and 4.5.0-rc1/rc2 that affect modules
  (primary sources: release announcements, merge requests).
- **4.5.0 final**: anything that changed from rc2 (std availability rules,
  removed variables, the CXX_SCAN_FOR_MODULES requirement for `std`).
- **MSVC** with CMake: the `.ixx` convention, `import std` behaviour in VS vs
  Ninja generators, VS 2026 status — not verifiable locally (no MSVC).
- **Header units**: the status of CMake issue 25293 and any shipped support.
- **Real-world adoption** with numbers from primary sources (Kitware posts,
  project mailing lists): build-time effects, migration reports.

## Questions to answer

1. **Version history.** Every CMake release from 3.25 to the latest (stable
   and RC, as of {{DATE}}) that changed C++ modules support, with what changed:
   the experimental period (`CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API`,
   `CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP`), 3.28 stabilization, and each later
   addition. A table: version → feature → doc link.
2. **Every CMake knob for modules.** All variables, target and source
   properties, policies and generator expressions involved, with defaults and
   the version that added each: `CXX_SCAN_FOR_MODULES` /
   `CMAKE_CXX_SCAN_FOR_MODULES`, `CXX_MODULE_STD`, `CXX_MODULE_SET*`,
   `CXX_MODULE_DIRS*`, `CXX_MODULE_BMI_*`, `CMAKE_CXX_MODULE_MAP_FORMAT`,
   `CMAKE_CXX_COMPILER_IMPORT_STD`, the policies (CMP0155 and any later ones),
   anything for `compile_commands.json` and module maps. Correct my list.
3. **Installing and exporting module libraries.** `install(TARGETS …
   FILE_SET CXX_MODULES DESTINATION …)`, `CXX_MODULES_BMI` install, `install(EXPORT)`
   and what the generated config files contain for modules; whether consumers
   rebuild BMIs (and why BMIs aren't portable between compilers/flags); CPS
   (Common Package Specification) and modules in CMake 4.x; `IMPORTED` targets
   with module file sets.
4. **Per compiler, exactly how CMake drives it.** For GCC (`-fmodules`,
   `-fdeps-format=p1689r5`, `.gcm`/`gcm.cache`), Clang (`clang-scan-deps`,
   `-fmodule-output`, `.pcm`, libc++ `std.cppm`), and MSVC (`/scanDependencies`,
   `/ifcOutput`, `.ifc`): the scanning step, the collation step, the
   P1689 dependency format, and the minimum versions. What breaks with
   `ccache`/`sccache` and distributed builds, and current workarounds.
5. **Generators.** Ninja (version floor, dyndep), Visual Studio, Ninja
   Multi-Config, Xcode (supported or not, as of {{DATE}}), Makefiles (status as
   of {{DATE}}). Which generators support `import std`.
6. **Header units and `import std.compat`.** Current CMake status of header
   units (any experimental support or plans), `import std.compat` support, and
   mixing `#include` with `import std` in one target.
7. **Tooling around CMake modules.** clangd / `compile_commands.json` with
   modules, clang-tidy, IDE support (CLion, VS, VS Code), and CMake's own
   guidance on using modules in a library that must also build without them.
8. **A minimal, current, correct example** — a library with an interface
   unit, an implementation unit and a partition, an executable that imports it
   and uses `import std;`, plus install/export and a separate consumer project
   — for CMake 3.28, 4.4 and 4.5. Mark which lines differ by version.
9. **Anything I'm missing** — known bugs, recent changes in GCC 15/16, Clang
   21/22, MSVC 17.14/18.x that affect CMake module builds, and real-world
   adoption reports from large projects (with sources).

## Output format

- Headings: **Version history · Knobs reference · Install and export ·
  Per-compiler details · Generators · Header units and std.compat · Tooling ·
  Worked example · Known issues · Sources**.
- Each recommendation: **short imperative** → reason → exact CMake
  command/variable/property → minimum CMake and compiler versions → source.
- Mark corrections to my list explicitly ("**Correction:** …").
- Cite **primary sources only**: the CMake manuals (`cmake-cxxmodules(7)`,
  `cmake-properties(7)`, `cmake-variables(7)`, `cmake-policies(7)`), CMake
  release notes, Kitware blog posts, CMake GitLab issues and merge requests,
  CMake Discourse announcements, GCC/Clang/MSVC documentation, WG21/SG15
  papers (P1689 and successors). No unsourced blog summaries.
- Give a **confidence level** and **source date** for every version or support
  claim; distinguish **stable release** from **release candidate**.
- Where sources disagree, show both and their reasons.
- **Don't invent** variables, properties, policies, flags or versions. If
  something can't be verified, say so.
- End with a **candidate sources table**: name, URL, best for, update
  frequency.
