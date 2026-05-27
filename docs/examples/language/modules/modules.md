# Modules

> A compiled, encapsulated alternative to textual `#include` — faster builds,
> real isolation, explicit interfaces. **Since:** C++20

## Why modules

`#include` literally pastes header text into every translation unit: slow
(re-parsed everywhere), fragile (macro leakage, include order), and leaky (private
helpers escape). A **module** is parsed once into a binary interface; importers
get only what's `export`ed, and macros don't leak across the boundary.

## A module interface unit

```cpp
// math.ixx  (or math.cppm — extension is toolchain-specific)
export module math;                  // declares the module `math`

export int add(int a, int b) { return a + b; }   // exported: visible to importers

int helper(int x) { return x * 2; }   // NOT exported: private to the module

export namespace geo {                 // export a whole namespace
    double area(double r);
}
```

## Using it

```cpp
import math;                          // no #include, no header

int main() {
    return add(2, 3);                 // visible
    // helper(...) is NOT accessible — it wasn't exported
}
```

## import std; (C++23)

The standard library itself is available as a module — one import instead of
dozens of headers:

```cpp
import std;                          // the entire standard library (C++23)

int main() {
    std::vector<int> v{1, 2, 3};
    std::println("{}", v.size());
}
```

(`import std.compat;` also brings the global-namespace C names.)

## Module partitions

Large modules can be split into partitions that combine into one logical module:

```cpp
export module math:trig;             // a partition
// in the primary interface:
export module math;
export import :trig;                  // re-export the partition
```

## Gotchas

- **Tooling is the hard part.** Build-system and compiler support for modules
  (especially `import std;`) is still maturing and varies — file extensions, flags,
  and BMI (binary module interface) handling differ across GCC/Clang/MSVC and
  CMake versions. Check what your toolchain supports before committing.
- **Import order doesn't matter and macros don't leak** — a feature vs headers,
  but it also means a module can't *receive* macros from importers (no
  configuration via `#define` before include).
- **You can't mix freely.** Migrating a header-based codebase is incremental; a
  module can `#include` legacy headers in its *global module fragment*
  (`module;` at the top) but exported entities have rules.
- **One primary interface unit per module.** Implementation units and partitions
  support the rest.

## See also

- [Modules — cppreference](https://en.cppreference.com/w/cpp/language/modules)
- [`import std;` — cppreference](https://en.cppreference.com/w/cpp/standard_library#Importing_modules)
