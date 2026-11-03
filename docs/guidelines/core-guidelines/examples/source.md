# SF: Source files — examples

> Our examples for the [SF rules](../source.md): headers declare interfaces,
> source files implement them, and namespaces keep names apart.

The snippet checker compiles each block as one translation unit, so a
multi-file layout is shown with `// --- name.h ---` comments. A block that
pastes a header's text inline stands for that header being `#include`d
there; one that names files that don't exist here is `nocompile`.

## SF.1: Use a `.cpp` suffix for code files and `.h` for interface files if your project doesn't already follow another convention

*No example: a file-naming convention (`.cpp` and `.h` unless the project already uses others, such as `.cc`/`.hpp`); the official rule defers to NL.27.*

## SF.2: A header file must not contain object definitions or non-inline function definitions

A header is included by many source files. A variable or a non-`inline`
function defined in it is then defined once per file, which breaks the
one-definition rule: the link fails with "multiple definition". Declare in
the header, or make the definition `inline`, `constexpr` or a template.

```cpp bad
// --- counter.h, included by a.cpp and b.cpp ---
int hits = 0;                       // BAD: defined in every file that includes it
int next_hit() { return ++hits; }   // BAD: the same; the link fails
```

```cpp
// --- counter.h ---
inline int hits = 0;                       // GOOD: C++17 inline variable, one object
inline int next_hit() { return ++hits; }   // GOOD: inline function
constexpr int max_hits = 1000;             // GOOD: constexpr is implicitly internal and constant
extern int total;                          // GOOD: a declaration; defined in one .cpp
```

The BAD block compiles on its own; the error appears only when two files
that include it are linked.

## SF.3: Use header files for all declarations used in multiple source files

Declare a function once, in a header, and include it wherever it's used and
where it's defined. A declaration copied by hand into each user can drift
from the definition, and the mismatch shows up late, at link time, or not at
all.

```cpp bad
// --- report.cpp ---
void log_event(const char* msg, int level) { /* ... */ }
// --- main.cpp ---
void log_event(const char* msg);   // BAD: a hand-copied, out-of-date declaration
int main() { log_event("start"); } // links against a function that doesn't exist
```

```cpp
// --- log.h: the one declaration ---
void log_event(const char* msg, int level);
// --- main.cpp, after #include "log.h" ---
int main() { log_event("start", 1); }   // GOOD: checked against the real signature
```

The BAD version compiles; the link fails with an undefined reference.

## SF.4: Include header files before other declarations in a file

Put every `#include` at the top. A header included halfway down sees
whatever was declared above it (macros, `using` directives, overloads), so
its meaning can depend on the including file. Includes at the top also show
a file's dependencies at a glance.

```cpp bad
#define max(a, b) ((a) > (b) ? (a) : (b))   // some code, then...
#include <algorithm>                         // BAD: the macro rewrites std::max inside the header
#include <vector>
```

```cpp
#include <algorithm>   // GOOD: includes first
#include <vector>

int biggest(const std::vector<int>& v) { return std::ranges::max(v); }
```

The BAD block is tagged `bad`: it compiles here only because the snippet
prelude has already included `<algorithm>`. In a real file the mistake shows
up as an error deep inside the standard header.

## SF.5: A `.cpp` file must include the header file(s) that defines its interface

`widget.cpp` should include `widget.h` first. The compiler then checks every
definition against its declaration, so a wrong return type is a compile
error in `widget.cpp`, not a link error (or a crash) somewhere else.

```cpp bad
// --- area.h ---
//   double area(double w, double h);
// --- area.cpp, which doesn't include area.h ---
float area(double w, double h) { return static_cast<float>(w * h); }   // BAD: the mismatch goes unnoticed here
```

When `area.cpp` includes its header, the same mistake doesn't compile:

```cpp compile-fail="ambiguating new declaration|differ only in their return type"
// --- area.h ---
double area(double w, double h);
// --- area.cpp, after #include "area.h" ---
float area(double w, double h) { return static_cast<float>(w * h); }   // error: caught at once
```

```cpp
// --- area.h ---
double area(double w, double h);
// --- area.cpp, after #include "area.h" ---
double area(double w, double h) { return w * h; }   // GOOD: matches, and the compiler checked it
```

## SF.6: Use `using namespace` directives for transition, for foundation libraries (such as `std`), or within a local scope (only)

