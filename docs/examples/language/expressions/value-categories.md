# Value categories

> Every expression has a **type** *and* a **value category**. Categories are what
> make move semantics and perfect forwarding work.

## The taxonomy

```
            expression
           /          \
       glvalue       rvalue
       /     \       /     \
   lvalue   xvalue        prvalue
```

- **lvalue** — has identity, can't be moved from. A named variable, `*p`, `a[i]`.
- **prvalue** — a pure value with no identity yet. A literal `42`, `a + b`, a
  function returning by value, a temporary.
- **xvalue** ("eXpiring") — has identity *and* is movable. The result of
  `std::move(x)` or a function returning `T&&`.
- **glvalue** = lvalue or xvalue (has identity). **rvalue** = prvalue or xvalue
  (movable).

## Why it matters

Overloads bind based on category, which is how the library knows when it may
steal resources:

```cpp
void sink(const std::string&);    // binds to lvalues (and rvalues as fallback): copy
void sink(std::string&&);          // binds to rvalues: may move

std::string s = "hi";
sink(s);              // lvalue  → const& overload (copy)
sink(std::move(s));   // xvalue  → && overload (move) — s is now moved-from
sink("temp");          // prvalue → && overload (move)
```

`std::move(x)` doesn't move anything — it's a **cast to an xvalue** that *permits*
a move overload to be selected. See [`std::move`](../../utilities/utility.md).

## Reference binding rules

```cpp
int&        a = x;             // lvalue ref: binds to lvalues only
const int&  b = 42;            // const lvalue ref: binds to anything (extends temp lifetime)
int&&       c = 42;            // rvalue ref: binds to rvalues only
auto&&      d = anything;      // forwarding ref (in deduced context): binds to either
```

## Gotchas

- **`std::move` on an lvalue you still need is a bug** — the move overload may
  gut it. Only move from things you're done with.
- **A named rvalue reference is itself an lvalue.** Inside `void f(T&& x)`, `x`
  has a name, so it's an lvalue — you must `std::move(x)` (or `std::forward`) to
  pass it onward as movable.
- **`auto&&` / `T&&` in a deduced context is a *forwarding* reference**, not a
  plain rvalue reference — pair it with [`std::forward`](../../utilities/utility.md).
- **Returning `std::move(local)` can disable copy elision (NRVO)** — usually just
  `return local;`.

## See also

- [Value categories — cppreference](https://en.cppreference.com/w/cpp/language/value_category)
- [Reference declaration — cppreference](https://en.cppreference.com/w/cpp/language/reference)
