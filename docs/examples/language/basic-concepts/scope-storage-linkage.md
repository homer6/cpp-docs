# Scope, storage duration & linkage

> Three independent questions about every name/object: **where** is it visible
> (scope), **how long** does it live (storage duration), and **across translation
> units** can it be referred to (linkage).

## Scope — where a name is visible

```cpp
int g;                       // namespace (global) scope
void f() {
    int local;               // block scope: visible to the end of the block
    for (int i = 0; i < 10; ++i) { /* i is scoped to the loop */ }
    if (int n = compute(); n > 0) { /* n scoped to the if (C++17 init-statement) */ }
}
```

Inner names **shadow** outer ones of the same name.

## Storage duration — how long an object lives

| Duration | Lives | Example |
|---|---|---|
| **automatic** | for its enclosing block | local variables |
| **static** | for the whole program | `static`/namespace-scope variables |
| **thread** | for the thread | `thread_local` variables |
| **dynamic** | until you free it | `new`/`make_unique` |

```cpp
void counter() {
    static int calls = 0;        // initialized once, persists across calls
    ++calls;
}
thread_local int per_thread;     // one instance per thread
```

## Linkage — visibility across translation units (TUs)

| Linkage | Meaning |
|---|---|
| **no linkage** | local variables — not referable elsewhere |
| **internal** | one per TU: `static` globals, `const`/`constexpr` globals, anonymous namespaces |
| **external** | one shared across the program: normal functions, non-const globals, `extern` |

```cpp
static int helper();             // internal: private to this .cpp
extern int shared;               // external: defined in one TU, declared in others
namespace { int hidden; }        // anonymous namespace → internal linkage (preferred over static)
inline int shared_inline = 42;   // inline variable (C++17): one definition across TUs, define in a header
```

## The One Definition Rule (ODR)

A program may **declare** an entity many times but must **define** it exactly once
(across all TUs) — except for `inline` functions/variables and templates, which
may be defined in every TU as long as the definitions are identical.

## Gotchas

- **Prefer `inline` (C++17) for header globals/constants** so each including TU
  sees one shared definition — no ODR violation, no "multiply defined" linker
  errors.
- **Anonymous namespaces over file-scope `static`** for internal linkage in modern
  C++ — they also work for types.
- **`static` is overloaded** — it means storage duration *and* internal linkage
  depending on context (and something else again inside classes). Read it
  carefully.
- **Static initialization order across TUs is unspecified** (the "static
  initialization order fiasco"). Use function-local statics (initialized on first
  use) to dodge it.

## See also

- [Scope — cppreference](https://en.cppreference.com/w/cpp/language/scope)
- [Storage duration & linkage — cppreference](https://en.cppreference.com/w/cpp/language/storage_duration)
- [One Definition Rule — cppreference](https://en.cppreference.com/w/cpp/language/definition)
