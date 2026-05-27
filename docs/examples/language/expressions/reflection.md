# Reflection (C++26)

> Inspect and act on the program's own entities at compile time: turn a type,
> function, or namespace into a value, query it, and splice it back into code.
> **Operator:** `^^` · **Library:** [`<meta>`](../../metaprogramming/reflection.md) ·
> **Since:** C++26

## Mental model

Reflection has two language pieces, both `consteval`:

1. **The reflection operator `^^`** — turns an entity into a first-class value of
   type `std::meta::info`. `^^int`, `^^std::vector`, `^^foo`, `^^::` (the global
   namespace) all produce reflections.
2. **Splicers `[: r :]`** — the inverse: turn a `std::meta::info` value *back* into
   a grammatical construct (a type, an expression, a template argument).

Between the two sits the [`<meta>` library](../../metaprogramming/reflection.md),
whose `consteval` functions query and transform `info` values.

```
   ^^X    : entity ─────▶ std::meta::info        (reflect)
  [: r :] : std::meta::info ─────▶ entity        (splice back)
```

## Reflecting and splicing

```cpp
#include <meta>

constexpr std::meta::info r = ^^int;     // reflect the type int
using T = [: r :];                        // splice it back → T is int
T x = 0;                                  // x is an int

constexpr auto rv = ^^std::vector;        // reflect a template
constexpr auto fn = ^^std::sqrt;          // reflect a function
constexpr auto ns = ^^::;                 // reflect the global namespace
```

Splice forms: `[: r :]` for a value/type, `typename[: r :]` where a type is
required, `template[: r :]` for a template name.

## The classic example: iterate a struct's members

```cpp
#include <meta>

struct Point { int x; int y; int z; };

template<class T>
constexpr void dump(const T& obj) {
    // expand over the reflected non-static data members (with `template for`, C++26):
    template for (constexpr auto member :
                  std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
        std::println("{} = {}", std::meta::identifier_of(member), obj.[:member:]);
        //                       └ the member's name        └ splice to access obj.<member>
    }
}

dump(Point{1, 2, 3});      // x = 1 / y = 2 / z = 3 — generated at compile time
```

This is what previously required macros or external code generators: enumerate
members, get their names, access them — all in the language.

## What it unlocks

- Automatic serialization / printing / hashing of any struct.
- Enum ↔ string conversion without hand-maintained tables
  (`std::meta::enumerators_of`).
- Generating types (`std::meta::define_aggregate`) and calling templates by
  reflection (`std::meta::substitute`).

## Gotchas

- **Everything is `consteval`** — reflection happens entirely at compile time;
  `std::meta::info` values don't exist at runtime.
- **The operator is `^^`, not `^`** (it changed late in design to avoid clashing
  with the Objective-C/blocks `^`). `^^::` reflects the global namespace.
- **Splicers are context-sensitive:** use `typename[: r :]` in type position and
  `template[: r :]` in template-name position; a bare `[: r :]` is for
  value/expression contexts.
- **Brand-new (C++26), very limited support.** Feature-test with the library
  facilities; early compilers shipped it experimentally (often via a `-freflection`
  flag and the P2996 reference implementation).
- The reflection *functions* live in the library — see
  [`<meta>`](../../metaprogramming/reflection.md).

## See also

- [Metaprogramming library / `<meta>` — cppreference](https://en.cppreference.com/w/cpp/meta)
- [`<meta>` reflection library (this repo)](../../metaprogramming/reflection.md)
