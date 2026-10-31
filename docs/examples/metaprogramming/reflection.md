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

// layout:
constexpr auto member = meta::nonstatic_data_members_of(^^T, ctx)[1];   // T::b
static_assert(meta::offset_of(member).bytes == offsetof(T, b));
static_assert(meta::size_of(^^T) == sizeof(T) && meta::alignment_of(^^T) == alignof(T));
```

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
