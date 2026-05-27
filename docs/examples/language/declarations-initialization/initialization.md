# Initialization

> C++ has many initialization forms with subtly different rules. Here's the map,
> and when to use `{}` vs `()`.

## The forms

```cpp
int a = 5;          // copy initialization
int b(5);           // direct initialization
int c{5};           // direct-list (brace) initialization
int d = {5};        // copy-list initialization
int e{};            // value initialization → 0
int f;              // default initialization → INDETERMINATE for locals (garbage!)

std::string s{"hi"};
std::vector<int> v{1, 2, 3};        // initializer_list
auto w = std::vector<int>(5, 0);    // 5 zeros — see the () vs {} trap below
```

## What the categories mean

- **value-initialization** (`T x{}` / `T()`): zeroes scalars, calls the default
  constructor for classes. Use it to avoid garbage.
- **default-initialization** (`T x;`): for class types, default-constructs; for
  scalars **as locals**, leaves indeterminate — a bug source.
- **list-initialization** (`{}`): also **forbids narrowing** and prefers an
  `initializer_list` constructor when one exists.
- **aggregate initialization**: brace-init the members of a struct/array with no
  user constructors.

## Aggregates & designated initializers (C++20)

```cpp
struct Config { int width = 80; int height = 25; bool wrap = false; };

Config a{100, 40};                       // positional aggregate init
Config b{.width = 100, .wrap = true};    // designated initializers (C++20): height stays 25
```

## The `()` vs `{}` trap

```cpp
std::vector<int> v1(5, 0);    // () → vector(size=5, value=0)  → {0,0,0,0,0}
std::vector<int> v2{5, 0};    // {} → initializer_list         → {5, 0}
```

Braces prefer the `initializer_list` constructor, so `{5, 0}` is *two elements*,
not "5 copies of 0." This is the most cited brace-init surprise.

## Gotchas

- **Always initialize locals.** `int x;` is indeterminate; reading it is UB.
  Prefer `int x{};` for a guaranteed zero.
- **`{}` bans narrowing** (`int x{3.9};` is an error) — a feature; prefer it to
  catch truncation. But...
- **...`{}` prefers `initializer_list` constructors**, which changes meaning for
  containers (`vector<int>{5,0}` ≠ `vector<int>(5,0)`). Use `()` when you mean the
  sizing/positional constructor.
- **Designated initializers (C++20) must be in declaration order** and only work
  on aggregates — you can't reorder or skip-then-go-back.
- **`= {}` vs `{}`:** copy-list vs direct-list init differ mainly around
  `explicit` constructors (copy-list can't call them).

## See also

- [Initialization — cppreference](https://en.cppreference.com/w/cpp/language/initialization)
- [List initialization — cppreference](https://en.cppreference.com/w/cpp/language/list_initialization)
- [Aggregate initialization — cppreference](https://en.cppreference.com/w/cpp/language/aggregate_initialization)