`using namespace` brings every name of a namespace into scope, and one of
them may collide with yours. Keep it to a function or block, to a namespace
of literals, or (in a `.cpp` file) to a foundation library you use
everywhere. A `using` declaration for one name is narrower still.

```cpp compile-fail="reference to 'count' is ambiguous"
#include <algorithm>
using namespace std;   // BAD here: pulls in std::count too
int count = 0;
int next_id() { return ++count; }   // error: ::count or std::count?
```

```cpp
#include <chrono>
int count = 0;
int next_id() { return ++count; }   // no clash
auto timeout() {
    using namespace std::chrono_literals;   // GOOD: local, and only literals
    return 250ms;
}
```

## SF.7: Don't write `using namespace` at global scope in a header file

A `using namespace` in a header applies to every file that includes it,
after the include, and the includer can't undo it. Names that were fine in
the includer's own code become ambiguous. (`using namespace
std::literals;` is the usual exception: its names can't clash with yours.)

```cpp compile-fail="reference to 'size' is ambiguous"
// --- util.h ---
#include <iterator>
using namespace std;   // BAD: leaks into everything that includes util.h
// --- shop.cpp, after #include "util.h" ---
int size = 3;
int cart_size() { return size; }   // error: ::size or std::size?
```

```cpp
// --- util.h ---
#include <iterator>
namespace util { using std::size; }   // GOOD: nothing leaks into the global scope
// --- shop.cpp, after #include "util.h" ---
int size = 3;
int cart_size() { return size; }
```

## SF.8: Use `#include` guards for all header files

