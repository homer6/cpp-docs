# Coroutines

> Functions that can **pause** and **resume**, keeping their state between
> suspensions — the basis for lazy generators and async code. **Since:** C++20

## Mental model

A function becomes a coroutine if its body uses any of `co_await`, `co_yield`, or
`co_return`. Instead of running to completion, it can suspend, hand control back
to the caller, and later resume where it left off. Its local state lives in a
heap-allocated **coroutine frame**.

```
 caller ──resume──▶ coroutine
        ◀──suspend── (co_yield / co_await)   ← state preserved across the gap
```

## The three keywords

```cpp
co_yield value;   // produce a value and suspend (generators)
co_await expr;    // suspend until `expr` (an awaitable) is ready, then resume with its result
co_return value;  // finish the coroutine, producing a final value
```

## The catch: C++20 gives mechanism, not types

C++20 standardized the *language* machinery (the `promise_type` protocol,
`std::coroutine_handle`, awaiter interface) but **not** ready-made coroutine
types — you had to write your own `promise_type`, or use a library. C++23 finally
adds a usable one: `std::generator`.

## std::generator (C++23) — the easy win

```cpp
#include <generator>

std::generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;                 // produce a, then pause
        std::tie(a, b) = std::tuple{b, a + b};
    }
}

for (int x : fibonacci() | std::views::take(10))   // lazy: only 10 values computed
    /* 0 1 1 2 3 5 8 13 21 34 */;
```

`std::generator<T>` is a lazy, single-pass range — perfect for producing
sequences on demand.

## What awaiting looks like (sketch)

```cpp
Task<int> fetch() {
    int a = co_await read_async();      // suspend until read completes, resume with the value
    int b = co_await read_async();
    co_return a + b;
}
```

Here `Task` is a user/library type whose `promise_type` defines how the coroutine
suspends and delivers its result. The C++26
[execution library](../../execution/README.md) integrates senders with
`co_await`.

## Gotchas

- **C++20 alone has no usable coroutine type** — you write a `promise_type` or use
  a library (cppcoro, the standard `std::generator` in C++23, or
  `std::execution`). Don't expect `co_yield` to "just work" without a return type
  that supports it.
- **The coroutine frame is heap-allocated** (unless the compiler elides it). For
  hot, fine-grained coroutines, watch allocation cost.
- **Dangling by reference.** A coroutine that captures references to caller locals
  (e.g. a parameter passed by reference) can dangle after suspension — be careful
  what the frame refers to; prefer by-value parameters for lazily-consumed
  generators.
- **`std::generator` is single-pass** — you can't restart or iterate it twice.
- **Don't `co_return` a reference to a frame local** — the frame is destroyed when
  the coroutine finishes.

## See also

- [Coroutines — cppreference](https://en.cppreference.com/w/cpp/language/coroutines)
- [std::generator (C++23) — cppreference](https://en.cppreference.com/w/cpp/coroutine/generator)
- [std::coroutine_handle — cppreference](https://en.cppreference.com/w/cpp/coroutine/coroutine_handle)
