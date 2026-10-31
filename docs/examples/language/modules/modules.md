# Modules

> A compiled, encapsulated alternative to `#include`: a module is compiled once
> into a binary interface (BMI), and importers see only what it `export`s.
> **Since:** C++20 (`import std;` since C++23)

## Mental model

A named module is a set of *module units* sharing one name. Its interface is
compiled into a BMI (`.gcm` GCC, `.pcm` Clang, `.ifc` MSVC) that `import` reads;
importers get only exported declarations, never macros. Providers compile
before importers ([Building modules](building.md)).

| Unit | Starts with | Provides a BMI |
|---|---|---|
| Primary interface unit | `export module M;` | yes (one per module) |
| Implementation unit | `module M;` (implicitly imports `M`) | no |
| Interface partition | `export module M:part;` | yes; must be re-exported by `M` |
| Internal partition | `module M:part;` | yes; importable only inside `M` |

## Examples

### Interface and implementation units

```cpp module=math
// math.cppm
export module math;
export int add(int a, int b) { return a + b; }   // exported
export int add_twice(int a, int b);              // defined in math.cpp
int twice(int x) { return x * 2; }               // not exported: module linkage
```

```cpp module=math
// math.cpp — implementation unit; sees everything in math, exported or not
module math;
int add_twice(int a, int b) { return twice(a) + twice(b); }
```

```cpp module=math
// main.cpp
import math;
int main() { return add(2, 3) + add_twice(1, 1) - 9; }   // 5 + 4 - 9
```

```cpp module=math compile-fail="twice"
import math;
int main() { return twice(2); }          // error: twice isn't exported
```

### Export forms

```cpp module=shapes
module;
#include <cmath>                         // global module fragment (below)
export module shapes;

export struct Point { double x, y; };                                  // a declaration
export { double area(double r) { return 3.14159 * r * r; } }           // a block
export namespace geo { template <class T> T square(T v) { return v * v; } }  // a namespace
export using std::sqrt;                  // a using-declaration (external-linkage names only)
```

```cpp module=shapes
import shapes;
int main() { return geo::square(3) == 9 && sqrt(16.0) == 4 && area(1) > 3 ? 0 : 1; }
```

`export import other;` re-exports a whole module. Only named, external-linkage
declarations at namespace scope can be exported, and only from an interface
unit:

```cpp module compile-fail="internal linkage"
export module bad;
export static int counter = 0;           // error: internal linkage can't be exported
```

### Partitions

Partitions split one module across files. The primary interface re-exports the
interface partitions; internal partitions hold shared private helpers.
Partitions import each other as `import :name;`, and nothing outside the module
can import a partition.

```cpp module=geo
export module geo:types;                 // interface partition
export struct point { int x, y; };
```

```cpp module=geo
module geo:detail;                       // internal partition
int abs_value(int v) { return v < 0 ? -v : v; }
```

```cpp module=geo
export module geo;
export import :types;                    // importers of geo see point
import :detail;
export int manhattan(point p) { return abs_value(p.x) + abs_value(p.y); }
```

```cpp module=geo
import geo;
int main() { return manhattan({3, -4}) == 7 ? 0 : 1; }
```

### Module linkage, visibility and reachability

A non-exported namespace-scope name has **module linkage**: every unit of the
module can use it, nothing outside can, and it doesn't clash with the same name
elsewhere (GCC and Clang put the module name in the symbol). Name lookup finds
only exported names (*visibility*), but every declaration in an imported
interface is *reachable*: an importer can use a non-exported type it gets
without naming it.

```cpp module=token
export module token;
struct Token { int id; };                // not exported
export Token make_token(int id) { return {id}; }
```

```cpp module=token
import token;
int main() { auto t = make_token(21); return t.id == 21 ? 0 : 1; }   // OK: reachable, never named
```

```cpp module=token compile-fail="Token"
import token;
Token t{1};                              // error: Token isn't visible
```

### The global and private module fragments

A module unit that needs headers starts with `module;` and only preprocessor
directives; those declarations belong to the global module, and its macros stay
in the file. The primary interface can end with `module :private;`: what
follows can't affect importers, so a small module fits in one file.

```cpp module=config
module;
#include <cstddef>
#define BUFFER 64                        // never reaches importers
export module config;
export constexpr std::size_t buffer_size = BUFFER;
```

```cpp module=config
import config;
#ifdef BUFFER
#error "macros don't cross a named-module import"
#endif
int main() { return buffer_size == 64 ? 0 : 1; }
```

```cpp module needs=private-module-fragment
export module tally;
export int next();
module :private;                         // invisible to importers
int value = 0;
int next() { return ++value; }
```

### Header units

`import <header>;` compiles a header as its own unit. Unlike a named module, a
header unit exports its macros.

```cpp module
import <version>;
static_assert(__cpp_lib_ranges > 0);     // the header unit's macro came through
```

## Gotchas

- **Macros don't cross a named-module import** in either direction; keep the
  `#include` for anything that's a macro (`assert`, `INT_MAX`).
- **Imports come first** in a module unit, before other declarations.
- **`module M:part;` is an internal partition**, not "the implementation of
  `:part`". Implementation units are always `module M;`.
- **BMIs aren't portable** across compilers, versions or language flags. Ship
  module sources.
- **GCC 15.2 and 16.2 don't implement the private module fragment** ("sorry,
  unimplemented"); Clang 22 does. Clang 22 warns that header units are
  experimental, and CMake doesn't support them.

## See also

- [Modules — cppreference](https://en.cppreference.com/w/cpp/language/modules)
- [[module] — the standard draft](https://eel.is/c++draft/module)
- [Building modules](building.md) · [Build and packaging § Modules](../../../guidelines/build-and-packaging.md#3-modules)
