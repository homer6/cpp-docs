# Pro: Profiles — examples

> Our examples for the [profile rules](../profile.md): the type, bounds and
> lifetime restrictions that together rule out whole classes of memory errors.
> [Safety](../../safety.md) explains the profiles and how to enforce them.

## Type.1: Avoid casts

A cast tells the compiler to stop checking. Most casts in modern code are
avoidable: use the right type in the first place, a conversion the language
checks, or a library function that does the conversion safely. The profile
singles out four patterns, each below.

### Don't use `reinterpret_cast`

Reading an object's bytes as an unrelated type through `reinterpret_cast`
breaks the aliasing rules. `std::bit_cast` (C++20) copies the bits into a new
object of the target type, which is defined behaviour.

```cpp nocompile
#include <cstdint>
// BAD: compiles, but reading a float through a uint32_t* breaks strict aliasing: UB.
std::uint32_t bits_of(float f) {
    return *reinterpret_cast<std::uint32_t*>(&f);
}
```

```cpp
#include <bit>
#include <cstdint>
std::uint32_t bits_of(float f) {
    return std::bit_cast<std::uint32_t>(f);   // GOOD: defined, and constexpr
}
```

### Don't use `static_cast` for arithmetic types

`static_cast<int>(x)` silently drops whatever doesn't fit. Brace
initialization rejects a narrowing conversion at compile time; where a value
really can be out of range, check it (`std::in_range`, C++20) or use a
checked helper like the GSL's `narrow`.

```cpp nocompile
#include <cstdint>
// BAD: compiles; 300 doesn't fit in int8_t, and the cast hides that.
std::int8_t level(int x) { return static_cast<std::int8_t>(x); }
```

```cpp
#include <cstdint>
#include <stdexcept>
#include <utility>
std::int8_t level(int x) {
    if (!std::in_range<std::int8_t>(x)) throw std::out_of_range("level");   // GOOD: checked
    return static_cast<std::int8_t>(x);                                     // now known to fit
}
std::int64_t widen(int x) { return std::int64_t{x}; }   // GOOD: {} would reject a narrowing
```

### Don't cast between pointer types where the source type and the target type are the same

A cast to the type the expression already has does nothing except make the
reader wonder what it was for, and it hides a real conversion if the source
type changes later.

```cpp nocompile
// BAD: compiles; w is already a Widget*, so the cast is noise.
void run_all(Widget* w) { static_cast<Widget*>(w)->run(); }
```

```cpp
void run_all(Widget* w) { w->run(); }   // GOOD
```

### Don't cast between pointer types when the conversion could be implicit

Converting a derived pointer to a base pointer (or any pointer to
`const void*`) happens implicitly and is checked. Writing a cast for it adds
nothing, and a cast written out of habit can later hide a conversion that
isn't safe.

```cpp nocompile
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape {};
// BAD: compiles; the upcast is implicit, so the static_cast is noise.
Shape* as_shape(Circle* c) { return static_cast<Shape*>(c); }
```

```cpp
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape {};
Shape* as_shape(Circle* c) { return c; }   // GOOD: implicit and checked
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Type.2: Don't use `static_cast` to downcast

`static_cast` from base to derived trusts you: if the object isn't really
that derived type, using the result is undefined. `dynamic_cast` checks at run
time and gives a null pointer (or throws, for references) when the object is
something else. Better still, a virtual function avoids the downcast.

```cpp nocompile
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape { double r = 1; };
struct Square : Shape { double side = 1; };
// BAD: compiles, but if s points to a Square, reading ->r is UB.
double radius(Shape* s) { return static_cast<Circle*>(s)->r; }
```

```cpp
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape { double r = 1; };
double radius(Shape* s) {
    if (auto* c = dynamic_cast<Circle*>(s)) return c->r;   // GOOD: null if it isn't a Circle
    return 0;
}
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Type.3: Don't use `const_cast` to cast away `const` (i.e., at all)

Writing to an object that was defined `const`, through a pointer you
`const_cast` to non-const, is undefined behaviour. When a `const` function
needs to update a cache, mark that member `mutable`; when you need both a
`const` and a non-const overload, write the non-const one properly.

