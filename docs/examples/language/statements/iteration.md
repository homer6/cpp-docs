# Iteration statements

> The loops — and why range-`for` should be your default.

## range-for — prefer this

```cpp body
std::vector<int> container{1, 2, 3}, small_trivials{4, 5, 6};
std::map<std::string, int> m{{"one", 1}, {"two", 2}};
auto make = [] { return std::vector<int>{1, 2, 3}; };

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

```cpp body
int n = 3;
bool cond = false;

for (int i = 0; i < n; ++i) { /* when you need the index */ }

while (cond) { /* zero or more times */ }

do { /* at least once */ } while (cond);
```

## break / continue and loop control

```cpp body
std::vector<int> xs{1, -2, 3, 500, 4};
auto skip = [](int x) { return x < 0; };
auto done = [](int x) { return x > 100; };

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

```cpp body needs=expansion-statements
std::tuple<int, std::string, double> t{1, "two", 3.0};

template for (const auto& elem : t) {         // unrolls: one body per element
    std::println("{}", elem);                  // elem's type differs each iteration
}
```

The loop variable is `constexpr` only when the range itself is a constant
expression; over a runtime tuple like `t`, declare it `auto&`/`const auto&`.
GCC 16 implements expansion statements with plain `-std=c++26`; GCC 15 and
Clang 22 don't yet.

This is what finally lets you iterate a heterogeneous tuple (or a struct's
reflected members) without recursion or `std::apply` tricks — see
[reflection](../expressions/reflection.md).

## Prefer algorithms over hand loops

Many loops are better expressed as a named [algorithm](../../algorithms/README.md)
or a [ranges](../../ranges/README.md) pipeline:

```cpp body
std::vector<int> xs{1, 2, 3, 4};
auto is_bad = [](int n) { return n < 0; };

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
