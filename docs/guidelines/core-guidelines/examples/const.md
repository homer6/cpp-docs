# Con: Constants and immutability — examples

> Our examples for the [Con rules](../const.md): immutability by default, so
> the compiler rejects an unintended change.

## Con.1: By default, make objects immutable

An object that never changes is easier to reason about and can't be changed by
mistake. Declare it `const` unless you have a reason to change it later; then
the compiler rejects an accidental write.

```cpp compile-fail="read-only|const-qualified"
int area(int w, int h) {
    const int result = w * h;
    result += 1;                  // BAD: the mistake is caught: result is const
    return result;
}
```

```cpp
int area(int w, int h) {
    const int result = w * h;     // GOOD: computed once, never changed
    return result;
}
```

Tested by [`tests/guidelines/const_test.cpp`](../../../../tests/guidelines/const_test.cpp).

## Con.2: By default, make member functions `const`

A member function that doesn't change the object should say so. Otherwise it
can't be called on a `const` object or through a `const&`, and callers lose
the promise that it has no effect on the object.

```cpp compile-fail="discards qualifiers|this argument has type|'this' argument"
struct Circle {
    double r;
    double area() { return 3.14159 * r * r; }   // BAD: not const
};
double total(const Circle& c) { return c.area(); }   // error: c is const
```

```cpp
struct Circle {
    double r;
    double area() const { return 3.14159 * r * r; }   // GOOD
};
double total(const Circle& c) { return c.area(); }
```

Tested by [`tests/guidelines/const_test.cpp`](../../../../tests/guidelines/const_test.cpp).

## Con.3: By default, pass pointers and references to `const`s

A parameter that the function only reads is `const T&` (or `const T*`). The
signature then tells callers their argument isn't changed, and it accepts
temporaries and `const` objects too.

```cpp compile-fail="cannot bind|expects an lvalue"
#include <string>
std::size_t length(std::string& s) { return s.size(); }   // BAD: only reads s
std::size_t n = length(std::string{"hi"});              // error: a temporary can't bind to std::string&
```

```cpp
#include <string>
std::size_t length(const std::string& s) { return s.size(); }   // GOOD
std::size_t n = length(std::string{"hi"});
```

Tested by [`tests/guidelines/const_test.cpp`](../../../../tests/guidelines/const_test.cpp).

## Con.4: Use `const` to define objects with values that do not change after construction

When a value is fixed once it's built, make the object `const`, even when the
value is only known at run time. Readers then know it stays the same for the
rest of the scope.

```cpp bad
#include <string>
#include <vector>
void greet(const std::vector<std::string>& names) {
    std::string greeting = "Hello, ";   // BAD: never changes, but nothing says so
    for (const auto& n : names) use(greeting + n);
}
```

```cpp
#include <string>
#include <vector>
void greet(const std::vector<std::string>& names) {
    const std::string greeting = "Hello, ";   // GOOD: fixed after construction
    for (const auto& n : names) use(greeting + n);
}
```

The BAD version compiles, so it's tagged `bad` (compiles, but is wrong): the
missing promise is for a reader or a linter to catch, such as clang-tidy's
`misc-const-correctness`.

Tested by [`tests/guidelines/const_test.cpp`](../../../../tests/guidelines/const_test.cpp).

## Con.5: Use `constexpr` for values that can be computed at compile time

`constexpr` makes the compiler compute the value, so it costs nothing at run
time and can be used where a constant expression is required: array bounds,
template arguments, `static_assert`.

```cpp compile-fail="constant|constexpr"
int square(int x) { return x * x; }
const int side = 4;
static_assert(square(side) == 16);   // BAD: square isn't constexpr, so this isn't a constant expression
```

```cpp
constexpr int square(int x) { return x * x; }
constexpr int side = 4;
constexpr int area = square(side);   // GOOD: computed by the compiler
static_assert(area == 16);
```

Tested by [`tests/guidelines/const_test.cpp`](../../../../tests/guidelines/const_test.cpp).
