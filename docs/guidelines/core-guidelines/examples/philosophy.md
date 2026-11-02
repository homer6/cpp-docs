# P: Philosophy — examples

> Our examples for the [P rules](../philosophy.md): the general ideas the
> other sections make concrete. Say what you mean in code, let the compiler
> check it, and don't leak or waste.

## P.1: Express ideas directly in code

The compiler checks code, not comments. When a loop is really a search, call
the search algorithm; when a number is really a temperature, give it a type.
Then the meaning is in the code, where tools can check it.

```cpp nocompile
#include <vector>
// BAD: compiles and works, but the reader has to work out that this is a
// search, and the "-1 means not found" convention lives only in our heads.
int index_of(const std::vector<int>& v, int x) {
    int i = 0;
    for (; i < static_cast<int>(v.size()); ++i)
        if (v[i] == x) break;
    return i == static_cast<int>(v.size()) ? -1 : i;
}
```

```cpp
#include <algorithm>
#include <vector>
struct Celsius { double value; };                  // GOOD: the unit is a type
void set_target(Celsius) {}

bool contains(const std::vector<int>& v, int x) {
    return std::ranges::find(v, x) != v.end();     // GOOD: says "find"
}
void tune() { set_target(Celsius{21.5}); }
```

Tested by [`tests/guidelines/philosophy_test.cpp`](../../../../tests/guidelines/philosophy_test.cpp).

## P.2: Write in ISO Standard C++

Compiler extensions tie the code to one compiler, and their meaning is often
not written down anywhere. A variable-length array is the classic case: GCC
and Clang accept it by default, but it isn't C++. With `-pedantic-errors` both
reject it.

```cpp compile-fail="variable length array|variable-length array"
void fill(int n) {
    int buffer[n];          // BAD: a VLA is a GCC/Clang extension, not ISO C++
    for (int i = 0; i < n; ++i) buffer[i] = i;
}
```

```cpp
#include <vector>
void fill(int n) {
    std::vector<int> buffer(static_cast<std::size_t>(n));   // GOOD: standard, sized at run time
    for (int i = 0; i < n; ++i) buffer[static_cast<std::size_t>(i)] = i;
}
```

## P.3: Express intent

Write what should happen, not just how. A range-`for` over `const&` says "look
at every element, change none of them"; an index loop says only "count from 0",
and leaves the index around after the loop.

```cpp nocompile
#include <vector>
// BAD: compiles, but the index is an implementation detail that outlives the
// loop, and nothing stops the body from changing the elements.
int total(std::vector<int>& v) {
    int sum = 0;
    std::size_t i = 0;
    for (i = 0; i < v.size(); ++i) sum += v[i];
    return sum;
}
```

```cpp
#include <vector>
int total(const std::vector<int>& v) {
    int sum = 0;
    for (const int x : v) sum += x;   // GOOD: every element, read only
    return sum;
}
```

## P.4: Ideally, a program should be statically type safe

Unions, casts, array decay, range errors and narrowing conversions are where
the type system stops protecting you. Each has a safer replacement; here a
tagged `std::variant` replaces a union that the code reads as the wrong type.

```cpp nocompile
// BAD: compiles, but reading i after writing f reads the inactive member: UB.
union Value { int i; float f; };
int read_back() {
    Value v;
    v.f = 1.5f;
    return v.i;
}
```

```cpp
#include <variant>
int read_back() {
    std::variant<int, float> v = 1.5f;          // GOOD: the variant knows what it holds
    if (const int* i = std::get_if<int>(&v)) return *i;
    return 0;                                   // it holds a float, so we get here
}
```

Tested by [`tests/guidelines/philosophy_test.cpp`](../../../../tests/guidelines/philosophy_test.cpp).

## P.5: Prefer compile-time checking to run-time checking

A check the compiler does costs nothing at run time and needs no error
handler. When an assumption is about types or constants, state it as a
`static_assert` or encode it in a type instead of testing it while the program
runs.

```cpp nocompile
#include <stdexcept>
// BAD: compiles, but the check runs on every call, can only fail after the
// program ships, and needs an error path for something known at build time.
void store(long long id) {
    if (sizeof(long long) < 8) throw std::runtime_error("ids need 64 bits");
    use(id);
}
```

```cpp
#include <cstdint>
static_assert(sizeof(long long) >= 8, "ids need 64 bits");   // GOOD: checked when compiling
void store(std::int64_t id) { use(id); }                     // GOOD: or let the type say it
```

Tested by [`tests/guidelines/philosophy_test.cpp`](../../../../tests/guidelines/philosophy_test.cpp).

## P.6: What cannot be checked at compile time should be checkable at run time

Some facts are only known at run time, such as how many elements a buffer holds.
Keep that information with the data so that a check is at least possible.
A bare pointer has thrown the size away; a `std::span` carries it.

```cpp nocompile
// BAD: compiles, but f can't know how many ints p points to, so no check
// is possible, at compile time or at run time.
void f(int* p) { p[9] = 0; }
```

```cpp
#include <span>
void f(std::span<int> s) {
    if (s.size() > 9) s[9] = 0;     // GOOD: the size travels with the data
}
```

