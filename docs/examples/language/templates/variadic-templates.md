# Variadic templates

> Templates that accept any number of type/value arguments via **parameter packs**.

## Parameter packs

```cpp
template<class... Ts>            // Ts is a template type parameter pack
struct Tuple;                    // (this is roughly how std::tuple is declared)

template<class... Args>
void call_all(Args&&... args);   // args is a function parameter pack

sizeof...(Args);                 // number of types in the pack
sizeof...(args);                 // number of arguments
```

## Expansion

The `...` expands a pack wherever a pattern repeats:

```cpp
template<class... Args>
void forward_to(Args&&... args) {
    sink(std::forward<Args>(args)...);   // perfect-forward every argument
}

template<class... Ts>
auto make_vec(Ts... xs) {
    return std::vector{xs...};            // expand into an initializer list
}
```

The pattern before `...` is repeated for each pack element, comma-separated.

## Fold expressions (C++17) — reduce a pack

The clean alternative to recursion:

```cpp
template<class... T> auto sum(T... xs)  { return (xs + ...); }         // x0 + (x1 + (...))
template<class... T> bool all(T... bs)  { return (... && bs); }         // left fold
template<class... T> void print(const T&... xs) {
    ((std::cout << xs << ' '), ...);     // comma-fold: do something per element
}
```

Four forms: `(pack op ...)`, `(... op pack)`, and the two with an init value
`(pack op ... op init)` / `(init op ... op pack)`.

## Recursive expansion (pre-fold, still useful)

```cpp
void print() {}                                   // base case
template<class T, class... Rest>
void print(const T& first, const Rest&... rest) { // peel one off
    std::cout << first << ' ';
    print(rest...);                                // recurse on the tail
}
```

Folds replaced most of this, but recursion is still needed when each step does
something stateful or type-dependent.

## Gotchas

- **`...` placement defines the pattern.** `f(g(args)...)` calls `g` on each arg;
  `f(g(args...))` passes the whole pack to one `g`. A misplaced `...` is a
  frequent compile error.
- **Empty packs.** `sizeof...(args) == 0` is legal; make sure fold/recursion base
  cases handle zero elements (a unary fold over `&&`/`||`/`,` has defined identity;
  others don't).
- **Forward packs with `std::forward<Args>(args)...`** to preserve value
  categories — see [value categories](../expressions/value-categories.md).
- **Prefer fold expressions to recursion** (C++17+) for reductions — far less
  boilerplate and faster to compile.

## See also

- [Parameter pack — cppreference](https://en.cppreference.com/w/cpp/language/parameter_pack)
- [Fold expressions — cppreference](https://en.cppreference.com/w/cpp/language/fold)
