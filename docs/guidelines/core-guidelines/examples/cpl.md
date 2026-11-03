# CPL: C-style programming — examples

> Our examples for the [CPL rules](../cpl.md): when C and C++ meet, compile
> with C++'s stricter checks and keep C++ in charge of the calling code.

## CPL.1: Prefer C++ to C

C++ checks types more strictly and gives you tools C lacks (constructors and
destructors, containers, templates). C silently converts `void*` to any
object pointer; C++ refuses, which catches a whole family of mistakes.

```cpp compile-fail="invalid conversion from 'void\*'|cannot initialize a variable of type 'int \*' with an rvalue of type 'void \*'"
#include <cstdlib>
void make_buffer() {
    int* p = std::malloc(10 * sizeof(int));   // BAD: valid C, rejected by C++: void* doesn't convert implicitly
    std::free(p);
}
```

```cpp
#include <vector>
void make_buffer() {
    std::vector<int> buffer(10);   // GOOD: typed, sized and freed for us
    use(buffer);
}
```

Tested by [`tests/guidelines/cpl_test.cpp`](../../../../tests/guidelines/cpl_test.cpp).

## CPL.2: If you must use C, use the common subset of C and C++, and compile the C code as C++

Code written in the common subset compiles as both C and C++, and the C++
compiler checks it more strictly. Here, taking a non-`const` pointer to a
`const` object is only a warning in C but an error in C++.

```cpp compile-fail="invalid conversion from 'const int\*'|cannot initialize a variable of type 'int \*' with an rvalue of type 'const int \*'"
static const int table_size = 8;
void reset() {
    int* p = &table_size;   // BAD: C warns; C++ rejects dropping the const
    *p = 0;
}
```

```cpp
/* GOOD: valid C and valid C++, with the declarations C++ code needs. */
#ifdef __cplusplus
extern "C" {
#endif
static const int table_size = 8;
int table_entry(const int* table, int i) { return (i >= 0 && i < table_size) ? table[i] : 0; }
#ifdef __cplusplus
}
#endif
```

Tested by [`tests/guidelines/cpl_test.cpp`](../../../../tests/guidelines/cpl_test.cpp).

## CPL.3: If you must use C for interfaces, use C++ in the calling code using such interfaces

When a library only has a C interface, keep the C at the edge: wrap its
handles in C++ types whose destructors call the library's release function.
Then the calling code gets RAII and exception safety.

```cpp bad
#include <cstdio>
// BAD: compiles, but if use() throws, or a return is added later, the file stays open.
void dump(const char* path) {
    std::FILE* f = std::fopen(path, "r");
    if (!f) return;
    use(f);
    std::fclose(f);
}
```

```cpp
#include <cstdio>
#include <memory>
struct FileCloser { void operator()(std::FILE* f) const { std::fclose(f); } };
using File = std::unique_ptr<std::FILE, FileCloser>;   // GOOD: C handle, C++ ownership

void dump(const char* path) {
    File f{std::fopen(path, "r")};
    if (!f) return;
    use(f.get());              // closed on every path
}
```

Tested by [`tests/guidelines/cpl_test.cpp`](../../../../tests/guidelines/cpl_test.cpp).