```cpp nocompile
const int limit = 10;
// BAD: compiles, but limit is a const object, so this write is UB.
void raise_limit() { *const_cast<int*>(&limit) = 20; }
```

```cpp
#include <optional>
class Report {
public:
    int total() const {
        if (!cache_) cache_ = compute();   // GOOD: mutable states that the cache can change
        return *cache_;
    }
private:
    int compute() const { return 42; }
    mutable std::optional<int> cache_;
};
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Type.4: Don't use C-style `(T)expression` or functional `T(expression)` casts

A C-style cast tries `const_cast`, `static_cast` and `reinterpret_cast` in
turn until one works, so it can cast away `const` or reinterpret memory
without saying so. A named cast does one thing and fails to compile when asked
for something else; `T{x}` constructs and rejects narrowing.

```cpp nocompile
// BAD: compiles; the C-style cast quietly casts away const, so the write is UB.
void shout(const char* msg) {
    char* p = (char*)msg;
    p[0] = 'H';
}
```

```cpp compile-fail="invalid 'static_cast'|static_cast from .* is not allowed"
// The named cast refuses the same conversion.
void shout(const char* msg) {
    char* p = static_cast<char*>(msg);   // error: static_cast can't remove const
    p[0] = 'H';
}
```

```cpp
#include <string>
std::string shout(const char* msg) {
    std::string s{msg};                  // GOOD: construct a copy we may change
    if (!s.empty()) s[0] = 'H';
    return s;
}
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Type.5: Don't use a variable before it has been initialized

Reading an uninitialized local is undefined behaviour (C++26 calls it
*erroneous behaviour*, which is still a bug). Initialize every variable where
it's declared; if the value depends on a condition, compute it with a lambda or
a `?:` so the variable never exists without a value.

```cpp nocompile
// BAD: compiles; when flag is false, x is read uninitialized.
int choose(bool flag) {
    int x;
    if (flag) x = 1;
    return x;
}
```

```cpp
int choose(bool flag) {
    const int x = flag ? 1 : 0;   // GOOD: initialized on every path
    return x;
}
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Type.6: Always initialize a data member

A class whose constructor forgets a member leaves it holding garbage. Give
every member a default member initializer, so even a constructor added later
can't leave it unset.

```cpp nocompile
// BAD: compiles; Counter c; leaves c.count indeterminate.
struct Counter {
    int count;
    void bump() { ++count; }
};
```

```cpp
struct Counter {
    int count = 0;              // GOOD: every constructor starts from 0
    void bump() { ++count; }
};
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Type.7: Avoid naked union

A plain `union` doesn't record which member is active, so nothing stops code
from reading the wrong one. `std::variant` keeps the tag for you and refuses
(by throwing, or returning null from `get_if`) to hand out the wrong
alternative.

```cpp nocompile
// BAD: compiles; the code must remember which member is live, and here it doesn't: UB.
union Number { int i; double d; };
double as_double() {
    Number n;
    n.i = 3;
    return n.d;
}
```

```cpp
#include <variant>
double as_double() {
    std::variant<int, double> n = 3;
    return std::visit([](auto v) { return static_cast<double>(v); }, n);   // GOOD: handles whichever is live
}
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Type.8: Avoid varargs

A C variadic function (`...`, `va_arg`) has no idea what types it was passed;
the caller and callee just agree, and a mismatch is undefined behaviour. A
variadic template (or an `std::initializer_list`) keeps every argument's type,
and a constraint rejects the wrong ones at compile time.

```cpp nocompile
#include <cstdarg>
// BAD: compiles; sum(2, 1, 2.5) passes a double where va_arg reads an int: UB.
int sum(int count, ...) {
    va_list args;
    va_start(args, count);
    int total = 0;
    for (int i = 0; i < count; ++i) total += va_arg(args, int);
    va_end(args);
    return total;
}
```

```cpp
#include <concepts>
template <std::integral... Ts>
int sum(Ts... xs) { return (0 + ... + xs); }   // GOOD: every type known; sum(1, 2.5) won't compile
int six = sum(1, 2, 3);
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Bounds.1: Don't use pointer arithmetic. Use `span` instead

