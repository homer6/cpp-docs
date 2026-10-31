# Build and packaging — CMake, dependencies, modules, compile times

> How to build a modern C++ project so every configuration in
> [safety](safety.md#build-configurations) is one command away, dependencies are
> pinned, and builds stay fast. **Sources:** CMake documentation (4.3.4 bundled
> help; versioned 3.30 and 4.4 manuals; 4.5.0-rc2 online manual) · **Status:**
> draft — see [sources](sources.md) for what's still being verified

**Which CMake is current (checked 2026-10-31):** the latest stable release is
**4.4.5** (announced 2026-10-29); **4.5.0-rc2** (2026-10-28) is a release
candidate. `cmake.org/cmake/help/latest/` currently shows the 4.5.0-rc2 manual,
so "latest" there doesn't mean "latest stable". Read the version banner, or use
a versioned URL (`/help/v4.4/`).

## 1. Target-based CMake

Describe **targets** and their requirements; never set global flags in
`CMakeLists.txt`. Requirements attached to a target with `PUBLIC` or
`INTERFACE` propagate to everything that links it; `PRIVATE` ones don't.

```cmake
cmake_minimum_required(VERSION 3.28)
project(geo VERSION 1.0 LANGUAGES CXX)

# Warnings live on an interface target so every target opts in the same way.
add_library(geo_warnings INTERFACE)
target_compile_options(geo_warnings INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->)

add_library(geo src/distance.cpp)
add_library(geo::geo ALIAS geo)
target_compile_features(geo PUBLIC cxx_std_20)      # consumers need C++20 too
target_include_directories(geo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(geo PRIVATE geo_warnings)     # warnings don't leak to users
set_target_properties(geo PROPERTIES COMPILE_WARNING_AS_ERROR ON)

if(PROJECT_IS_TOP_LEVEL)                            # not when used as a subproject
    enable_testing()
    add_executable(geo_tests tests/distance_test.cpp)
    target_link_libraries(geo_tests PRIVATE geo::geo geo_warnings)
    add_test(NAME geo_tests COMMAND geo_tests)
endif()
```

- **Set the standard per target** with `target_compile_features(... cxx_std_NN)`,
  not `CMAKE_CXX_STANDARD` in a library — a consumer may need a newer standard.
  Make it `PUBLIC` only if your *headers* need it.
- **Keep warnings `PRIVATE`** (or on an `INTERFACE` target you link privately),
  so you don't impose your warning level on users.
- **`COMPILE_WARNING_AS_ERROR`** (CMake 3.24) beats hard-coding `-Werror`:
  anyone can turn it off with `cmake --compile-no-warning-as-error` when a new
  compiler adds warnings.
- **Always provide a `namespace::name` alias**, so `target_link_libraries`
  fails loudly on a typo instead of treating it as a linker flag.
- **Export `compile_commands.json`** (`CMAKE_EXPORT_COMPILE_COMMANDS`) for
  clangd and clang-tidy.

## 2. Presets for every build configuration

`CMakePresets.json` (committed) gives each configuration from
[safety](safety.md#build-configurations) a name, so local builds and CI run
the same thing. Per-developer tweaks go in an uncommitted
`CMakeUserPresets.json`.

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": { "CMAKE_EXPORT_COMPILE_COMMANDS": "ON" }
    },
    {
      "name": "debug",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_CXX_FLAGS": "-D_GLIBCXX_ASSERTIONS -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG"
      }
    },
    {
      "name": "release-hardened",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        "CMAKE_CXX_FLAGS": "-D_GLIBCXX_ASSERTIONS -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST -ftrivial-auto-var-init=zero -fstack-protector-strong"
      }
    },
    {
      "name": "asan-ubsan",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_CXX_FLAGS": "-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer",
        "CMAKE_EXE_LINKER_FLAGS": "-fsanitize=address,undefined"
      }
    },
    {
      "name": "tsan",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_CXX_FLAGS": "-fsanitize=thread -fno-omit-frame-pointer",
        "CMAKE_EXE_LINKER_FLAGS": "-fsanitize=thread"
      }
    }
  ],
  "buildPresets": [
    { "name": "debug", "configurePreset": "debug" },
    { "name": "release-hardened", "configurePreset": "release-hardened" },
    { "name": "asan-ubsan", "configurePreset": "asan-ubsan" },
    { "name": "tsan", "configurePreset": "tsan" }
  ],
  "testPresets": [
    { "name": "asan-ubsan", "configurePreset": "asan-ubsan",
      "output": { "outputOnFailure": true } }
  ],
  "workflowPresets": [
    {
      "name": "ci-asan",
      "steps": [
        { "type": "configure", "name": "asan-ubsan" },
        { "type": "build", "name": "asan-ubsan" },
        { "type": "test", "name": "asan-ubsan" }
      ]
    }
  ]
}
```

```bash
cmake --preset release-hardened && cmake --build --preset release-hardened
cmake --workflow --preset ci-asan      # configure + build + test in one step
```

Notes:

- Schema **version 6** needs CMake 3.25 and is the first with workflow
  presets. Each newer schema version raises the minimum CMake: 7 → 3.27,
  8 → 3.28, 9 → 3.30, 10 → 3.31, 11 → 4.3. Use the lowest version that has the
  features you use, and test the presets with that minimum CMake, not just the
  newest one installed.
- Keep machine-specific paths and personal choices out of the committed file;
  they belong in `CMakeUserPresets.json`.
- Both hardening macros are defined; each standard library ignores the other's.
  The libc++ modes are `NONE`, `FAST`, `EXTENSIVE`, and `DEBUG`.
- These presets are GCC/Clang flags. Give MSVC its own presets (see the
  [flag table](#6-flag-equivalents)), using a `condition` on the host system or
  a separate hidden base.
- Setting `CMAKE_CXX_FLAGS` in a preset replaces, not extends, the cache value —
  keep presets as the single place those flags are set.

Tested: `debug`, `release-hardened`, `tsan`, and the `ci-asan` workflow build
and pass with CMake 4.3.4, Ninja, and Apple Clang 15 on macOS.

## 3. Modules

CMake scans sources for named modules since **3.28**. Module sources are
listed in a `CXX_MODULES` file set:

```cmake
add_library(geo)
target_sources(geo
    PUBLIC FILE_SET CXX_MODULES FILES src/geo.cppm)
