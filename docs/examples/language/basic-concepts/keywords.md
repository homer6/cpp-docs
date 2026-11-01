# Keywords and reserved identifiers

> The words you can't use as names: the keywords, the alternative tokens, the
> identifiers that have a special meaning only in certain places, and the names
> reserved for the implementation. **Since:** C++98 (list grows each standard)

## Mental model

```
identifier-shaped token
 ├─ keyword                 always reserved (Table 5)        int, class, co_await, contract_assert
 ├─ alternative token       an operator spelled as a word    and, or, not, xor_eq ...
 ├─ identifier with special meaning   only in context        final, override, import, module, pre, post
 │                                    (a normal name everywhere else)
 └─ ordinary identifier
      └─ ... but reserved to the implementation if it has __ anywhere,
             starts with _Uppercase, or starts with _ at global scope
```

Keywords are recognised in translation phase 6 (preprocessing tokens become
tokens) and are "unconditionally treated as keywords in phases 6 and 7" — except
inside an attribute token, which is why `[[maybe_unused]]` and friends never
clash with them. To the preprocessor (phase 4) they are still identifiers
(MSVC is an exception: it rejects a keyword in `#if` with C1017, even with
`/Zc:preprocessor`).

## Declaration

### Keywords — C++26, Table 5 ([lex.key])

All 82, with the standard that added each one (11 = C++11, …). Unmarked ones
date from C++98. Checked against [N5050](https://wg21.link/n5050) (the C++26
draft that sets `__cplusplus` for C++26) and the current C++29 working draft at
eel.is: Tables 4, 5 and 6 are identical in both.

| | | | |
|---|---|---|---|
| `alignas` (11) | `constinit` (20) | `int` | `struct` |
| `alignof` (11) | `continue` | `long` | `switch` |
| `asm` | `contract_assert` (26) | `mutable` | `template` |
| `auto` | `decltype` (11) | `namespace` | `this` |
| `bool` | `default` | `new` | `thread_local` (11) |
| `break` | `delete` | `noexcept` (11) | `throw` |
| `case` | `do` | `nullptr` (11) | `true` |
| `catch` | `double` | `operator` | `try` |
| `char` | `dynamic_cast` | `private` | `typedef` |
| `char16_t` (11) | `else` | `protected` | `typeid` |
| `char32_t` (11) | `enum` | `public` | `typename` |
| `char8_t` (20) | `explicit` | `register` | `union` |
| `class` | `export` | `reinterpret_cast` | `unsigned` |
| `co_await` (20) | `extern` | `requires` (20) | `using` |
| `co_return` (20) | `false` | `return` | `virtual` |
| `co_yield` (20) | `float` | `short` | `void` |
| `concept` (20) | `for` | `signed` | `volatile` |
| `const` | `friend` | `sizeof` | `wchar_t` |
| `const_cast` | `goto` | `static` | `while` |
| `consteval` (20) | `if` | `static_assert` (11) |  |
| `constexpr` (11) | `inline` | `static_cast` |  |

Keywords whose meaning changed:

| Keyword | History |
|---|---|
| `auto` | C++98: storage class (redundant). C++11: type deduction. |
| `register` | Deprecated in C++11, meaning removed in C++17. **Still reserved**, unused ("reserved for future use"). GCC accepts it with a warning (`-Wregister`). |
| `export` | C++98: exported templates (almost never implemented). Removed in C++11 but kept reserved. C++20: modules. |
| `bool`, `static_assert`, `thread_local`, `alignas`, `alignof` | C++ keywords that were only *macros* in C (`<stdbool.h>`, `<assert.h>`, `<threads.h>`, `<stdalign.h>`) until C23 made them keywords. |

### Alternative tokens — Table 6

Reserved, and they **are** the operator, not a macro for it:

| Token | Means | Token | Means |
|---|---|---|---|
| `and` | `&&` | `not` | `!` |
| `and_eq` | `&=` | `not_eq` | `!=` |
| `bitand` | `&` | `or` | `\|\|` |
| `bitor` | `\|` | `or_eq` | `\|=` |
| `compl` | `~` | `xor` | `^` |
| | | `xor_eq` | `^=` |

The digraphs `<%` `%>` `<:` `:>` `%:` `%:%:` (for `{` `}` `[` `]` `#` `##`) are
punctuators, not identifiers. Trigraphs (`??=` …) were **removed in C++17**.

### Identifiers with special meaning — Table 4 ([lex.name])

C++26 lists exactly six:

