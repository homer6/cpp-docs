# Constant expressions (`constexpr`, `consteval`, `constinit`)

> Moving computation to compile time: `constexpr` allows it, `consteval` requires
> it, `constinit` guarantees static initialization happens at compile time.

## constexpr — may run at compile time

A `constexpr` variable is a compile-time constant; a `constexpr` function *can*
run at compile time when its inputs are constant, and at runtime otherwise.

```cpp
constexpr int square(int x) { return x * x; }
int read();                       // some runtime input

constexpr int a = square(5);     // computed at compile time → 25
int n = read();
int b = square(n);                // same function, runs at runtime here

constexpr int N = square(8);
int arr[N];                       // usable where a constant is required
static_assert(square(4) == 16);
```

Modern `constexpr` is powerful: loops, branches, local variables, most of the
standard library (containers, `std::string`, algorithms) are `constexpr` in
recent standards.
What each standard allows inside constant evaluation (virtual calls, transient
`new`, `goto`, placement `new`…) is on
[what constant evaluation allows](constant-evaluation-rules.md).

## consteval — must run at compile time (C++20)

An **immediate function**: every call is evaluated at compile time, or it's an
error. Use it to force compile-time-only work (e.g. validating a format string).

```cpp compile-fail="call to consteval function .* is not a constant expression"
consteval int force_ct(int x) { return x * x; }
int read();

constexpr int ok = force_ct(5);   // fine
int n = read();
int bad = force_ct(n);             // ERROR: argument isn't a constant expression
```

## constinit — guarantee static init at compile time (C++20)

Asserts a static/thread-local variable is initialized at compile time, avoiding
the static-initialization-order fiasco — without making it `const`.

```cpp
constexpr int compute_constant() { return 42; }

constinit int counter = compute_constant();   // must be a constant initializer;
                                               // but `counter` is still mutable at runtime
```

## `if constexpr` — compile-time branch (C++17)

Discards the untaken branch at compile time — essential for templates:

```cpp
template<class T>
auto stringify(const T& v) {
    if constexpr (std::is_arithmetic_v<T>) return std::to_string(v);
    else                                   return std::string{v};
}
```

The false branch isn't compiled for that instantiation, so each branch only needs
to be valid for the types that reach it.

## `static_assert` with a computed message (C++26)

The message can be any constant expression whose type has `size()` and
`data()`, such as a `constexpr` `std::string`, so it can name the values that
failed:

```cpp compile-fail="Slots holds at most 4, got 7"
template <std::size_t N>
struct Slots {
    static_assert(N <= 4, std::string("Slots holds at most 4, got ") + char('0' + N));
};
Slots<3> fine;
Slots<7> bad;     // error: static assertion failed: Slots holds at most 4, got 7
```

Clang evaluates the message even when the assertion holds, so it must always be
a constant expression (`std::to_string` isn't `constexpr`, so it can't be used).

## Casting from `void*` (C++26)

`static_cast` from `void*` back to the object's real type is allowed during
constant evaluation, which lets type-erased code run at compile time:

```cpp
constexpr int through_void(int v) {
    void* erased = &v;
    return *static_cast<int*>(erased);   // ok in constexpr since C++26
}
static_assert(through_void(7) == 7);
```

Casting to the *wrong* type is still undefined, so in a constant expression it's
an error instead of a silent bug.

## Exceptions during constant evaluation (C++26)

A `constexpr` function may `throw` and `catch` while being evaluated at compile
time. An exception caught inside the evaluation is fine; one that escapes makes
the expression non-constant, and the compiler reports it. Only GCC 16 has this
(`__cpp_constexpr_exceptions` is `202411L`); GCC 15 and Clang 22 reject any
`throw` reached at compile time.

```cpp needs=constexpr-exceptions
#include <stdexcept>

constexpr int checked_div(int a, int b) {
    if (b == 0) throw std::invalid_argument("divide by zero");
    return a / b;
}
constexpr int div_or_zero(int a, int b) {
    try { return checked_div(a, b); }
    catch (const std::invalid_argument&) { return 0; }
}
static_assert(div_or_zero(10, 2) == 5);
static_assert(div_or_zero(1, 0) == 0);   // thrown and caught at compile time
// constexpr int r = checked_div(1, 0);  // error: uncaught exception of type 'std::invalid_argument'
```

The same function still throws normally at run time; see
[throw / try / catch](../exceptions/throw-try-catch.md).

## Gotchas

- **`constexpr` is a *permission*, not a guarantee** — a `constexpr` function
  still runs at runtime if its arguments aren't constant. Use `consteval` (or
  assign to a `constexpr`/`static_assert`) to *force* compile time.
- **`constexpr` on a variable implies `const`; on a function it doesn't make the
  function `const`** (a member `constexpr` function isn't automatically a `const`
  member — though it usually should be).
- **`constinit` ≠ `constexpr`.** `constinit` only constrains *initialization*
  timing; the object remains mutable. `constexpr` makes the object a constant.
- **A computed `static_assert` message must be a constant expression even when
  the assertion passes** (Clang checks it every time).
- **`static_cast` from `void*` must name the object's actual type.** Casting an
  `int`'s address to `float*` is rejected during constant evaluation (GCC: "pointed-to
  type 'int' is not similar to 'float'").
- **`if constexpr` discards, it doesn't just skip.** The discarded branch must
  still parse, but needn't be valid for the current instantiation — that's what
  makes it more powerful than a runtime `if` in templates.

## See also

- [constexpr — cppreference](https://en.cppreference.com/w/cpp/language/constexpr)
- [consteval — cppreference](https://en.cppreference.com/w/cpp/language/consteval)
- [constinit — cppreference](https://en.cppreference.com/w/cpp/language/constinit)
- [if constexpr — cppreference](https://en.cppreference.com/w/cpp/language/if)
- [static_assert — cppreference](https://en.cppreference.com/w/cpp/language/static_assert) ·
  [constant expressions — cppreference](https://en.cppreference.com/w/cpp/language/constant_expression)
- [P2741: user-generated static_assert messages](https://wg21.link/P2741) ·
  [P2738: constexpr cast from `void*`](https://wg21.link/P2738) ·
  [P3068: allowing exception throwing in constant evaluation](https://wg21.link/P3068)
