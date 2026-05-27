# Declarations, `auto` & type deduction

> Naming things with types — and letting the compiler deduce the type for you.

## auto — deduce the type from the initializer

```cpp
auto i = 42;                 // int
auto d = 3.14;               // double
auto s = "hi"s;              // std::string
auto& r = x;                 // reference
const auto& cr = x;          // const reference (no copy)
auto* p = &x;                // pointer

for (const auto& [k, v] : m) {}   // with structured bindings
auto lambda = [](int x){ return x*x; };   // lambdas have no spellable type — auto is required
```

`auto` strips top-level `const` and references **unless you write them**:
`auto x = cref;` copies (drops const/ref); `auto& x = cref;` keeps them.

## decltype — the declared type of an expression

```cpp
int x = 0;
decltype(x)   a;        // int
decltype((x)) b = x;    // int&  — extra parens → the expression is an lvalue
decltype(auto) f();      // deduce return type *preserving* ref/const (vs plain auto)
```

`decltype(auto)` is the tool for perfect-forwarding return types (keeps
references that `auto` would strip).

## Trailing return types & deduced returns

```cpp
auto add(int a, int b) -> int;          // trailing return type
auto multiply(int a, int b) { return a*b; }   // return type deduced (C++14)

template<class T, class U>
auto sum(T t, U u) -> decltype(t + u);   // return type depends on params
```

## Structured bindings (C++17)

Decompose a tuple/pair/array/aggregate into named pieces:

```cpp
std::pair<int, std::string> p{1, "one"};
auto [id, name] = p;                       // id=1, name="one"
auto& [a, b] = some_struct;                 // bind by reference to mutate
auto [x, y, z] = std::array{1, 2, 3};
```

## Gotchas

- **`auto` drops references and top-level const.** `auto x = vec[0];` copies; use
  `auto&` / `const auto&` to bind without copying.
- **`auto&&` is a forwarding reference** in deduced context (binds to lvalue or
  rvalue) — not the same as a plain rvalue ref. See
  [value categories](../expressions/value-categories.md).
- **`decltype(x)` vs `decltype((x))` differ:** the parenthesized form yields a
  reference type because `(x)` is an lvalue expression. Easy to trip over.
- **Don't overuse `auto` where the type aids readability** — `auto total = 0;`
  hides that it's an `int` (and can cause integer/float surprises). It shines for
  iterators, lambdas, and verbose template types.
- **Almost-Always-Auto vs clarity** is a style call; be consistent with the
  codebase.

## See also

- [auto specifier — cppreference](https://en.cppreference.com/w/cpp/language/auto)
- [decltype — cppreference](https://en.cppreference.com/w/cpp/language/decltype)
- [Structured bindings — cppreference](https://en.cppreference.com/w/cpp/language/structured_binding)