| Identifier | Since | Special where |
|---|---|---|
| `final` | C++11 | after a class name, or at the end of a virtual function declarator |
| `override` | C++11 | at the end of a virtual function declarator |
| `import` | C++20 | starting an import declaration / `import` directive |
| `module` | C++20 | starting a module declaration / `module` directive |
| `pre` | C++26 | function contract specifier ([contracts](contracts.md)) |
| `post` | C++26 | function contract specifier |

Everywhere else they are ordinary names. Any ambiguity is resolved in favour of
the ordinary identifier.

`trivially_relocatable_if_eligible` and `replaceable_if_eligible` (P2786,
trivial relocatability) were in Table 4 of earlier C++26 drafts (e.g. N5008)
but were removed before N5032; they are **not** in C++26's Table 4, and neither
GCC 16 nor Clang 22 accepts them as class-property specifiers. Don't rely on
them.

### Reserved identifiers ([lex.name]/4)

Using these is ill-formed, no diagnostic required (the compiler won't tell you):

| Pattern | Reserved for | Example |
|---|---|---|
| contains `__` anywhere | the implementation, everywhere | `my__var`, `__init` |
| `_` + uppercase letter | the implementation, everywhere | `_Data`, `_MAX` |
| `_` + anything | the implementation, in the global namespace | `::_count` |

Library-specific reservations add more: the `std` namespace (and `std2`,
`posix`, …), every standard macro and header name, and user-defined literal
suffixes that don't start with `_`.

## When to use it

- Treat the table above as the list of names a declaration can never use, and
  the reserved patterns as names you should never invent: no leading `_` + capital
  for include guards (`_MY_HEADER_H` is reserved), no `__` in macros.
- Use `final` / `override` (C++11) on every overriding function and on leaf
  classes; that they are contextual is a compatibility detail, not an
  invitation to name variables `override`.
- Alternative tokens (`and`, `or`, `not`) are standard and portable. Whether to
  use them is a style choice; be consistent.

## Examples

### Contextual identifiers are still ordinary names

```cpp
struct Base { virtual void f(); };
struct Derived final : Base { void f() override; };   // special meaning here

int final = 1, override = 2;          // ...and ordinary names here
int pre = 3, post = 4, module = 5;    // compiles with GCC 15/16 and Clang 22
```

### Alternative tokens and digraphs

```cpp
constexpr bool exactly_one(bool a, bool b) { return (a or b) and not (a and b); }
static_assert(exactly_one(true, false));

int digraph_array<:2:> = <%1, 2%>;    // int digraph_array[2] = {1, 2};
```

### Keywords can't be names

```cpp compile-fail
int class = 0;
```

```cpp compile-fail needs=contracts
int contract_assert = 0;   // a keyword in C++26
```

`contract_assert` is only reserved by compilers that implement contracts: GCC 16
rejects the line above with `-std=c++26`, while GCC 15 and Clang 22 still accept
it as an ordinary name. Don't use it as a name in code meant to move to C++26.

### Inside attributes, keywords aren't reserved

```cpp
[[maybe_unused]] static int counter = 0;
```

## Gotchas

- **`#define private public` is forbidden**, as is defining any keyword or any
  Table 4 identifier as a macro ([cpp.replace.general]) — but compilers
  don't all diagnose it. Clang errors under `-pedantic-errors`
  (`-Wkeyword-macro`); GCC 15 accepts it silently. (`likely` and `unlikely`
  may be function-like macros; nothing else that is an attribute token may be.)
- **Reserved identifiers fail silently.** `_Tp`, `__x` and `_MY_GUARD_H` compile
  today and can collide with a macro in tomorrow's standard library. The rule
  is "no diagnostic required", so no compiler will warn.
- **`register` is not free.** It has no meaning since C++17, but it's still a
  keyword, so `int register = 0;` doesn't compile.
- **C keywords differ.** C has `restrict`, `_Generic`, `_Atomic`, and (since C23)
  `typeof`, `constexpr` and `static_assert` as keywords; none of the
  `_Keyword` forms are C++ keywords.

## See also

- [Keywords — cppreference](https://en.cppreference.com/w/cpp/keyword)
- [Identifiers — cppreference](https://en.cppreference.com/w/cpp/language/identifiers)
- [Alternative operator representations — cppreference](https://en.cppreference.com/w/cpp/language/operator_alternative)
- [N5050](https://wg21.link/n5050) — C++26 draft (`[lex.key]`, `[lex.name]`)
- [\[lex.key\]](https://eel.is/c++draft/lex.key),
  [\[lex.name\]](https://eel.is/c++draft/lex.name),
  [\[lex.digraph\]](https://eel.is/c++draft/lex.digraph) — C++ working draft
- [Comments](comments.md) · [Contracts](contracts.md)
