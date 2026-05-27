# Static reflection (`<meta>`)

> The library half of [reflection](../language/expressions/reflection.md): a
> single value type `std::meta::info` plus a large set of `consteval` functions to
> query and synthesize program entities. **Header:** `<meta>` ·
> **Namespace:** `std::meta` · **Since:** C++26

## Mental model

The language gives you the `^^` operator (entity → `info`) and splicers
(`[: info :]` → entity). `<meta>` is everything in between: functions that take
and return `std::meta::info`, all `consteval`. You compose them to inspect types,
list members, transform types, and even generate new ones.

```cpp
using info = decltype(^^::);     // std::meta::info is literally the type of a reflection
```

## Querying entities

```cpp
#include <meta>
namespace meta = std::meta;

constexpr meta::info t = ^^std::string;

meta::identifier_of(t);        // "string"        — the name
meta::type_of(r);              // the type of a reflected variable/member
meta::parent_of(t);            // enclosing namespace/class
meta::is_type(t);              // category predicates: is_type / is_function /
meta::is_function(r);          //   is_namespace / is_class_type / is_enum_type / ...
meta::is_public(m);            // access, virtual, const, static, ... predicates
```

## Listing members, bases, enumerators

```cpp
constexpr auto ctx = meta::access_context::current();

meta::members_of(^^T, ctx);                  // all members
meta::nonstatic_data_members_of(^^T, ctx);   // just the data members
meta::bases_of(^^T, ctx);                     // base classes
meta::enumerators_of(^^SomeEnum);             // enum constants

// layout:
meta::offset_of(member);   meta::size_of(t);   meta::alignment_of(t);
```

### Enum ↔ string, generated

```cpp
template<class E>
constexpr std::string_view to_string(E value) {
    template for (constexpr auto e : std::meta::enumerators_of(^^E))
        if (value == [: e :])                       // splice the enumerator value
            return std::meta::identifier_of(e);      // its name
    return "<unknown>";
}
```

## Type transformations (reflection-flavored type traits)

`<meta>` mirrors `<type_traits>` as functions on `info`:

```cpp
meta::add_const(t);  meta::remove_reference(t);  meta::decay(t);
meta::is_same_type(a, b);  meta::is_base_of_type(b, d);  meta::is_convertible_type(s, d);
```

## Generating and substituting

```cpp
// instantiate a template by reflection:
constexpr meta::info v = meta::substitute(^^std::vector, {^^int});   // reflects std::vector<int>
using V = [: v :];                                                    // → std::vector<int>

// extract a compile-time value from a reflection:
constexpr int n = meta::extract<int>(some_constant_reflection);

// build an aggregate type at compile time:
meta::define_aggregate(^^Incomplete, { meta::data_member_spec(^^int, {.name = "x"}),
                                        meta::data_member_spec(^^int, {.name = "y"}) });
```

## Gotchas

- **All `consteval`, all compile time.** `std::meta::info` and these functions
  cannot appear at runtime — results must feed constant evaluation / code
  generation.
- **`members_of` and friends take an `access_context`** (C++26) to decide what's
  visible — pass `std::meta::access_context::current()` for the normal "what can I
  see here" rule.
- **It's the function layer; the operator/splicers are the language layer** — you
  need both (see [reflection (language)](../language/expressions/reflection.md)).
- **Reflection errors throw `std::meta::exception`** during constant evaluation
  (e.g. querying something invalid).
- **C++26, early/experimental support.** Expect flags, partial implementations,
  and churn; the P2996 reference implementation is the proving ground.

## See also

- [Metaprogramming library / `<meta>` — cppreference](https://en.cppreference.com/w/cpp/meta)
- [Standard library header `<meta>` — cppreference](https://en.cppreference.com/w/cpp/header/meta)
- [Reflection (language operator & splicers)](../language/expressions/reflection.md)
