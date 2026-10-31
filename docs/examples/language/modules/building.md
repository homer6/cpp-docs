# Building modules

> Compiling module units with GCC, Clang, MSVC and CMake, and using
> `import std;` / `import std.compat;`. **Since:** C++20 (`std` modules C++23)

## Mental model

A unit that imports `M` needs `M`'s BMI first, so a module build has an order:
partitions, then the primary interface, then implementation units and
importers. By hand you write that order; build systems ask the compiler to
scan each source for imports (P1689 output) and order the compilations from it.

## GCC

`-fmodules` turns modules on (checked with GCC 15.2 and 16.2); BMIs go to
`gcm.cache/` in the current directory, where later compiles find them.

```bash
g++-15 -std=c++26 -fmodules -c geo-types.cppm             # partition geo:types
g++-15 -std=c++26 -fmodules -c geo.cppm                   # export module geo;
g++-15 -std=c++26 -fmodules -c geo.cpp -o geo_impl.o      # module geo;
g++-15 -std=c++26 -fmodules -c main.cpp                   # import geo;
g++-15 geo-types.o geo.o geo_impl.o main.o -o app
```

`import std;` needs the library's `std` module built (and linked) first;
GCC 16 can do it for you with `--compile-std-module`:

```bash
g++-15 -std=c++26 -fmodules -fsearch-include-path -c bits/std.cc          # std
g++-15 -std=c++26 -fmodules -fsearch-include-path -c bits/std.compat.cc   # std.compat
g++ -std=c++26 -fmodules --compile-std-module main.cpp                    # GCC 16
```

## Clang

Named modules are on with `-std=c++20` or later (Clang's `-fmodules` is a
different feature). Interface and partition units use `.cppm`;
`-fmodule-output=` writes the BMI and `-fprebuilt-module-path=` finds
`<module>.pcm` (`geo-types.pcm` for `geo:types`):

```bash
CXX="clang++ -std=c++26 -stdlib=libc++"
$CXX -c geo-types.cppm -fmodule-output=geo-types.pcm
$CXX -c geo.cppm -fprebuilt-module-path=. -fmodule-output=geo.pcm
$CXX -c geo.cpp  -fprebuilt-module-path=. -o geo_impl.o
$CXX -c main.cpp -fprebuilt-module-path=.
```

libc++ ships `std.cppm` and `std.compat.cppm`, listed in
`libc++.modules.json` (Homebrew LLVM 22: `/opt/homebrew/opt/llvm/lib/c++/`):

```bash
V=/opt/homebrew/opt/llvm/share/libc++/v1
$CXX -Wno-reserved-module-identifier --precompile $V/std.cppm -o std.pcm
$CXX -fmodule-file=std=std.pcm -c main.cpp
```

## MSVC

Not checked locally (no MSVC here). CMake scans with `-scanDependencies` and
needs toolset 14.34+; `import std` needs 14.36+ (VS 17.6). See Microsoft's
[Overview of modules in C++](https://learn.microsoft.com/en-us/cpp/cpp/modules-cpp).

## CMake

Named modules since **CMake 3.28**, with Ninja 1.11+ or Visual Studio 2022+
(not Makefiles or Xcode); GCC 14+, Clang 16+, MSVC 14.34+. Units that
*provide* a module (interfaces and partitions) go in a `CXX_MODULES` file set;
implementation units and importers are ordinary sources.

```cmake
cmake_minimum_required(VERSION 3.28)       # CMP0155 NEW: C++20 sources are scanned
project(geo LANGUAGES CXX)
add_library(geo)
target_sources(geo
  PRIVATE src/geo.cpp
  PUBLIC FILE_SET CXX_MODULES BASE_DIRS src FILES src/geo.cppm src/geo-types.cppm)
target_compile_features(geo PUBLIC cxx_std_20)
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE geo)
```

Checked with CMake 3.28.6, 4.3.4, 4.4.5 and 4.5.0-rc2 (GCC 15, Clang 22, Ninja).

## `import std;` and `import std.compat;`

`std` exports the whole library in `std::` (C functions as `std::printf`);
`std.compat` adds the global C names (`::printf`). Neither exports macros. Put
`#include`s before `import std;` — the other order fails on GCC 15.2.

```cpp module
#include <climits>                       // macros still need their header
import std.compat;
int main() { ::printf("%zu %d\n", ::strlen("abc"), INT_MAX > 0); }
```

```cpp module compile-fail="printf"
import std;
int main() { ::printf("hi\n"); }         // error: only std::printf
```

| Toolchain | `import std` |
|---|---|
| Compilers (CMake's list) | GCC 15+ (16+ on macOS with CMake 4.5); Clang 18.1.2+ with libc++ or libstdc++; MSVC 14.36+ |
| CMake 4.4.x (stable) | experimental: set `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` to the release's UUID before `project()`, then `CXX_MODULE_STD ON` |
| CMake 4.5 (rc2 on 2026-10-31) | automatic for every scanned C++20+ source; `CXX_MODULE_STD` removed |

Ninja generators only. Details: [Build and packaging § Modules](../../../guidelines/build-and-packaging.md#3-modules).

## Gotchas

- **Dialect mismatch:** a BMI built as C++20 can't be imported by a C++23
  compile ("language dialect differs" on GCC, "configuration mismatch" on Clang).
- **`gcm.cache/` is per directory** on GCC; run every compile from the same one.
- **Build `std` with the same flags** as the code that imports it.

## See also

- [Modules — cppreference](https://en.cppreference.com/w/cpp/language/modules) ·
  [Importing modules — cppreference](https://en.cppreference.com/w/cpp/standard_library#Importing_modules)
- [GCC: C++ Modules](https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Modules.html) ·
  [Clang: Standard C++ Modules](https://clang.llvm.org/docs/StandardCPlusPlusModules.html) ·
  [cmake-cxxmodules(7)](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html)
- [Modules](modules.md)