A header that gets included twice in one file, often through two other
headers, redefines its classes and fails to compile. An include guard makes
the second inclusion empty. Name the guard after the project and the file so
it doesn't collide with another library's. (`#pragma once` does the same
and is widely supported, but isn't standard.)

```cpp compile-fail="redefinition of"
// --- geo/point.h, reached twice (through two other headers) ---
struct Point { double x, y; };
// --- the same header again ---
struct Point { double x, y; };   // BAD: no guard: redefinition
```

```cpp
// --- geo/point.h ---
#ifndef GEO_POINT_H
#define GEO_POINT_H
struct Point { double x, y; };
#endif // GEO_POINT_H
// --- the same header again: now empty ---
#ifndef GEO_POINT_H
#define GEO_POINT_H
struct Point { double x, y; };
#endif // GEO_POINT_H
```

## SF.9: Avoid cyclic dependencies among source files

When `a.h` includes `b.h` and `b.h` includes `a.h`, neither can be
understood or compiled on its own, and include guards only hide the cycle
(one of them sees the other half-defined). Break it by depending on a
declaration instead of a definition, or by moving the shared part into a
third header.

```cpp nocompile
// --- order.h ---
#include "customer.h"   // BAD: order.h needs customer.h...
struct Order { Customer* buyer; };
// --- customer.h ---
#include "order.h"      // ...and customer.h needs order.h: a cycle
#include <vector>
struct Customer { std::vector<Order> orders; };
```

```cpp
// --- order.h: no include of customer.h ---
struct Customer;                        // GOOD: a declaration is enough for a pointer
struct Order { Customer* buyer; };
// --- customer.h ---
#include <vector>
// #include "order.h": a one-way dependency
struct Customer { std::vector<Order> orders; };
```

## SF.10: Avoid dependencies on implicitly `#include`d names

Include the header that declares each name you use, even if another header
already happens to bring it in. What a standard header includes varies
between libraries and versions, so code that relies on it breaks when you
change compiler.

```cpp bad
#include <iostream>   // BAD: std::string and std::getline happen to work through
void echo() {         // <iostream> on some libraries, but nothing guarantees it
    std::string line;
    std::getline(std::cin, line);
    std::cout << line << '\n';
}
```

```cpp
#include <iostream>
#include <string>     // GOOD: std::string and std::getline come from <string>
void echo() {
    std::string line;
    std::getline(std::cin, line);
    std::cout << line << '\n';
}
```

The BAD block is tagged `bad`: it compiles here because the snippet prelude
includes `<string>`. On its own, whether it compiles depends on which headers
the standard library happens to pull in.

## SF.11: Header files should be self-contained

A header should compile when it's the first (or only) thing a file includes:
it includes what it uses itself. Otherwise every user has to know its
dependencies and include them first, in the right order.

```cpp bad
// --- names.h ---
// BAD: uses std::string and std::vector but includes neither;
// it only compiles if the includer happened to include them first
std::vector<std::string> split(const std::string& text, char sep);
```

```cpp
// --- names.h ---
#include <string>   // GOOD: includes its own dependencies
#include <vector>
std::vector<std::string> split(const std::string& text, char sep);
```

A simple check: compile a `.cpp` file that includes only the header. The
BAD block is tagged `bad`: it compiles here only because the snippet prelude
supplies the missing headers.

## SF.12: Prefer the quoted form of `#include` for files relative to the including file and the angle bracket form everywhere else

Use `"..."` for a file found relative to the including file (your own
component) and `<...>` for everything found through the include path: the
standard library, other libraries, and your project's other components. The
two forms search differently, so the wrong one can find the wrong file.

```cpp nocompile
#include "vector"               // BAD: searches next to this file first; a local "vector" would win
#include <parser_internal.h>    // BAD: a file next to this one, found only if its folder is on the path
```

```cpp nocompile
#include <vector>               // GOOD: standard library
#include <json/reader.h>        // GOOD: another library, through the include path
#include "parser_internal.h"    // GOOD: next to this file, in the same component
```

Both blocks name project headers that don't exist here, so they're
`nocompile`.

## SF.13: Use portable header identifiers in `#include` statements

Write header names exactly as the file is spelled, case included, and
separate directories with `/`. A wrong case works on a case-insensitive file
system (Windows, macOS by default) and fails on Linux; a backslash isn't a
separator everywhere.

```cpp nocompile
#include <Vector>            // BAD: the header is <vector>; fails on case-sensitive file systems
#include "Net\Socket.h"      // BAD: backslash isn't portable, and the file is net/socket.h
```

```cpp
#include <vector>            // GOOD: exact spelling
#include <string>
// #include "net/socket.h"   // GOOD: forward slashes, exact case
```

The BAD block is `nocompile`: whether it compiles depends on the file
system.

## SF.20: Use `namespace`s to express logical structure

Group what belongs together in a namespace named for the component. Names
then say where they come from, and two components can both have a `parse`
without colliding.

```cpp compile-fail="redefinition of"
// --- json component ---
int parse(const char* text) { return text[0] == '{'; }
// --- csv component ---
int parse(const char* text) { return text[0] != '\0'; }   // BAD: no namespaces: redefinition
```

```cpp
namespace json {
    int parse(const char* text) { return text[0] == '{'; }   // GOOD: json::parse
}
namespace csv {
    int parse(const char* text) { return text[0] != '\0'; }  // GOOD: csv::parse
}
int both(const char* t) { return json::parse(t) + csv::parse(t); }
```

## SF.21: Don't use an unnamed (anonymous) namespace in a header

Each file that includes a header gets its own copy of whatever is in an
unnamed namespace there: a separate variable per file, duplicate code, and
inline functions or templates that refer to it break the one-definition
rule. In a header, use a named namespace and `inline`.

```cpp bad
// --- config.h ---
namespace {
    int load_count = 0;                         // BAD: one variable per including file
    int load() { return ++load_count; }         // BAD: each file counts separately
}
```

```cpp
// --- config.h ---
namespace config {
    inline int load_count = 0;                  // GOOD: one variable for the program
    inline int load() { return ++load_count; }
}
```

The BAD block compiles; the files just silently stop sharing state.

## SF.22: Use an unnamed (anonymous) namespace for all internal/non-exported entities

Helpers used only in one `.cpp` file belong in an unnamed namespace there.
They get internal linkage, so they can't clash with a same-named function in
another file and nothing outside can come to depend on them. It also works
for types, which `static` doesn't.

```cpp bad
// --- report.cpp ---
int format_row(int x) { return x * 2; }   // BAD: external linkage; another file's format_row
int report(int x) { return format_row(x); }   // clashes at link time
```

```cpp
// --- report.cpp ---
namespace {
    struct Row { int value; };                        // GOOD: internal to this file
    int format_row(int x) { return Row{x * 2}.value; }
}
int report(int x) { return format_row(x); }           // the only exported function
```

The BAD block compiles; the clash appears only when another file defines
`format_row` too.
