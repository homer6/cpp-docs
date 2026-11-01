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

```cpp needs=reflection
using info = decltype(^^::);     // std::meta::info is literally the type of a reflection
```

## Querying entities

```cpp needs=reflection
#include <meta>
namespace meta = std::meta;

struct S { int x; void f(); };
constexpr meta::info t = ^^std::string;
constexpr meta::info r = ^^S::x;
constexpr meta::info m = ^^S::f;

static_assert(meta::identifier_of(t) == "string");  // the name
static_assert(meta::type_of(r) == ^^int);           // the type of a reflected variable/member
static_assert(meta::parent_of(t) == ^^std);         // enclosing namespace/class
static_assert(meta::is_type(t));        // category predicates: is_type / is_function /
static_assert(meta::is_function(m));    //   is_namespace / is_class_type / is_enum_type / ...
static_assert(meta::is_public(m));      // access, virtual, const, static, ... predicates
```

## Listing members, bases, enumerators

```cpp needs=reflection
#include <meta>
namespace meta = std::meta;

struct Base {};
struct T : Base { int a; double b; };
enum class SomeEnum { x, y };

constexpr auto ctx = meta::access_context::current();

// each returns a std::vector<std::meta::info>:
static_assert(meta::members_of(^^T, ctx).size() >= 2);             // all members
static_assert(meta::nonstatic_data_members_of(^^T, ctx).size() == 2); // just the data members
static_assert(meta::bases_of(^^T, ctx).size() == 1);                // base classes
static_assert(meta::enumerators_of(^^SomeEnum).size() == 2);        // enum constants
```

Function parameters, template arguments, operators, source locations, the
predicate families and the layout queries (`offset_of`, `size_of`,
`bit_size_of`…) are on [Reflection queries](reflection-queries.md).

### Enum ↔ string, generated

```cpp needs=reflection
template<class E>
constexpr std::string_view to_string(E value) {
    // define_static_array: a constexpr vector can't outlive constant evaluation,
    // so turn the result into a static array that `template for` can expand over
    template for (constexpr auto e :
                  std::define_static_array(std::meta::enumerators_of(^^E)))
        if (value == [: e :])                       // splice the enumerator value
            return std::meta::identifier_of(e);      // its name
    return "<unknown>";
}

enum class Color { red, green };
static_assert(to_string(Color::green) == "green");
```

## Type transformations (reflection-flavored type traits)

`<meta>` mirrors `<type_traits>` as functions on `info`:

```cpp needs=reflection
namespace meta = std::meta;
struct Base {};
struct Derived : Base {};

static_assert(meta::add_const(^^int) == ^^const int);
static_assert(meta::remove_reference(^^int&) == ^^int);
static_assert(meta::decay(^^int[3]) == ^^int*);
static_assert(meta::is_same_type(^^int, ^^int));
static_assert(meta::is_base_of_type(^^Base, ^^Derived));
static_assert(meta::is_convertible_type(^^int, ^^double));
```

## Generating and substituting

```cpp needs=reflection
namespace meta = std::meta;

// instantiate a template by reflection:
constexpr meta::info v = meta::substitute(^^std::vector, {^^int});   // reflects std::vector<int>
using V = [: v :];                                                    // → std::vector<int>

// extract a compile-time value from a reflection:
constexpr meta::info some_constant_reflection = meta::reflect_constant(42);
constexpr int n = meta::extract<int>(some_constant_reflection);      // 42

// build an aggregate type at compile time (in a consteval block):
struct Incomplete;
consteval {
    meta::define_aggregate(^^Incomplete, { meta::data_member_spec(^^int, {.name = "x"}),
                                            meta::data_member_spec(^^int, {.name = "y"}) });
}
Incomplete pt{1, 2};                                                  // now complete: .x, .y
```

## Errors: `std::meta::exception` (C++26)

A meta function that can't answer (members of `int`, the name of something
nameless) **throws `std::meta::exception`** during constant evaluation (P3560).
Uncaught, that's a compile error naming the problem. Caught inside a `consteval`
function, it lets you probe and fall back:

```cpp needs=reflection
#include <meta>
namespace meta = std::meta;

consteval bool has_members(meta::info r) {
    try {
        (void)meta::members_of(r, meta::access_context::current());
        return true;
    } catch (const meta::exception& e) {    // e.what(), e.from() (the failing function),
        return false;                       // e.where() (a std::source_location)
    }
}

struct S { int x; };
static_assert(has_members(^^S));
static_assert(!has_members(^^int));        // members_of(^^int) threw; we caught it

consteval bool from_identifier_of() {
    try { (void)meta::identifier_of(^^int); }  // int has no identifier
    catch (const meta::exception& e) { return e.from() == ^^meta::identifier_of; }
    return false;
}
static_assert(from_identifier_of());
```

Cheaper than catching: test first with the matching predicate
(`has_identifier`, `has_template_arguments`, `is_complete_type`, `can_substitute`).

## Gotchas

- **All `consteval`, all compile time.** `std::meta::info` and these functions
  cannot appear at runtime — results must feed constant evaluation / code
  generation.
- **`members_of` and friends take an `access_context`** (C++26) to decide what's
  visible — pass `std::meta::access_context::current()` for the normal "what can I
  see here" rule.
- **It's the function layer; the operator/splicers are the language layer** — you
  need both (see [reflection (language)](../language/expressions/reflection.md)).
- **Reflection errors throw `std::meta::exception`** during constant evaluation;
  the exception can't escape to run time, so catch it in a `consteval` function or
  let it become a compile error.
- **C++26, one local compiler.** Only GCC 16 with `-freflection` implements
  P2996 (`__cpp_lib_reflection` is `202603L`); GCC 15 and Clang 22 have no
  `<meta>`. Names changed late in C++26, so older articles may not match.

## See also

- [Metaprogramming library / `<meta>` — cppreference](https://en.cppreference.com/w/cpp/meta)
- [Standard library header `<meta>` — cppreference](https://en.cppreference.com/w/cpp/header/meta)
- [Reflection (language operator & splicers)](../language/expressions/reflection.md)
- [Reflection queries: parameters, templates, operators, layout](reflection-queries.md)
- [P3560 Error handling in reflection](https://wg21.link/P3560)
