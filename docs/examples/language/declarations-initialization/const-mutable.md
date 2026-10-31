# `const`, `constexpr`, `mutable` & `volatile`

> The qualifiers that control mutability and where it can be bent.

## const — "I won't modify this"

```cpp
int x = 0;
const int max = 100;            // a constant
const int* p1;                  // pointer to const int (can't change *p1)
int* const p2 = &x;             // const pointer (can't repoint p2)
const int* const p3 = &x;       // both

void print(const std::string& s);   // const ref param: no copy, no mutation

struct Counter {
    int value() const;          // const member function: callable on const objects, won't mutate *this
};
```

Read pointer declarations right-to-left: `int* const` = "const pointer to int".

## const-correctness

Mark everything that doesn't mutate as `const` — parameters, member functions,
locals. It documents intent, enables optimizations, and lets your API be called
on `const` objects. A `const` member function can only call other `const` members
and can't modify data members (except `mutable` ones).

## mutable — escape hatch in a const object

```cpp
struct Result { int value; };
Result compute();               // the expensive call being cached

class Cache {
    mutable std::mutex m_;          // lockable even from const methods
    mutable std::optional<Result> cached_;
public:
    Result get() const {            // logically const, physically caches
        std::lock_guard lk{m_};
        if (!cached_) cached_ = compute();
        return *cached_;
    }
};
```

`mutable` says "this member isn't part of the object's logical state" — typically
caches, mutexes, lazy-computed values.

## constexpr / constinit

`constexpr` is "const **and** known at compile time"; it's stronger than `const`.
See [constant expressions](../expressions/constant-expressions.md).

```cpp
int read();
const int     a = read();      // const, but value computed at runtime
constexpr int b = 5 * 5;       // const AND a compile-time constant
```

## volatile — NOT for threading

```cpp
volatile int* hardware_reg;     // tells the compiler: don't optimize away reads/writes
```

`volatile` means "this memory can change outside the program's control" (memory-
mapped hardware, signal handlers). It does **not** provide atomicity or ordering —
use [`std::atomic`](../../concurrency/atomics.md) for threads.

## Gotchas

- **`const` member functions and `mutable`:** a `const` method can modify
  `mutable` members; use that sparingly for caches/mutexes only — overuse hides
  real mutation.
- **Top-level vs low-level const.** `const int* p` (points to const) differs from
  `int* const p` (const pointer). `auto` drops top-level const; spell it out if
  you want it kept.
- **`constexpr` implies `const`** for variables, but is much stronger; don't use
  plain `const` and expect compile-time evaluation.
- **`volatile` is widely misused for concurrency** — it's wrong for that. It's for
  hardware/signal memory only.
- **Don't `const_cast` away const to mutate a truly-const object** — UB. See
  [conversions](../expressions/conversions.md).

## See also

- [const/volatile (cv) qualifiers — cppreference](https://en.cppreference.com/w/cpp/language/cv)
- [mutable specifier — cppreference](https://en.cppreference.com/w/cpp/language/cv)
