# Deep research prompt — building and packaging modern C++

For issue #6 (`docs/guidelines/build-and-packaging.md`). Paste everything below
the line into Perplexity Deep Research (or `perplexity_research`). Save the
report as `.firecrawl/deep-research-issue-6-<YYYY-MM-DD>.md`, then ask Claude to
run the `refresh-guidelines` skill in **import** mode on it, targeting
`build-and-packaging.md`.

---

I maintain C++ coding guidelines for experienced engineers, targeting C++26 with
C++20/23 as the production baseline. I need a deep, well-sourced research
report on **current best practice for building and packaging C++ projects** —
CMake, dependency management, C++ modules, reproducible builds, and compile
times. Today's date is {{DATE}}.

## What I already cover (don't re-explain; only report changes, disagreements, or nuance)

- Target-based CMake basics: `target_compile_features(... cxx_std_20)`,
  `target_include_directories` with `BUILD_INTERFACE`/`INSTALL_INTERFACE`,
  `PRIVATE`/`PUBLIC`/`INTERFACE`, a warnings `INTERFACE` target,
  `COMPILE_WARNING_AS_ERROR` (3.24), `PROJECT_IS_TOP_LEVEL`
- `CMakePresets.json` schema v6 (CMake 3.25) with configure/build/test/workflow
  presets for debug, release-hardened, ASan+UBSan and TSan builds
- CMake C++ module scanning since 3.28 (Ninja 1.11+, VS 2022 17.4+; Clang 16+,
  GCC 14+, MSVC 14.34+); `import std` experimental from 3.30 behind
  `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD`, documented as supported in 4.5 (Ninja
  only; Clang 18.1.2+, MSVC 14.36+, GCC 15+ / GCC 16+ on macOS)
- `FetchContent` with `FIND_PACKAGE_ARGS` (3.24), dependency providers (3.24),
  `CMAKE_CXX_COMPILER_LAUNCHER` (ccache/sccache), `target_precompile_headers`
  and `CMAKE_UNITY_BUILD` (3.16), `CMAKE_LINKER_TYPE` (3.29)

## Questions to answer

1. **CMake releases** — What is the latest stable CMake release, and what
   changed for C++ projects in the 3.28 → current releases (modules, `import
   std`, presets schema versions, new policies, `CMAKE_LINKER_TYPE`, instrumentation
   / build-time profiling, SBOM or package-information export)? Is `import std`
   out of experimental in a *stable* release yet, and from which version?
2. **Modules in practice** — For each of GCC, Clang/libc++, Clang/libstdc++ and
   MSVC, what are the minimum versions where named modules and `import std;`
   work end to end with CMake + Ninja today? Known bugs, distro problems (e.g.
   Ubuntu's `libstdc++.modules.json`), installing/exporting module libraries,
   IDE and clang-tidy/clangd support. Include any published build-time
   measurements (modules vs headers vs PCH).
3. **Dependency management** — Compare vcpkg (manifest mode, baselines,
   binary caching, registries), Conan 2 (`CMakeDeps`, `CMakeToolchain`,
   lockfiles), CMake `FetchContent`/dependency providers, and system packages:
   strengths, failure modes, and what large projects actually choose and why.
   Note any 2025–2026 changes in each.
4. **Reproducible and secure builds** — Current guidance on reproducible
   builds (`-ffile-prefix-map`, `SOURCE_DATE_EPOCH`, deterministic archives),
   pinning toolchains, SBOM generation for C++, and supply-chain practice for
   C++ dependencies.
5. **Compile times** — Evidence-backed techniques ranked by typical payoff:
   ccache/sccache, PCH, unity builds, modules, faster linkers (lld, mold),
   `-ftime-trace` / ClangBuildAnalyzer / `-ftime-report`, split DWARF, include
   hygiene (include-what-you-use), explicit template instantiation. Cite
   measurements where they exist.
6. **Cross-compiler flags** — An accurate GCC / Clang / MSVC equivalence table
   for: language standard, warnings (incl. `/W4 /permissive-`), warnings as
   errors, debug info, optimization, sanitizers (MSVC ASan; is there MSVC
   UBSan/TSan?), stack protection (`/GS`), standard-library hardening (libc++
   `_LIBCPP_HARDENING_MODE`, libstdc++ `_GLIBCXX_ASSERTIONS`, MSVC STL
   hardening — which VS version, which macro?), automatic variable
   initialization, `__cplusplus` reporting (`/Zc:__cplusplus`), exceptions
   (`/EHsc`).
7. **Packaging and installing libraries** — Current best practice for
   `install(EXPORT)`, `<Pkg>Config.cmake` + version files, `FILE_SET HEADERS`,
   relocatable packages, and `cps` (Common Package Specification) status in
   CMake.

## Output format

- Headings: **CMake · Presets · Modules · Dependencies · Reproducibility &
  supply chain · Compile times · Flag equivalents · Packaging · Sources**.
- Each recommendation: **short imperative rule** → one-line reason → source(s)
  → minimum tool version → (optional) minimal snippet.
- Where sources or projects disagree, show both positions and their reasons.
- Cite primary sources: cmake.org documentation and release notes, Kitware
  blog, gcc.gnu.org / clang.llvm.org / learn.microsoft.com docs, vcpkg and
  Conan docs, compiler release notes. Avoid unsourced blog summaries.
- Mark every version or support claim with a **confidence level** and the
  **date** of its source. Flag conflicts. Don't invent versions or options.
- End with a **candidate sources table**: name, URL, best for, update
  frequency.