target_compile_features(geo PUBLIC cxx_std_20)
```

**Toolchain floor for module scanning** (from the CMake manual): Ninja 1.11+ or
Visual Studio 2022 17.4+ generators; MSVC toolset 14.34+, Clang 16+, GCC 14+.
Header units aren't supported.

### `import std;`

```cmake
add_executable(app src/main.cpp)
target_compile_features(app PRIVATE cxx_std_23)
set_target_properties(app PROPERTIES CXX_MODULE_STD ON)
```

`import std` support is separate from named-module support: it has its own
section and toolchain list in the CMake manual, and it's still experimental in
every stable release.

| CMake | Status | Toolchains the manual lists |
|---|---|---|
| 3.30 | Experimental (gate required) | Clang 18.1.2+ with `-stdlib=libc++`; MSVC toolset 14.36+ |
| 4.4 (current stable) | Experimental (gate required) | Clang 18.1.2+ with libc++ or libstdc++; MSVC 14.36+ (VS 17.6+); GCC 15+ |
| 4.5.0-rc2 (release candidate) | "Added in version 4.5": no gate, for all C++20-and-later code with module scanning on | as 4.4, but GCC 15+ except on macOS, GCC 16+ on macOS |

- **Up to and including 4.4:** set `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` to the
  gate UUID for *your exact CMake version* (listed in CMake's
  `Help/dev/experimental.rst`) before `project()`. The UUID changes between
  releases — don't copy one from a blog post.
- **4.5:** the release candidate's manual and release notes drop the gate.
  Treat that as the plan until 4.5.0 final ships, and re-check then.
- **Ninja generators only** — Visual Studio generators can't build BMIs for
  `IMPORTED` targets, which the `std` module is.
- **Ubuntu releases before 26.04** ship a broken `libstdc++.modules.json`
  ([Ubuntu bug 2141579](https://bugs.launchpad.net/ubuntu/+source/gcc-15/+bug/2141579)).
  4.5 adds `CMAKE_CXX_STDLIB_MODULES_JSON` to point CMake at a fixed copy.

### Pilot before adopting

Google and Chromium still ban modules (see
[industry practice](industry-practice.md)). Before using them across a
codebase, prove the whole pipeline on one target: clean and incremental
builds (after editing a module interface *and* an implementation unit),
`ccache`, debugging, clangd/IDE indexing, clang-tidy, installing and exporting
the module library, and a separate consumer built against the *installed*
package. Do this for each compiler/standard library pair you support, and
record the exact compiler, standard library, CMake and Ninja versions that
worked in your [feature matrix](project-conventions.md#feature-matrix).

## 4. Dependencies

| Approach | Good for | Watch out for |
|---|---|---|
| **System packages** + `find_package` | Distro-packaged software; fast builds | Versions vary by distro; hard to pin |
| **`FetchContent`** | A few small dependencies, no extra tooling | Builds every dependency from source in every build tree; dependency's CMake options and warnings mix with yours |
| **vcpkg** (manifest mode: `vcpkg.json` + `builtin-baseline`) | Many dependencies, pinned versions, binary caching | Another tool to install; ports may lag upstream |
| **Conan 2** (`CMakeDeps` + `CMakeToolchain`) | Many dependencies, custom binary repositories, lockfiles | Steeper learning curve; Python tooling |

- **Prefer `find_package` in your `CMakeLists.txt` regardless of where packages
  come from.** vcpkg and Conan both feed `find_package` through a toolchain
  file, so your build code doesn't depend on the package manager.
- **`FetchContent_Declare(... FIND_PACKAGE_ARGS)`** (CMake 3.24) tries
  `find_package` first and only downloads if the package isn't found — users
  with the dependency installed don't rebuild it.
- **Dependency providers** (`CMAKE_PROJECT_TOP_LEVEL_INCLUDES`, 3.24) let a
  package manager intercept `find_package`/`FetchContent` without changing your
  project.
- **Pin everything:** a vcpkg baseline, a Conan lockfile, or `FetchContent`
  with a full commit hash (never a branch name).

## 5. Compile times

Profile the build before choosing a fix. No one ranking holds across
codebases, so measure clean builds, incremental builds after a header edit,
link time, and CI with a cold cache, then try the technique that targets the
slowest one.

| Technique | How | Notes |
|---|---|---|
| Compiler cache | `CMAKE_CXX_COMPILER_LAUNCHER=ccache` (or `sccache`) | Biggest win for CI and branch switching |
| Faster linker | `CMAKE_LINKER_TYPE=LLD` or `MOLD` (CMake 3.29) | Link time dominates incremental builds of large binaries |
| Precompiled headers | `target_precompile_headers` (3.16) | Put stable, heavy headers (`<vector>`, `<string>`, third-party) in it, not your own changing headers |
| Unity builds | `CMAKE_UNITY_BUILD=ON` (3.16) | Fewer, bigger translation units; can expose name clashes between files in anonymous namespaces |
| Find the slow parts | Clang `-ftime-trace` (+ ClangBuildAnalyzer), GCC `-ftime-report` | Measure before restructuring code |
| Time the whole build | [`cmake-instrumentation(7)`](https://cmake.org/cmake/help/latest/manual/cmake-instrumentation.7.html) (4.3) | Timing for configure, build, test and install steps. In 4.3.4 the index/snippet `version` field changed from an integer to `{ "major": 1, "minor": 0 }`, so tools that read it must handle both forms |
| Include hygiene | include-what-you-use; forward declarations; keep heavy headers out of public headers | Pays off forever |
| Split debug info | `-gsplit-dwarf` | Smaller objects, faster links |
| Modules | see [Modules](#3-modules) | No general build-time figure yet; measure your own supported configurations |

## 6. Flag equivalents

| Purpose | GCC / Clang | MSVC |
|---|---|---|
| Language standard | `-std=c++23` | `/std:c++latest` (or `/std:c++20`) — prefer `cxx_std_NN` in CMake |
| Report the real `__cplusplus` | — | `/Zc:__cplusplus` |
| Standards conformance | `-pedantic` | `/permissive-` |
| Warnings | `-Wall -Wextra -Wpedantic` | `/W4` |
| Warnings as errors | `-Werror` | `/WX` — prefer `COMPILE_WARNING_AS_ERROR` |
| Debug info | `-g` | `/Zi` (or `/Z7`) |
| Optimization | `-O2` | `/O2` |
| Standard C++ exceptions | (default) | `/EHsc` |
| AddressSanitizer | `-fsanitize=address` | `/fsanitize=address` |
| Other sanitizers | `-fsanitize=undefined`, `-fsanitize=thread` | MSVC's `/fsanitize` documents only `address` (VS 2019 16.9), `kernel-address` (VS 2022 17.11) and experimental `fuzzer` (VS 2022 17.0) — no UB or thread sanitizer |
| Stack protection | `-fstack-protector-strong` | `/GS` (on by default) |
| Library hardening | libc++ `_LIBCPP_HARDENING_MODE`, libstdc++ `_GLIBCXX_ASSERTIONS` | `_MSVC_STL_HARDENING=1` project-wide (VS 2022 17.14; off by default, implements P3471) |
| Zero-init locals | `-ftrivial-auto-var-init=zero` | see "To verify" |

Rows that look alike aren't always identical in behavior (e.g. `-pedantic`
vs `/permissive-`); check the compiler docs before relying on one.

## 7. Packaging

**Install with a target export, and test a consumer against the installed
package**, not the build tree. CMake 4.3 added the
[Common Package Specification](https://cps-org.github.io/cps/) (CPS):
`install(PACKAGE_INFO)` / `export(PACKAGE_INFO)` write a `.cps` file, and
`find_package` finds and imports `.cps` packages. Keep the CMake-script config
file as well while consumers still use CMake < 4.3:

```cmake
cmake_minimum_required(VERSION 4.3)
project(geo VERSION 1.2.0 COMPAT_VERSION 1.0.0 SPDX_LICENSE "MIT" LANGUAGES CXX)

