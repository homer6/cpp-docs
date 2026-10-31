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

## What I already cover (verified 2026-10-31 — don't re-explain; only report changes, disagreements, or nuance)

- Target-based CMake basics: `target_compile_features(... cxx_std_20)`,
  `target_include_directories` with `BUILD_INTERFACE`/`INSTALL_INTERFACE`,
  `PRIVATE`/`PUBLIC`/`INTERFACE`, a warnings `INTERFACE` target,
  `COMPILE_WARNING_AS_ERROR` (3.24), `PROJECT_IS_TOP_LEVEL`
- Release status as of 2026-10-31: CMake 4.4.5 is the latest stable (2026-10-29);
  4.5.0-rc2 is a release candidate (2026-10-28); `/help/latest/` shows the RC
- `CMakePresets.json` schema versions → minimum CMake: 6 → 3.25 (workflow
  presets), 7 → 3.27, 8 → 3.28, 9 → 3.30, 10 → 3.31, 11 → 4.3
- Module scanning since 3.28 (Ninja 1.11+, VS 2022 17.4+; Clang 16+, GCC 14+,
  MSVC 14.34+). `import std`: experimental behind
  `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` in 3.30 (Clang 18.1.2+ libc++, MSVC
  14.36+) and still in 4.4 (adds Clang+libstdc++, GCC 15+); the 4.5.0-rc2
  manual drops the gate (GCC 16+ on macOS); Ninja only; Ubuntu < 26.04 ships a
  broken `libstdc++.modules.json`; 4.5 adds `CMAKE_CXX_STDLIB_MODULES_JSON`
- CPS: CMake 4.3 `install/export(PACKAGE_INFO)`, `find_package` imports `.cps`,
  `project(COMPAT_VERSION SPDX_LICENSE)`; tested with 4.3.4
- `cmake-instrumentation(7)` (4.3) and its 4.3.4 `version`-field change;
  4.5-rc adds `install(SBOM)` / `CMAKE_INSTALL_SBOM_FORMATS`
- MSVC: `_MSVC_STL_HARDENING=1` (VS 2022 17.14, off by default, P3471);
  `/fsanitize` documents only `address`, `kernel-address`, `fuzzer`
- `FetchContent` with `FIND_PACKAGE_ARGS` (3.24), dependency providers (3.24),
  `CMAKE_CXX_COMPILER_LAUNCHER` (ccache/sccache), `target_precompile_headers`
  and `CMAKE_UNITY_BUILD` (3.16), `CMAKE_LINKER_TYPE` (3.29)

## Still open from the last run (prioritize these)

1. **CMake 4.5 final** — has 4.5.0 shipped? Does it keep `import std` without
   the gate? Is `install(SBOM)` stable or experimental in it?
2. **Modules end to end** — per GCC, Clang/libc++, Clang/libstdc++ and MSVC:
   versions where named modules and `import std` work with CMake + Ninja,
   known bugs, installing/exporting module libraries, clangd/clang-tidy and IDE
   support. Published module vs header vs PCH build-time measurements, with
   their setups.
3. **Dependency management** — vcpkg (baselines, registries, triplets, binary
   caching), Conan 2 (profiles, lockfiles, `CMakeDeps`/`CMakeToolchain`),
   `FetchContent`/providers, system packages: 2025–2026 changes, and *named*
   large projects with their stated reasons.
4. **Reproducible and secure builds** — official guidance for
   `-ffile-prefix-map`, `SOURCE_DATE_EPOCH`, deterministic archives
   (`ar D`), toolchain pinning; SBOM generation for C++ (CMake 4.5, SPDX/
   CycloneDX tools); dependency attestation.
5. **Compile times** — measurements (with setups) for ccache/sccache, PCH,
   unity builds, lld/mold, split DWARF, IWYU, explicit instantiation.
6. **Flag table** — GCC/Clang/MSVC for warnings (`/W4 /permissive-`),
   conformance, debug formats (`/Zi` vs `/Z7`), optimization, `/GS` vs
   `-fstack-protector-strong`, automatic variable init (MSVC equivalent of
   `-ftrivial-auto-var-init`?), `/Zc:__cplusplus`, `/EHsc`; note where similar
   flags aren't equivalent.
7. **Packaging** — current recipes for `install(EXPORT)`,
   `write_basic_package_version_file`, `FILE_SET HEADERS`, relocatable
   packages; which consumers (vcpkg, Conan, Meson, pkg-config, build2) read CPS.

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
