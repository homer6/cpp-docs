# Enumerations

> Named integer constants grouped into a type. Prefer the scoped `enum class` —
> it's type-safe and doesn't leak names.

## Scoped enums (`enum class`) — the default choice

```cpp
enum class Color { red, green, blue };       // values: 0, 1, 2

Color c = Color::green;                        // must qualify with Color::
// int n = c;                                  // ERROR: no implicit conversion (good!)
int n = static_cast<int>(c);                   // explicit if you really want the number

enum class Status : std::uint8_t { ok, warn, error };   // choose the underlying type
```

Scoped enums: names are scoped to the enum (no global pollution), and there's
**no implicit conversion** to/from `int` — so you can't accidentally compare a
`Color` to a `Status`.

## Unscoped enums (plain `enum`) — legacy

```cpp
enum Direction { north, south, east, west };  // names leak into the enclosing scope
Direction d = north;                            // unqualified — and...
int x = north;                                  // ...implicitly converts to int

enum Flags : unsigned { a = 1, b = 2, c = 4 }; // explicit values; underlying type (C++11)
```

Unscoped enums implicitly convert to their integer value (handy for bit flags,
risky everywhere else). Prefer `enum class` unless you specifically want this.

## Using scoped enum values conveniently (C++20)

```cpp
void f(Color c) {
    using enum Color;            // C++20: bring red/green/blue into scope here
    if (c == red) { /* no Color:: needed in this scope */ }
}
```

## Helpers

```cpp
std::to_underlying(Status::error);   // C++23: enum → underlying integer, safely
                                     // (was: static_cast<std::underlying_type_t<Status>>(...))
```

[Reflection](../expressions/reflection.md) (C++26) enables generic enum↔string
without hand-written tables (`std::meta::enumerators_of`).

## Gotchas

- **Prefer `enum class`.** Unscoped enums leak their enumerator names into the
  surrounding scope and implicitly convert to `int`, causing collisions and silent
  mixing of unrelated enums.
- **Scoped enums don't implicitly convert** — that's the safety. Use
  `static_cast` or `std::to_underlying` (C++23) when you need the number.
- **Set the underlying type for ABI/serialization** (`: std::uint8_t`) so the size
  is fixed and known.
- **Enums aren't bit-flag types out of the box** — for `enum class` flags you must
  define the bitwise operators yourself (or keep flags as an unscoped enum / use a
  flags library).

## See also

- [Enumerations — cppreference](https://en.cppreference.com/w/cpp/language/enum)
- [std::to_underlying — cppreference](https://en.cppreference.com/w/cpp/utility/to_underlying)
