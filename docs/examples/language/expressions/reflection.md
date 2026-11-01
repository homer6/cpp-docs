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

```cpp needs=reflection
#include <meta>

int twice(int);

constexpr std::meta::info r = ^^int;     // reflect the type int
using T = [: r :];                        // splice it back → T is int
T x = 0;                                  // x is an int

constexpr auto rv = ^^std::vector;        // reflect a template
constexpr auto fn = ^^twice;              // reflect a function (not an overload set:
                                          // ^^std::sqrt is ill-formed)
constexpr auto ns = ^^::;                 // reflect the global namespace
```

Splice forms: `[: r :]` for a value/type, `typename[: r :]` where a type is
required, `template[: r :]` for a template name.

## The classic example: iterate a struct's members

```cpp needs=reflection
#include <meta>
#include <print>

struct Point { int x; int y; int z; };

template<class T>
void dump(const T& obj) {
    // expand over the reflected non-static data members (with `template for`, C++26).
    // nonstatic_data_members_of returns a std::vector, which can't outlive constant
    // evaluation; std::define_static_array turns it into a static array to iterate.
    template for (constexpr auto member :
                  std::define_static_array(
                      std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()))) {
        std::println("{} = {}", std::meta::identifier_of(member), obj.[:member:]);
        //                       └ the member's name        └ splice to access obj.<member>
    }
}

int main() {
    dump(Point{1, 2, 3});  // x = 1 / y = 2 / z = 3 — the loop is expanded at compile time
}
```

This is what previously required macros or external code generators: enumerate
members, get their names, access them — all in the language.

## Annotations (C++26)

An **annotation** `[[=expr]]` attaches a constant value to a declaration, for
reflection to read back (P3394). It looks like an attribute but means nothing to
the compiler on its own: it's data for your own `consteval` code, such as a
serializer's field name or a validator's range.

```cpp needs=reflection
#include <meta>

struct Range { int lo, hi; };               // annotation values must be structural types

struct Config {
    [[=Range{1, 65535}]] int port;          // one annotation
    [[=1, =2]]           int level;         // several in one list: each starts with =
                         int plain;         // none
};

namespace meta = std::meta;
static_assert(meta::annotations_of(^^Config::level).size() == 2);   // in declaration order
static_assert(meta::annotations_of(^^Config::plain).empty());

// constant_of turns an annotation into a reflection of its value; splice or extract it:
static_assert([: meta::constant_of(meta::annotations_of(^^Config::level)[1]) :] == 2);
// annotations_of_with_type filters by type; extract<T> reads the value:
constexpr auto r = meta::annotations_of_with_type(^^Config::port, ^^Range)[0];
static_assert(meta::extract<Range>(r).hi == 65535);
```

Types, aliases, variables, functions, parameters, namespaces, enumerators, base
specifiers and non-static data members can all be annotated.

## What it unlocks

- Automatic serialization / printing / hashing of any struct, steered by
  annotations (`[[=Name{...}]]`, `[[=skip]]`).
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
- **Annotations aren't attributes.** One `[[...]]` list holds either attributes or
  annotations: `[[nodiscard, =1]]` is an error ("mixing annotations and attributes
  in the same list"); write `[[nodiscard]] [[=1]]`. Every annotation is distinct, so
  `[[=42, =42]]` gives two, and annotations on redeclarations add up.
- **Brand-new (C++26), one local compiler.** Only GCC 16 implements P2996, and only
  with `-std=c++26 -freflection` (`__cpp_impl_reflection` and
  `__cpp_lib_reflection` are both `202603L` there). GCC 15 and Clang 22 have no
  `<meta>`; Clang's reflection work lives in a separate upstream branch, not in
  the Homebrew Clang 22 release. The examples here were checked with GCC 16.
- The reflection *functions* live in the library — see
  [`<meta>`](../../metaprogramming/reflection.md).

## See also

- [Metaprogramming library / `<meta>` — cppreference](https://en.cppreference.com/w/cpp/meta)
- [Annotations (C++26) — cppreference](https://en.cppreference.com/w/cpp/language/annotations)
- [`<meta>` reflection library (this repo)](../../metaprogramming/reflection.md)
- [P2996 Reflection for C++26](https://wg21.link/P2996) ·
  [P3394 Annotations for reflection](https://wg21.link/P3394)
