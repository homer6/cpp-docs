# What constant evaluation allows

> What code may do while it runs at compile time, and how each standard since
> C++20 widened it. **Since:** C++11, relaxed in C++14, C++20, C++23 and C++26

The specifiers themselves (`constexpr`, `consteval`, `constinit`, `if constexpr`)
are on [constant expressions](constant-expressions.md). Asking "am I running at
compile time?" (`std::is_constant_evaluated`, `if consteval`,
`std::is_within_lifetime`) is on
[constant evaluation context](../../metaprogramming/constant-evaluation.md).

## Mental model

The compiler runs constant-evaluated code in an interpreter of the abstract
machine. The interpreter tracks every object's type, bounds and lifetime, so it
can refuse anything the language leaves undefined:

```
constexpr int r = f();    compiler interprets f()
                          ├─ every step legal?          → r is a constant
                          ├─ undefined behaviour hit?   → compile error
                          ├─ new without delete?        → compile error
                          └─ step / depth limit hit?    → compile error
```

Each standard has removed restrictions:

| Standard | Now allowed in constant evaluation | Paper |
|---|---|---|
| C++20 | `virtual` calls, `dynamic_cast`, `typeid` | P1064, P1327 |
| C++20 | `try`/`catch` blocks (no `throw` yet), changing a union's active member | P1002, P1330 |
| C++20 | `new`/`delete`, `std::allocator`, `std::construct_at` (transient only) | P0784 |
| C++23 | `static constexpr` locals; non-literal variables, `static` locals and `goto` that aren't reached | P2647, P2242 |
| C++23 | a `constexpr` function that can never be constant isn't an error | P2448 |
| C++23 | `consteval` propagates up through templates and lambdas | P2564 |
| C++26 | casting from `void*`, `throw` (see [constant expressions](constant-expressions.md)) | P2738, P3068 |
| C++26 | placement `new` | P2747 |

## Examples

### `virtual`, `dynamic_cast` and `typeid` (C++20)

Polymorphism works at compile time. The interpreter knows each object's dynamic
type:

```cpp
struct Shape {
    virtual constexpr ~Shape() = default;
    virtual constexpr int area() const = 0;
};
struct Square : Shape {
    int side;
    constexpr explicit Square(int s) : side(s) {}
    constexpr int area() const override { return side * side; }
};

constexpr bool check() {
    Square sq{3};
    const Shape& s = sq;
    return s.area() == 9                                 // virtual call
        && dynamic_cast<const Square*>(&s) != nullptr    // dynamic_cast
        && typeid(s) == typeid(Square);                  // typeid
}
static_assert(check());
```

### `try`/`catch` and union members (C++20)

A `try` block may appear, as long as nothing throws during constant evaluation
(throwing came in C++26). A union's active member may change by assignment or
`std::construct_at`:

```cpp
constexpr int guarded(int x) {
    try { return x + 1; }            // OK since C++20: nothing is thrown
    catch (...) { return -1; }
}
static_assert(guarded(1) == 2);

union IntOrFloat { int i; float f; };
constexpr float switch_member() {
    IntOrFloat u{.i = 1};
    u.f = 2.5f;                      // f becomes the active member
    return u.f;                      // reading u.i here would be an error
}
static_assert(switch_member() == 2.5f);
```

### Transient allocation (C++20)

`new` and `delete` work, and so do `std::vector` and `std::string`, but every
allocation must be freed before the evaluation ends. Use them inside the
computation and return a plain value:

```cpp
constexpr int sum_to(int n) {
    std::vector<int> v(n);
    std::iota(v.begin(), v.end(), 1);
    return std::accumulate(v.begin(), v.end(), 0);   // v is freed on return
}
static_assert(sum_to(100) == 5050);
```

The allocation can't escape into a `constexpr` variable, because nothing would
be left to free it at run time:

```cpp compile-fail="operator new|heap-allocated"
constexpr std::vector<int> v = {1, 2, 3};   // error: refers to memory from operator new
```