add_library(geo src/distance.cpp)
add_library(geo::geo ALIAS geo)
target_compile_features(geo PUBLIC cxx_std_20)
target_sources(geo PUBLIC FILE_SET HEADERS BASE_DIRS include
               FILES include/geo/distance.hpp)

include(GNUInstallDirs)
install(TARGETS geo EXPORT geo-targets FILE_SET HEADERS)
install(EXPORT geo-targets NAMESPACE geo:: FILE geo-config.cmake  # CMake < 4.3
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/geo)
install(PACKAGE_INFO geo EXPORT geo-targets)                      # geo.cps, 4.3+
```

- `project(... COMPAT_VERSION ... SPDX_LICENSE ...)` (4.3) supplies the
  `.cps` file's compatibility version and licence.
- Tested with CMake 4.3.4 + Ninja: the install writes
  `lib/cps/geo/geo.cps` and `lib/cmake/geo/geo-config.cmake`. A consumer doing
  `find_package(geo 1.0 REQUIRED)` + `target_link_libraries(app PRIVATE
  geo::geo)` built and ran against a copy of the install at a different prefix
  with the CMake-script files deleted, so it found the `.cps` file.
- CMake supporting CPS doesn't mean every consumer does. Ship the formats your
  consumers need, and test each one.

## See also

- [CMake C++ modules manual](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html) ·
  [CMake presets manual](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) ·
  [CMake 4.3 release notes](https://cmake.org/cmake/help/latest/release/4.3.html) ·
  [CMake announcements](https://discourse.cmake.org/c/announcements/8) ·
  [MSVC STL hardening](https://github.com/microsoft/STL/wiki/STL-Hardening)
- [Memory & safety — build configurations](safety.md#build-configurations) ·
  [Project conventions](project-conventions.md) ·
  [modules (reference)](../examples/language/modules/modules.md)
