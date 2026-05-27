# Concepts library

> Named compile-time requirements on types, used to constrain templates and give
> readable errors. **Header:** `<concepts>` · **Since:** C++20

Mirrors [cppreference's concepts library](https://en.cppreference.com/w/cpp/concepts).

| Page | What |
|---|---|
| [Standard concepts](standard-concepts.md) | the built-in concepts + how to write/use your own |

## The one-line pitch

A **concept** is a compile-time predicate on types. Constrain a template with one
and you get (a) clear intent at the declaration, (b) crisp error messages when a
caller violates it, and (c) cleaner overloading than SFINAE.

```cpp
#include <concepts>

template<std::integral T>           // only integer types
T half(T x) { return x / 2; }

half(10);     // ok
half(3.14);   // error: "3.14 does not satisfy std::integral" — not a wall of template noise
```

Concepts are the modern replacement for most [`enable_if`/SFINAE](../metaprogramming/type-traits.md)
tricks, and the vocabulary the [ranges](../ranges/README.md) and
[iterators](../iterators/README.md) libraries are built on.
