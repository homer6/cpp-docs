# Iteration statements

> The loops — and why range-`for` should be your default.

## range-for — prefer this

```cpp
for (const auto& x : container) { /* read */ }     // by const ref: no copies
for (auto& x : container) x *= 2;                   // by ref: mutate in place
for (auto x : small_trivials) { /* copy is fine for ints etc. */ }

// C++20: init-statement, to keep a helper scoped to the loop
for (auto vec = make(); const auto& x : vec) { /* ... */ }

// C++17: structured bindings over a map
for (const auto& [key, value] : m) { /* ... */ }
```

Works on anything with `begin()`/`end()` (containers, arrays, `string_view`,
ranges). It expresses "visit each element" without index bookkeeping.

## Classic for / while / do-while

```cpp
for (int i = 0; i < n; ++i) { /* when you need the index */ }

while (cond) { /* zero or more times */ }

do { /* at least once */ } while (cond);
```

## break / continue and loop control

```cpp
for (auto& x : xs) {
    if (skip(x)) continue;     // next iteration
    if (done(x)) break;        // exit loop
}
```

(There is no labeled `break`; to exit nested loops, refactor into a function and
`return`, or use a flag — see Gotchas.)

## `template for` — expansion statements (C++26)

A **compile-time** loop that unrolls over the elements of a tuple-like object, a
parameter pack, or a [reflection](../expressions/reflection.md) range. The body is
instantiated once per element, with the loop variable a *constant* each time — so
each iteration can have a different type:

```cpp
std::tuple<int, std::string, double> t{1, "two", 3.0};

template for (constexpr auto& elem : t) {     // unrolls: one body per element
    std::println("{}", elem);                  // elem's type differs each iteration
}
```

This is what finally lets you iterate a heterogeneous tuple (or a struct's
reflected members) without recursion or `std::apply` tricks — see
[reflection](../expressions/reflection.md).

## Prefer algorithms over hand loops

Many loops are better expressed as a named [algorithm](../../algorithms/README.md)
or a [ranges](../../ranges/README.md) pipeline:

```cpp
// instead of a loop with a flag:
if (std::ranges::any_of(xs, is_bad)) { /* ... */ }
auto evens = xs | std::views::filter([](int n){ return n % 2 == 0; });
```

## Gotchas

- **Range-`for` over a temporary range can dangle.** `for (auto x : f().items())`
  where `f()` returns a temporary that owns `items()` was a known pitfall; C++23
  fixed the common case, but be wary on older standards — bind the temporary
  first.
- **Use `const auto&`** in range-`for` to avoid silent per-element copies; use
  `auto&` to mutate; plain `auto` only for cheap/trivial types.
- **Don't modify a container's size while range-`for`-ing it** — inserting/erasing
  invalidates the loop's iterators (see each container's invalidation rules).
- **No labeled break.** For deep nested-loop exits, extract the loops into a
  function and `return`, which reads better than goto/flags.
- **Off-by-one and `int` index overflow** plague classic `for`. Range-`for` and
  algorithms sidestep both.

## See also

- [Range-for loop — cppreference](https://en.cppreference.com/w/cpp/language/range-for)
- [for / while / do-while — cppreference](https://en.cppreference.com/w/cpp/language/for)
