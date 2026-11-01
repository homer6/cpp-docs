# Reflection queries (`<meta>`)

> The `<meta>` functions that look inside one entity: a function's parameters, a
> specialization's template and arguments, an operator's kind, where something was
> declared, what it is, and how it's laid out. **Header:** `<meta>` ·
> **Namespace:** `std::meta` · **Since:** C++26

## Mental model

Each query takes a `std::meta::info` and returns another `info`, a
`std::vector<info>`, a `bool`, a number or a `std::source_location`, all at
compile time. The basics (`^^`, splicers, `members_of`, `define_aggregate`) are
on [Static reflection](reflection.md); this page covers the narrower queries.

```
  ^^connect ─ parameters_of ─▶ { host, port } ─ identifier_of / type_of
  ^^std::array<int, 3> ─ template_of ─▶ ^^std::array
                       └ template_arguments_of ─▶ { ^^int, 3 }
```

Only GCC 16 with `-std=c++26 -freflection` implements these locally; GCC 15 and
Clang 22 have no `<meta>`.

## Examples

### Function parameters (P3096)

```cpp needs=reflection
#include <meta>
namespace meta = std::meta;

void connect(const char* host, int port = 80);

// parameters_of returns a std::vector, which can't be a constexpr variable;
// define_static_array keeps it as a static span
constexpr auto params = std::define_static_array(meta::parameters_of(^^connect));
static_assert(params.size() == 2);
static_assert(meta::identifier_of(params[1]) == "port");
static_assert(meta::type_of(params[0]) == ^^const char*);
static_assert(meta::has_default_argument(params[1]));
static_assert(meta::return_type_of(^^connect) == ^^void);

// a function *type* has parameters too, but they have no names:
static_assert(meta::parameters_of(^^void(int, double)).size() == 2);
```

A parameter's name comes from the declarations the compiler has seen. If two
declarations name it differently, it has no name:

```cpp needs=reflection
void g(int a);
void g(int b);
static_assert(!std::meta::has_identifier(std::meta::parameters_of(^^g)[0]));
```

### Templates and their arguments

```cpp needs=reflection
#include <array>
#include <meta>
#include <vector>
namespace meta = std::meta;

constexpr auto a = ^^std::array<int, 3>;
static_assert(meta::has_template_arguments(a));
static_assert(meta::template_of(a) == ^^std::array);
static_assert(meta::template_arguments_of(a)[0] == ^^int);              // a type
static_assert(meta::extract<std::size_t>(meta::template_arguments_of(a)[1]) == 3);  // a value

// defaulted arguments are listed too: vector<int> is vector<int, allocator<int>>
static_assert(meta::template_arguments_of(^^std::vector<int>).size() == 2);

// an alias isn't a specialization until you look through it:
using VI = std::vector<int>;
static_assert(!meta::has_template_arguments(^^VI));
static_assert(meta::has_template_arguments(meta::dealias(^^VI)));
```

### Operators

`operator_of` says which operator an operator function overloads, as a
`std::meta::operators` enumerator; `symbol_of` spells it.

```cpp needs=reflection
#include <meta>
namespace meta = std::meta;

struct V {
    V operator+(V) const;
    bool operator==(const V&) const = default;
};
static_assert(meta::is_operator_function(^^V::operator+));
static_assert(meta::operator_of(^^V::operator+) == meta::operators::op_plus);
static_assert(meta::symbol_of(meta::operator_of(^^V::operator==)) == "==");
```

### Names and source locations

```cpp needs=reflection
#include <meta>
#include <print>
#include <vector>

struct Config { int port; };
using VI = std::vector<int>;

int main() {
    constexpr auto loc = std::meta::source_location_of(^^Config);
    std::println("{}:{}", loc.file_name(), loc.line());       // where Config was declared
    std::println("{}", std::meta::display_string_of(^^VI));   // GCC 16: "VI {aka std::vector<int>}"
}
```

`identifier_of` is the plain name and fails without one (check `has_identifier`
first). `display_string_of` always works, but its text is up to the
implementation, so use it for messages, not logic.

### Predicates

About 150 `is_*` / `has_*` predicates, in families:

| Family | Examples |
|---|---|
| What it is | `is_type`, `is_function`, `is_variable`, `is_namespace`, `is_template`, `is_function_parameter`, `is_annotation` |
| Access | `is_public`, `is_protected`, `is_private`, `is_accessible(r, ctx)` |
| Members | `is_static_member`, `is_nonstatic_data_member`, `is_bit_field`, `is_mutable_member` |
| Functions | `is_virtual`, `is_pure_virtual`, `is_override`, `is_deleted`, `is_defaulted`, `is_noexcept`, `is_constructor`, `is_special_member_function` |
| Qualifiers | `is_const`, `is_volatile`, `is_lvalue_reference_qualified` (on a function or object) |
| Storage, linkage | `has_static_storage_duration`, `has_internal_linkage`, `has_external_linkage` |
| Type traits | `is_class_type`, `is_const_type`, `is_trivially_copyable_type`… (`<type_traits>` with a `_type` suffix) |

```cpp needs=reflection
#include <meta>
namespace meta = std::meta;

struct B {
    virtual void f();
    static int s;
    const int k = 0;
};
static_assert(meta::is_virtual(^^B::f));
static_assert(meta::is_static_member(^^B::s));
static_assert(meta::is_const(^^B::k) && meta::is_const_type(meta::type_of(^^B::k)));
```

### Layout

`offset_of` returns a `std::meta::member_offset` of `bytes` and `bits`, so it
places bit-fields too; `bit_size_of` and `size_of` give a member's width.

```cpp needs=reflection
#include <cstddef>
#include <cstdint>
#include <meta>
namespace meta = std::meta;

struct Flags { std::uint8_t a : 3; std::uint8_t b : 5; int c; };

static_assert(meta::bit_size_of(^^Flags::b) == 5);
static_assert(meta::offset_of(^^Flags::b).bytes == 0);
static_assert(meta::offset_of(^^Flags::b).bits == 3);         // 3 bits into byte 0
static_assert(meta::offset_of(^^Flags::c).bytes == offsetof(Flags, c));
static_assert(meta::offset_of(^^Flags::c).total_bits() == 8 * offsetof(Flags, c));
static_assert(meta::size_of(^^Flags) == sizeof(Flags));
static_assert(meta::alignment_of(^^Flags) == alignof(Flags));
```

## Gotchas

- **Aliases hide specializations.** `^^VI` reflects the alias; `template_of` and
  `template_arguments_of` throw on it. Call `dealias` first.
- **`size_of(bit-field)` throws** (`bit_size_of` is the one to use); `offset_of`
  of a bit-field needs its `.bits`.
- **Parameter names are fragile:** they come from declarations, so a header and
  a definition that disagree leave the parameter unnamed.
- **`display_string_of` text isn't portable**; compare reflections, not strings.
- **Failing queries throw `std::meta::exception`** at compile time; see
  [errors](reflection.md#errors-stdmetaexception-c26).

## See also

- [Metaprogramming library / `<meta>` — cppreference](https://en.cppreference.com/w/cpp/meta)
- [Standard library header `<meta>` — cppreference](https://en.cppreference.com/w/cpp/header/meta)
- [Static reflection (`<meta>`)](reflection.md) ·
  [Reflection (language)](../language/expressions/reflection.md)
- [P2996 Reflection for C++26](https://wg21.link/P2996) ·
  [P3096 Function parameter reflection](https://wg21.link/P3096)