A `new` without a `delete` is an error too ("allocated storage has not been
deallocated"). To keep the result, copy it into a `std::array` before returning.

### `static constexpr` locals, non-literal variables, `goto` (C++23)

A `static constexpr` local is allowed. A variable of non-literal type, a plain
`static` local, or a `goto` may now appear in a `constexpr` function, as long as constant
evaluation never reaches it:

```cpp
constexpr char hex_digit(int n) {
    static constexpr char digits[] = "0123456789abcdef";   // C++23
    return digits[n];
}
static_assert(hex_digit(11) == 'b');

constexpr int parse_digit(char c) {
    if (c < '0' || c > '9') goto fail;    // C++23: may appear...
    return c - '0';
fail:
    static int failures = 0;              // ...and so may a static local
    ++failures;
    return -1;                            // ...but only run time may get here
}
static_assert(parse_digit('7') == 7);
// static_assert(parse_digit('x') == -1);  // error: 'goto' is not a constant expression
```

### `consteval` propagation (C++23)

A function template or lambda that calls a `consteval` function with one of its
own parameters becomes `consteval` itself, instead of being an error (P2564):

```cpp
consteval int checked(int x) { return x; }

template <class T>
constexpr int forward_checked(T x) { return checked(x); }   // becomes immediate

static_assert(forward_checked(5) == 5);
```

Calling `forward_checked` with a run-time value is then an error at the call
site. A plain (non-template) `constexpr` function doesn't escalate: calling
`checked(x)` in one is an error, as before.

### Placement `new` (C++26) and `std::construct_at` (C++20)

`std::construct_at` was C++20's constexpr spelling of placement `new`. Since
C++26 placement `new` itself works (`__cpp_constexpr >= 202406L`, P2747), on
storage from `std::allocator` or on a union member:

```cpp
struct Point { int x, y; };

constexpr int build() {
    std::allocator<Point> alloc;
    Point* p = alloc.allocate(2);
    std::construct_at(p, 1, 2);                         // C++20
    ::new (static_cast<void*>(p + 1)) Point{3, 4};      // C++26
    int r = p[0].x + p[1].y;
    std::destroy(p, p + 2);
    alloc.deallocate(p, 2);
    return r;
}
static_assert(build() == 5);
```

### Undefined behaviour is a compile error

Signed overflow, reading out of bounds, using freed memory, dividing by zero, an
oversized shift or reading an uninitialized variable is UB at run time, but in
constant evaluation the compiler must diagnose it. This makes `static_assert` a
UB detector for `constexpr` code:

```cpp compile-fail="overflow in constant expression|outside the range of representable"
constexpr int add(int a, int b) { return a + b; }
constexpr int r = add(2'147'483'647, 1);      // error: signed overflow

constexpr int at(int i) { int a[3] = {1, 2, 3}; return a[i]; }
constexpr int r2 = at(3);                     // error: out of bounds

constexpr int dangle() { int* p = new int(1); delete p; return *p; }
constexpr int r3 = dangle();                  // error: use after delete
```

The same calls compile and silently misbehave when they run at run time.

### Evaluation limits

Compilers stop runaway evaluations. Each limit is a flag:

| Limit | GCC (default) | Clang (default) |
|---|---|---|
| total work | `-fconstexpr-ops-limit=` (33554432) | `-fconstexpr-steps=` (1048576) |
| iterations of one loop | `-fconstexpr-loop-limit=` (262144) | counted in the steps |
| recursion depth | `-fconstexpr-depth=` (512) | `-fconstexpr-depth=` (512) |

A loop of two million iterations fails with both defaults ("loop iteration
count exceeds limit of 262144" on GCC, "hit maximum step limit" on Clang).

### The constexpr standard library

**Algorithms.** Most of `<algorithm>` and `<numeric>` has been `constexpr`
since C++20 (P0202, P0879: `std::sort`, `std::accumulate`…). The ones that may
allocate a buffer, `std::stable_sort`, `std::stable_partition` and
`std::inplace_merge`, followed in C++26 (P2562,
`__cpp_lib_constexpr_algorithms == 202306L`; all three toolchains):

```cpp
struct Rec { int key; char tag; };

constexpr bool keeps_order() {
    std::array<Rec, 4> r{{{2, 'a'}, {1, 'b'}, {2, 'c'}, {1, 'd'}}};
    std::stable_sort(r.begin(), r.end(), [](Rec x, Rec y) { return x.key < y.key; });
    return r[0].tag == 'b' && r[1].tag == 'd' && r[2].tag == 'a' && r[3].tag == 'c';
}
static_assert(keeps_order());   // equal keys keep their original order
```

**Containers and utilities.** Usable at compile time (transiently, as above):

| Since | What | GCC 15 | Clang 22 | GCC 16 |
|---|---|---|---|---|
| C++20 | `std::vector`, `std::string`, `std::optional`, `std::variant` | yes | yes | yes |
| C++23 | `std::unique_ptr` (P2273), `std::bitset`, `<charconv>` integers | yes | yes | yes |
| C++26 | `std::inplace_vector` | no | no | yes |
| C++26 | `list`, `forward_list`, `queue` (P3372) | no | yes | no |
| C++26 | `flat_map`, `flat_set` (P3372) | no | yes | yes |
| C++26 | `deque`, `map`, `set`, `unordered_*`, `stack` (P3372) | no | no | no |

Each has its own macro, such as `__cpp_lib_constexpr_list`.

**`<cmath>`.** C++23 made the simple functions `constexpr` (`fabs`, `floor`,
`fmax`, `fmod`, `copysign`…, P0533), and C++26 added most of the rest (`sqrt`,
`sin`, `exp`…, P1383; not the special functions). No toolchain here defines
`__cpp_lib_constexpr_cmath` yet. GCC accepts all of these anyway, even in
`-std=c++17`, because it folds its math builtins. Clang 22 with libc++ rejects
even `std::fabs` and integer `std::abs` ("non-constexpr function"), though
`std::isnan` and `std::signbit` work:

```cpp needs=constexpr-cmath
static_assert(std::fabs(-2.5) == 2.5);    // GCC: yes; Clang 22 + libc++: not yet
static_assert(std::sqrt(4.0) == 2.0);     // C++26 (P1383); GCC folds it today
```

## Gotchas

- **Transient means transient.** A `constexpr std::vector` variable doesn't
  compile. A short `constexpr std::string` sometimes does, because it fits the
  small buffer, but that depends on its length, the library and the scope
  (libstdc++ rejects it as a local): don't rely on it. Compute into a `std::array`.
- **Portable `constexpr` math needs care.** Code that `static_assert`s on
  `std::sqrt` compiles on GCC and fails on Clang. Test with Clang too, or write
  your own `constexpr` helper.
- **"Not a constant expression" hides the real error.** Clang's notes (and
  GCC's following lines) say *which* step failed: read past the first line.

## See also

- [Constant expressions — cppreference](https://en.cppreference.com/w/cpp/language/constant_expression) ·
  [constexpr — cppreference](https://en.cppreference.com/w/cpp/language/constexpr) ·
  [\[expr.const\] — the draft](https://eel.is/c++draft/expr.const)
- [P0784](https://wg21.link/P0784) (transient allocation) ·
  [P2242](https://wg21.link/P2242) (non-literal variables, goto) ·
  [P2647](https://wg21.link/P2647) (static constexpr locals) ·
  [P2564](https://wg21.link/P2564) (consteval propagation) ·
  [P2747](https://wg21.link/P2747) (placement new)
- [P2562](https://wg21.link/P2562) (stable sorting) ·
  [P3372](https://wg21.link/P3372) (containers) ·
  [P0533](https://wg21.link/P0533) and [P1383](https://wg21.link/P1383) (`<cmath>`)
- [GCC C++ dialect options](https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Dialect-Options.html) ·
  [Clang command-line reference](https://clang.llvm.org/docs/ClangCommandLineReference.html)
- [Constant evaluation context](../../metaprogramming/constant-evaluation.md) ·
  [lambdas](../functions/lambdas.md#generic-mutable-constexpr-consteval) ·
  [object model](../basic-concepts/object-model.md)