Tested by [`tests/guidelines/philosophy_test.cpp`](../../../../tests/guidelines/philosophy_test.cpp).

## P.7: Catch run-time errors early

Check a value once, where it enters the program, and then carry it in a type
that can only hold valid values. Errors show up at the source instead of as
corrupted data far away, and the rest of the code doesn't re-check.

```cpp nocompile
#include <string>
// BAD: compiles, but a bad month passes through as a string and is only
// found (if at all) deep inside whatever finally parses it.
void schedule(const std::string& month) { use(month); }
void caller() { schedule("13"); }
```

```cpp
#include <stdexcept>
class Month {
public:
    explicit Month(int m) : m_{m} {
        if (m < 1 || m > 12) throw std::invalid_argument("month out of range");   // GOOD: checked once
    }
    int value() const { return m_; }
private:
    int m_;
};
void schedule(Month m) { use(m.value()); }   // every Month here is valid
```

Tested by [`tests/guidelines/philosophy_test.cpp`](../../../../tests/guidelines/philosophy_test.cpp).

## P.8: Don't leak any resources

Every resource you acquire (memory, a file, a lock, a socket) must be released
on every path, including early returns and exceptions. Give it to an object
whose destructor releases it (RAII), and no path can forget.

```cpp nocompile
// BAD: compiles, but if process() throws or we return early, the Widget leaks.
bool handle(bool skip) {
    Widget* w = new Widget;
    if (skip) return false;          // leak
    process(*w);                     // leak if this throws
    delete w;
    return true;
}
```

```cpp
#include <memory>
bool handle(bool skip) {
    auto w = std::make_unique<Widget>();   // GOOD: freed on every path
    if (skip) return false;
    process(*w);
    return true;
}
```

Tested by [`tests/guidelines/philosophy_test.cpp`](../../../../tests/guidelines/philosophy_test.cpp).

## P.9: Don't waste time or space

C++ is chosen for efficiency, so don't throw it away on work that buys
nothing. Calling `strlen` in a loop condition walks the whole string on every
iteration, which turns a linear loop into a quadratic one.

```cpp nocompile
#include <cstring>
// BAD: compiles and is correct, but strlen(s) runs on every iteration: O(n²).
int count_spaces(const char* s) {
    int n = 0;
    for (std::size_t i = 0; i < std::strlen(s); ++i)
        if (s[i] == ' ') ++n;
    return n;
}
```

```cpp
#include <algorithm>
#include <string_view>
long count_spaces(std::string_view s) {
    return std::ranges::count(s, ' ');   // GOOD: one pass; the length is computed once
}
```

## P.10: Prefer immutable data to mutable data

A value that can't change is easier to reason about, can't be changed by
mistake, and can't be part of a data race. Make it `const` (or `constexpr`)
and the compiler rejects an accidental write. The [Con examples](const.md)
go further.

```cpp compile-fail="read-only|const-qualified"
const double tax_rate = 0.13;
double with_tax(double price) {
    tax_rate = 0.15;                 // BAD: the mistake is caught: tax_rate is const
    return price * (1 + tax_rate);
}
```

```cpp
constexpr double tax_rate = 0.13;    // GOOD: fixed, and known at compile time
double with_tax(double price) { return price * (1 + tax_rate); }
```

Tested by [`tests/guidelines/philosophy_test.cpp`](../../../../tests/guidelines/philosophy_test.cpp).

## P.11: Encapsulate messy constructs, rather than spreading through the code

Low-level code (manual memory, casts, tricky lifetimes) is where bugs hide.
Write it once, behind a clean interface, or better, use a library that already
did. Then the rest of the program uses the interface and never sees the mess.

```cpp nocompile
#include <cstdlib>
// BAD: compiles, but every caller repeats the growth logic; this copy forgets
// to check for a null result and to update the capacity.
void push(int*& data, std::size_t& size, std::size_t& capacity, int x) {
    if (size == capacity)
        data = static_cast<int*>(std::realloc(data, 2 * capacity * sizeof(int)));
    data[size++] = x;
}
```

```cpp
#include <vector>
void push(std::vector<int>& v, int x) { v.push_back(x); }   // GOOD: growth lives in one well-tested place
```

## P.12: Use supporting tools as appropriate

*No example: it's about running static analyzers, sanitizers and linters on the code, not about how to write a piece of code.*

## P.13: Use support libraries as appropriate

A well-used library is better tested, better documented and usually faster
than what you'd write in an afternoon. Reach for the standard library (and
your project's foundation libraries) first, and need a reason not to.

```cpp nocompile
#include <vector>
// BAD: compiles, but it's a hand-rolled O(n²) sort that we now own, test and debug.
void sort_scores(std::vector<int>& v) {
    for (std::size_t i = 0; i < v.size(); ++i)
        for (std::size_t j = 0; j + 1 < v.size() - i; ++j)
            if (v[j] > v[j + 1]) { int t = v[j]; v[j] = v[j + 1]; v[j + 1] = t; }
}
```

```cpp
#include <algorithm>
#include <vector>
void sort_scores(std::vector<int>& v) { std::ranges::sort(v); }   // GOOD
```

Tested by [`tests/guidelines/philosophy_test.cpp`](../../../../tests/guidelines/philosophy_test.cpp).