A pointer plus an offset can point anywhere; nothing records where the array
ends. `std::span` holds the pointer and the size together, so slicing with
`subspan`, `first` and `last` keeps track of the bounds.

```cpp nocompile
// BAD: compiles, but nothing stops p + n from running past the end of the array.
int sum_tail(const int* p, int n, int from) {
    int total = 0;
    for (const int* q = p + from; q != p + n; ++q) total += *q;
    return total;
}
```

```cpp
#include <span>
int sum_tail(std::span<const int> s, std::size_t from) {
    int total = 0;
    for (int x : s.subspan(from)) total += x;   // GOOD: the slice knows its own length
    return total;
}
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Bounds.2: Only index into arrays using constant expressions

An index computed at run time can be out of range, and `a[i]` doesn't check.
Iterate with a range-`for` or an algorithm, use `std::get<N>` (checked at
compile time) for a fixed position, or `at()` (checked at run time) when the
index really is dynamic.

```cpp nocompile
#include <array>
// BAD: compiles; for i >= 3 this reads past the end: UB.
int pick(const std::array<int, 3>& a, std::size_t i) { return a[i]; }
```

```cpp
#include <array>
int middle(const std::array<int, 3>& a) { return std::get<1>(a); }        // GOOD: checked at compile time
int pick(const std::array<int, 3>& a, std::size_t i) { return a.at(i); }   // GOOD: throws when out of range
```

A constant index that is out of range doesn't compile:

```cpp compile-fail="out of bounds|index out of|static assertion failed|static_assert"
#include <array>
int last(const std::array<int, 3>& a) { return std::get<3>(a); }   // error: index 3 of a 3-element array
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Bounds.3: No array-to-pointer decay

Passing a built-in array to a pointer parameter throws away its length.
Pass a `std::span` instead: it's built from the array, keeps the length (in
the type, for a fixed-size span), and costs the same as a pointer and a size.

```cpp nocompile
// BAD: compiles; inside average the array has decayed to int*, so its length is gone.
double average(const int* values) { return values[0]; }
int data[4] = {1, 2, 3, 4};
double a = average(data);
```

```cpp
#include <numeric>
#include <span>
double average(std::span<const int> values) {
    if (values.empty()) return 0;
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}
int data[4] = {1, 2, 3, 4};
double a = average(data);   // GOOD: the span knows there are 4
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Bounds.4: Don't use standard-library functions and types that are not bounds-checked

`strcpy`, `memcpy`, `sprintf` and friends write as far as the source tells
them, whatever the size of the destination. Use types that manage their own
size (`std::string`, `std::vector`), algorithms over ranges, and `at()` where
an index might be out of range.

```cpp nocompile
#include <cstring>
// BAD: compiles; a name longer than 15 characters overflows buf.
void greet(const char* name) {
    char buf[16];
    std::strcpy(buf, name);
    use(buf);
}
```

```cpp
#include <string>
void greet(const char* name) {
    std::string buf = name;   // GOOD: grows to fit
    use(buf);
}
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).

## Lifetime.1: Don't dereference a possibly invalid pointer

A pointer, reference or iterator is invalid once its object is gone: a local
that went out of scope, a deleted object, or a vector element after the
vector reallocated. Don't keep one past its object's life; return by value,
and re-fetch (or hold an index) after an operation that can invalidate.

```cpp nocompile
#include <vector>
// BAD: compiles; push_back may reallocate, leaving first dangling, so *first is UB.
int first_then_grow(std::vector<int>& v) {
    int* first = &v[0];
    v.push_back(42);
    return *first;
}
```

```cpp
#include <vector>
int first_then_grow(std::vector<int>& v) {
    const std::size_t first = 0;   // GOOD: an index stays valid across reallocation
    v.push_back(42);
    return v[first];
}
```

Tested by [`tests/guidelines/profile_test.cpp`](../../../../tests/guidelines/profile_test.cpp).
