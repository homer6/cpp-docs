# Constant evaluation context

> Asking "am I running at compile time?" inside a `constexpr` function, and
> what's alive during constant evaluation. **Header:** `<type_traits>` ·
> **Since:** C++20 (`is_constant_evaluated`), C++23 (`if consteval`), C++26
> (`is_within_lifetime`)

## Mental model

A `constexpr` function can run in two worlds. During constant evaluation it
must stay inside the rules (no `reinterpret_cast`, no intrinsics, no I/O); at
run time it may use whatever is fastest. These tools let one function pick a
branch per world:

```
constexpr int f(int x) ──┬── constant evaluation ──► portable, constexpr-safe path
                         └── run time ─────────────► intrinsics, SIMD, logging...
```

## Declaration

```cpp nocompile
constexpr bool std::is_constant_evaluated() noexcept;          // C++20
if consteval { /* compile time */ } else { /* run time */ }    // C++23, a statement
template<class T>
consteval bool std::is_within_lifetime(const T* p) noexcept;   // C++26
```

## When to use it

- **`if consteval`** (C++23) for new code: it's a language statement, so the
  compile-time branch may call `consteval` functions and you can't misuse it
  with `if constexpr`. See [selection statements](../language/statements/selection.md).
- **`std::is_constant_evaluated()`** when you must support C++20, or need the
  answer as a `bool` value.
- **`std::is_within_lifetime(p)`** inside a compile-time branch, to ask whether
  `*p` is alive, for example which union member is active.

## Examples

### `std::is_constant_evaluated` (C++20)

It returns `true` when the call is part of a constant evaluation, and `false`
at run time:

```cpp
constexpr int where() {
    if (std::is_constant_evaluated()) return 1;   // compile time
    else                              return 2;   // run time
}

void demo() {
    constexpr int c = where();   // 1: a constexpr initializer is constant-evaluated
    int r = where();             // 2: an ordinary local runs at run time
    static int s = where();      // 1: static storage tries constant initialization first
    use(c, r, s);
}
```

### `if consteval` (C++23)

The compile-time branch of `if consteval` is an immediate-function context, so
it can call a `consteval` function with a non-constant parameter:

```cpp
consteval int sq(int x) { return x * x; }

constexpr int sq_or(int x) {
    if consteval { return sq(x); }    // OK: x is a constant here
    else         { return x * x; }
}
static_assert(sq_or(4) == 16);
```

A plain `if (std::is_constant_evaluated())` doesn't give that, so the same call
is an error:

```cpp compile-fail="consteval"
consteval int sq(int x) { return x * x; }
constexpr int f(int x) {
    if (std::is_constant_evaluated()) return sq(x);   // error: x isn't a constant expression
    return x * x;
}
```

### `std::is_within_lifetime` (C++26): which union member is active?

Reading an inactive union member during constant evaluation is an error, so a
compact optional can't test its own state by reading. `is_within_lifetime`
answers without reading. It's `consteval`, so it goes in an `if consteval`
branch. Of the local toolchains only Clang 22 (libc++) has it
(`__cpp_lib_is_within_lifetime == 202306L`, P2641); GCC 15 and 16 don't.

```cpp needs=is-within-lifetime
union OptBool {
    char none = 2;   // sentinel byte: no value
    bool b;
    constexpr OptBool() = default;
    constexpr OptBool(bool v) : b(v) {}
    constexpr bool has_value() const {
        if consteval { return std::is_within_lifetime(&b); }   // is b the active member?
        else         { return none != 2; }                     // run time: check the byte
    }
};
static_assert(!OptBool{}.has_value());
static_assert(OptBool{false}.has_value());   // engaged, even though the value is false
```

## Gotchas

- **`if constexpr (std::is_constant_evaluated())` is always true.** The
  condition of `if constexpr` is itself constant-evaluated, so the run-time
  branch is discarded. Use plain `if`, or better, `if consteval`.
- **Constant-evaluated doesn't mean `constexpr` variables only.** Template
  arguments, array bounds, `static_assert`, and the initializers of
  `constinit`, static-storage and `const` integral variables are tried as
  constant evaluation too, so `where()` above returns 1 for a global
  `int g = where();`. The optimizer folding a run-time call doesn't count: that
  call still sees `false`.
- **Keep both branches equivalent.** The compiler may pick either world, and
  code that behaves differently is hard to test. Unit-test the run-time branch
  and `static_assert` the compile-time one.
- **`is_within_lifetime` is compile-time only.** It's `consteval`; there's no
  run-time answer to "is this object alive?".

## See also

- [std::is_constant_evaluated — cppreference](https://en.cppreference.com/w/cpp/types/is_constant_evaluated)
- [if consteval — cppreference](https://en.cppreference.com/w/cpp/language/if#Consteval_if)
- [std::is_within_lifetime — cppreference](https://en.cppreference.com/w/cpp/types/is_within_lifetime)
- [P2641R4: Checking if a union alternative is active](https://wg21.link/P2641R4)
- [P1938R3: if consteval](https://wg21.link/P1938R3)
- [Constant expressions](../language/expressions/constant-expressions.md) — `constexpr`, `consteval`, `constinit`
- [Type traits](type-traits.md)
