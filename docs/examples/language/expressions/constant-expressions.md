# Constant expressions (`constexpr`, `consteval`, `constinit`)

> Moving computation to compile time: `constexpr` allows it, `consteval` requires
> it, `constinit` guarantees static initialization happens at compile time.

## constexpr — may run at compile time

A `constexpr` variable is a compile-time constant; a `constexpr` function *can*
run at compile time when its inputs are constant, and at runtime otherwise.

```cpp
constexpr int square(int x) { return x * x; }

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

## consteval — must run at compile time (C++20)

An **immediate function**: every call is evaluated at compile time, or it's an
error. Use it to force compile-time-only work (e.g. validating a format string).

```cpp
consteval int force_ct(int x) { return x * x; }

constexpr int ok = force_ct(5);   // fine
int n = read();
int bad = force_ct(n);             // ERROR: argument isn't a constant expression
```

## constinit — guarantee static init at compile time (C++20)

Asserts a static/thread-local variable is initialized at compile time, avoiding
the static-initialization-order fiasco — without making it `const`.

```cpp
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

## Gotchas

- **`constexpr` is a *permission*, not a guarantee** — a `constexpr` function
  still runs at runtime if its arguments aren't constant. Use `consteval` (or
  assign to a `constexpr`/`static_assert`) to *force* compile time.
- **`constexpr` on a variable implies `const`; on a function it doesn't make the
  function `const`** (a member `constexpr` function isn't automatically a `const`
  member — though it usually should be).
- **`constinit` ≠ `constexpr`.** `constinit` only constrains *initialization*
  timing; the object remains mutable. `constexpr` makes the object a constant.
- **`if constexpr` discards, it doesn't just skip.** The discarded branch must
  still parse, but needn't be valid for the current instantiation — that's what
  makes it more powerful than a runtime `if` in templates.

## See also

- [constexpr — cppreference](https://en.cppreference.com/w/cpp/language/constexpr)
- [consteval — cppreference](https://en.cppreference.com/w/cpp/language/consteval)
- [constinit — cppreference](https://en.cppreference.com/w/cpp/language/constinit)
- [if constexpr — cppreference](https://en.cppreference.com/w/cpp/language/if)
