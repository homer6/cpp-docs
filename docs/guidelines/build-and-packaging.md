# Build and packaging — CMake, dependencies, modules, compile times

> How to build a modern C++ project so every configuration in
> [safety](safety.md#build-configurations) is one command away, dependencies are
> pinned, and builds stay fast. **Sources:** CMake documentation (4.3 bundled
> help, 4.5 online manual) · **Status:** first draft — see
> [sources](sources.md) for what's still being verified

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
  presets. Newer schema versions add features but raise the minimum CMake.
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

- **CMake 3.30 – 4.4:** experimental. You must set
  `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` to the gate UUID for *your exact CMake
  version* (listed in CMake's `Help/dev/experimental.rst`) before `project()`.
  The UUID changes between releases — don't copy one from a blog post.
- **CMake 4.5:** documented as supported without the gate, and available for
  C++20 and newer.
- **Ninja generators only** — Visual Studio generators can't build the BMIs
  for the imported `std` target.
- **Toolchains:** Clang 18.1.2+ with libc++ or libstdc++; MSVC 14.36+ (VS
  17.6+); GCC 15+ (GCC 16+ on macOS). Ubuntu releases before 26.04 ship a
  broken `libstdc++.modules.json`.

### Pilot before adopting

Google and Chromium still ban modules (see
[industry practice](industry-practice.md)). Before using them across a
codebase, prove the whole pipeline on one target: clean and incremental
builds, `ccache`, debugging, clangd/IDE indexing, clang-tidy, installing and
exporting the module library, and a downstream consumer. Record the exact tool
versions that worked in your [feature matrix](project-conventions.md#feature-matrix).

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

Roughly in order of payoff for effort:

| Technique | How | Notes |
|---|---|---|
| Compiler cache | `CMAKE_CXX_COMPILER_LAUNCHER=ccache` (or `sccache`) | Biggest win for CI and branch switching |
| Faster linker | `CMAKE_LINKER_TYPE=LLD` or `MOLD` (CMake 3.29) | Link time dominates incremental builds of large binaries |
| Precompiled headers | `target_precompile_headers` (3.16) | Put stable, heavy headers (`<vector>`, `<string>`, third-party) in it, not your own changing headers |
| Unity builds | `CMAKE_UNITY_BUILD=ON` (3.16) | Fewer, bigger translation units; can expose name clashes between files in anonymous namespaces |
| Find the slow parts | Clang `-ftime-trace` (+ ClangBuildAnalyzer), GCC `-ftime-report` | Measure before restructuring code |
| Include hygiene | include-what-you-use; forward declarations; keep heavy headers out of public headers | Pays off forever |
| Split debug info | `-gsplit-dwarf` | Smaller objects, faster links |
| Modules | see [Modules](#3-modules) | Potentially large win; tooling still maturing |

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
| UB / thread sanitizers | `-fsanitize=undefined`, `-fsanitize=thread` | not available in MSVC |
| Stack protection | `-fstack-protector-strong` | `/GS` (on by default) |
| Library hardening | libc++ `_LIBCPP_HARDENING_MODE`, libstdc++ `_GLIBCXX_ASSERTIONS` | see "To verify" in [sources](sources.md) |
| Zero-init locals | `-ftrivial-auto-var-init=zero` | see "To verify" |

## See also

- [CMake C++ modules manual](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html) ·
  [CMake presets manual](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [Memory & safety — build configurations](safety.md#build-configurations) ·
  [Project conventions](project-conventions.md) ·
  [modules (reference)](../examples/language/modules/modules.md)
