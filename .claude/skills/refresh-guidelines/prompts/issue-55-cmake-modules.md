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

## What I already cover (verify, correct, or add nuance — don't re-explain)

- CMake scans sources for named modules since **3.28**; module sources go in a
  `FILE_SET CXX_MODULES` (`target_sources(geo PUBLIC FILE_SET CXX_MODULES FILES src/geo.cppm)`).
- Toolchain floor for scanning, per the CMake manual: Ninja 1.11+ or Visual
  Studio 2022 17.4+ generators; MSVC toolset 14.34+, Clang 16+, GCC 14+.
  Header units aren't supported.
- `import std;` via `CXX_MODULE_STD` / `CMAKE_CXX_MODULE_STD`: experimental
  through CMake 4.4 (gated by `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` with a
  per-version UUID); CMake 4.5 (release candidate as of late Oct 2026) lists
  it as "Added in version 4.5" with no gate. GCC 15+ (16+ on macOS) for
  libstdc++. 4.5 adds `CMAKE_CXX_STDLIB_MODULES_JSON`; Ubuntu releases before
  26.04 ship a broken `libstdc++.modules.json` (Launchpad bug 2141579).
- Google and Chromium still ban modules; I recommend piloting one target
  end-to-end (clean and incremental builds, ccache, debugging, clangd,
  clang-tidy, install/export, a consumer built against the installed package).

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
