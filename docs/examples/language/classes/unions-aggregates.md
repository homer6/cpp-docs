# Unions & aggregates

> Plain data structures (aggregates) and the raw `union` (and why
> [`std::variant`](../../utilities/variant.md) usually replaces it).

## Aggregates

An **aggregate** is a class/struct with no user-declared constructors, no private
non-static data, no virtuals — so it can be brace-initialized member-by-member:

```cpp
struct Point { int x; int y; };           // aggregate
struct Config { int w = 80; bool wrap = false; };   // default members allowed (C++14)

Point p{1, 2};                             // aggregate initialization
Config c{.w = 100, .wrap = true};          // designated initializers (C++20)
int arr[3]{1, 2, 3};                        // arrays are aggregates too
auto [x, y] = p;                            // structured bindings work on aggregates
```

Aggregates are the right choice for plain bundles of data with no invariant.

## Unions

A `union` stores **one** of its members at a time, all sharing the same memory.
It does **not** track which member is active — that's on you.

```cpp
union Value {
    int    i;
    double d;
    char   bytes[8];
};
Value v;
v.i = 42;          // active member is i
v.d = 3.14;        // now d — reading v.i after this is UB
```

Reading a member other than the last one written is undefined behavior (no type
punning via unions in standard C++ — use [`std::bit_cast`](../../numerics/bit.md)).

## Prefer std::variant

`std::variant<int, double, std::string>` is a **type-safe union**: it knows which
alternative is active, checks access, and runs the right destructor.

```cpp
std::variant<int, double, std::string> v = 3.14;   // tracks the active type
std::get<double>(v);                                // checked; throws on mismatch
```

Reach for raw `union` only for low-level/ABI reasons; otherwise use `variant`.

## Gotchas

- **A raw `union` doesn't know its active member** and won't destroy non-trivial
  members for you — a union containing a `std::string` needs manual lifetime
  management (placement new / explicit destructor calls). Almost always, use
  [`std::variant`](../../utilities/variant.md) instead.
- **Type punning through a union is UB** in C++ (unlike C). Use `std::bit_cast`
  (trivially-copyable) for bit reinterpretation.
- **Aggregate rules tightened over versions.** Adding a user constructor (even
  `= default` in some forms) or private data makes a type a non-aggregate, so
  brace-member-init stops working — keep aggregates plain.
- **Designated initializers (C++20) need declaration order** and only work on
  aggregates.

## See also

- [Aggregate initialization — cppreference](https://en.cppreference.com/w/cpp/language/aggregate_initialization)
- [Union — cppreference](https://en.cppreference.com/w/cpp/language/union)
- [std::variant — cppreference](https://en.cppreference.com/w/cpp/utility/variant)
