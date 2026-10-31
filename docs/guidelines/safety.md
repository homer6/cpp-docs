# Memory & safety

> How to keep C++ free of type, bounds, and lifetime errors: the Core
> Guidelines' safety profiles, ownership patterns, the common dangling traps,
> and what C++26 adds. **Sources:** C++ Core Guidelines (Jun 14, 2026),
> cppreference

## The three safety profiles

The Core Guidelines group their safety rules into three *profiles* that tools
(clang-tidy's `cppcoreguidelines-*` checks, MSVC's Core Check) can enforce.

> **Status:** profiles are a Core Guidelines / tooling concept. A language
> feature for them was proposed for C++26 but **did not make it in**; it's
> being worked on separately. What C++26 *did* add is below
> ([hardening](#c26-hardened-standard-library),
> [erroneous behaviour](#c26-erroneous-behaviour)).

### 1. Type safety (`Pro.type`)

Goal: never treat a `T` as a `U` through casts, unions, or varargs.

| Rule | |
|---|---|
| **Type.1** | Avoid casts (no `reinterpret_cast`; no `static_cast` for arithmetic you could avoid) |
| **Type.2** | Don't use `static_cast` to downcast (use `dynamic_cast`, or better, redesign) |
| **Type.3** | Don't use `const_cast` to cast away `const` |
| **Type.4** | Don't use C-style `(T)e` or functional `T(e)` casts |
| **Type.5** | Don't use a variable before it has been initialized |
| **Type.6** | Always initialize a data member |
| **Type.7** | Avoid naked `union` (use `std::variant`) |
| **Type.8** | Avoid varargs (use variadic templates) |

```cpp
// BAD — C-style cast, manual memory
int* p = (int*)malloc(sizeof(int) * n);

// BETTER — named cast shows intent
auto p = static_cast<int*>(malloc(sizeof(int) * n));

// BEST — no cast, no manual memory
auto v = std::vector<int>(n);
```

### 2. Bounds safety (`Pro.bounds`)

Goal: never access outside an object's range.

| Rule | |
|---|---|
| **Bounds.1** | Don't use pointer arithmetic; use `std::span` |
| **Bounds.2** | Only index into arrays using constant expressions |
| **Bounds.3** | No array-to-pointer decay |
| **Bounds.4** | Don't use library functions and types that aren't bounds-checked |

```cpp
// BAD — pointer + length, arithmetic
void f(int* p, int n) {
    for (int i = 0; i < n; ++i) use(*(p + i));
}

// GOOD — the span carries its own size
void f(std::span<const int> xs) {
    for (int x : xs) use(x);
}
```

### 3. Lifetime safety (`Pro.lifetime`)

Goal: no leaks, and no access through a pointer to something that's gone.
The profile currently has one rule, which covers a lot of ground:

- **Lifetime.1: Don't dereference a possibly invalid pointer** — uninitialized,
  null, past the end, or pointing to a destroyed object.

The patterns below are the common ways to break it.

## Ownership with smart pointers

```cpp
// unique_ptr — one owner
auto p  = std::make_unique<Widget>();
auto p2 = std::move(p);          // p is now null
Widget* w = p2.get();            // observe, don't own

// shared_ptr — shared owners; weak_ptr to observe without owning
auto s = std::make_shared<Widget>();
std::weak_ptr<Widget> weak = s;
if (auto locked = weak.lock()) {
    locked->use();               // safe: kept alive while `locked` exists
}
```

**Mistakes:**

```cpp
// BAD — two owners of one raw pointer → double delete
Widget* raw = new Widget{};
std::unique_ptr<Widget> a{raw};
std::unique_ptr<Widget> b{raw};

// BAD — shared_ptr cycle never frees
struct Node { std::shared_ptr<Node> next; std::shared_ptr<Node> prev; };

// GOOD — break the cycle: children own, parents observe
struct Node { std::shared_ptr<Node> child; std::weak_ptr<Node> parent; };
```

## Dangling references

### Returning references to locals or temporaries

```cpp
std::string& bad() {
    std::string local = "hello";
    return local;                  // dangling
}

const std::string& bad2() {
    return "hello";                // reference to a temporary std::string — dangling
}

std::string good() {
    std::string local = "hello";
    return local;                  // moved or elided, no copy
}
```

### Views that outlive their source

`std::string_view` and `std::span` don't own anything. Never let one outlive the
data it points at.

```cpp
std::string_view name() {
    std::string s = load();
    return s;                      // dangling view
}
```

### Lambda captures

```cpp
auto make_counter() {
    int count = 0;
    return [&count] { return ++count; };   // BAD: captures a dead local
}

auto make_counter_ok() {
    return [count = 0]() mutable { return ++count; };   // owns its state
}
```

Capturing `this` is the same trap: if the callback can outlive the object,
capture the data you need by value (or a `weak_ptr` to the object).

### Member declaration order (C.13)

Members are destroyed in reverse declaration order. A member that uses another
must be declared after it — see
[C.13](core-guidelines.md#classes-c).

### Iterator and reference invalidation

```cpp
std::vector<int> v{1, 2, 3, 4, 5};

// BAD — erase invalidates `it`
for (auto it = v.begin(); it != v.end(); ++it)
    if (*it == 3) v.erase(it);

// GOOD — C++20
std::erase(v, 3);
std::erase_if(v, [](int x) { return x % 2 == 0; });

// BAD — push_back may reallocate and dangle `first`
int& first = v[0];
v.push_back(6);
use(first);
```

Each container page in `docs/examples/containers/` has an "Iterator & reference
invalidation" table.

### Use after move

A moved-from standard library object is valid but unspecified. Don't read it
until you've assigned it a new value.

```cpp
auto v2 = std::move(v);
v.clear();                         // OK: puts v back into a known state
```

## Null safety

```cpp
void process(const Widget& w);       // can't be null — prefer this
void process(const Widget* w);       // may be null — check before use
std::optional<Widget> find(Key k);   // absence is a normal outcome
std::optional<Widget&> find_ref(Key k);   // C++26: optional reference
```

## C++26: hardened standard library

C++26 lets an implementation provide a **hardened** mode in which many library
preconditions become checked
[contract assertions](../examples/language/basic-concepts/contracts.md):
`operator[]` on `vector`, `span`, `string`, `string_view`, `array`, and
`mdspan`; `front()`/`back()` on empty containers; dereferencing an empty
`optional` or `expected`; and similar. A violation is caught at run time
instead of silently corrupting memory.

How you turn it on is up to the toolchain (e.g. libc++'s
`_LIBCPP_HARDENING_MODE`, libstdc++'s `_GLIBCXX_ASSERTIONS`). Enable it in
debug and test builds at minimum; the overhead is usually small enough for
production too.

This doesn't replace Bounds.1–4 — it's the safety net for when they're missed.

## Hardening in practice

**You don't need to wait for C++26 to turn hardening on.** The library modes
above already exist and run in production at scale. Google enabled hardened
libc++ across its server fleet (and in Chrome since 2022) and
[reported](https://security.googleblog.com/2024/11/retrofitting-spatial-safety-to-hundreds.html)
(Nov 2024):

- **0.30% average performance impact** across its services.
- **Over 1,000 bugs found**, with an estimated 1,000–2,000 new bugs prevented
  per year.
- **30% fewer segmentation faults** across production.

Those are averages for one fleet — measure on your own workload, but expect
the cost to be small.

**Rules of thumb:**

- **Hardened isn't memory-safe.** Library hardening catches *spatial* errors
  (out-of-bounds) in library types. It doesn't catch use-after-free, data races,
  or bounds errors on raw pointers. Keep claims about each defence separate.
- **Zero-initialization isn't erroneous behaviour.** Compiler flags like
  `-ftrivial-auto-var-init=zero` are a deployment mitigation; C++26 erroneous
  behaviour is a language rule. Use both, and still initialize (ES.20).
- **Look before you leap, then use the unchecked API.** For indexes that are
  correct by construction, plain `v[i]` plus hardening is the right
  combination; for untrusted indexes, check explicitly and handle the error.
  `at()` and its exception is rarely what you want (Abseil
  [#224](https://abseil.io/tips/224)).

### Build configurations

One configuration can't do everything. A reasonable set:

| Build | What it's for | Typical settings (check your toolchain's docs) |
|---|---|---|
| Release | What ships | Library hardening on (libc++ `_LIBCPP_HARDENING_MODE`, libstdc++ `_GLIBCXX_ASSERTIONS`); `-ftrivial-auto-var-init=zero`; `-fstack-protector-strong`; `_FORTIFY_SOURCE` where the platform supports it |
| Debug / test | Catch bugs early | Above, plus contracts `enforce`d and the library's debug mode |
| ASan + UBSan | Memory errors and undefined behaviour | `-fsanitize=address,undefined` |
| TSan | Data races | `-fsanitize=thread` (can't combine with ASan) |
| MSan | Uninitialized reads | `-fsanitize=memory` (Clang; dependencies must be instrumented too) |
| Fuzzing | Hostile-input boundaries (parsers, decoders, network) | libFuzzer or similar, with ASan |

Every bug a sanitizer or fuzzer finds gets a regression test.

Exact flag names, default modes, and costs vary by compiler version — verify
against the toolchain you've pinned before copying these into a build.

## C++26: erroneous behaviour

Reading an uninitialized automatic variable used to be **undefined behaviour**.
In C++26 it's **erroneous behaviour**: the variable holds some unspecified
value, the program is still wrong, and the implementation is encouraged to
diagnose it — but the optimizer may no longer assume it can't happen.

```cpp
int x;              // C++26: has an erroneous value, not an indeterminate one
use(x);             // erroneous behaviour — still a bug (ES.20)

int y [[indeterminate]];   // opt back into the old semantics, deliberately
```

Use `[[indeterminate]]` only for measured hot paths (e.g. a large buffer that
is filled immediately). Everywhere else, follow **ES.20** and initialize.

## GSL (Guidelines Support Library)

The Guidelines' helper library. Several of its types now have standard
equivalents:

| GSL | Standard replacement |
|---|---|
| `gsl::span` | `std::span` (C++20) |
| `Expects()` / `Ensures()` | `pre` / `post` contracts (C++26) |
| `gsl::finally` | — (still useful; or write a small scope guard) |
| `gsl::not_null<T*>` | — (still useful; or take a reference) |
| `gsl::owner<T*>` | — (annotation only; better to use `unique_ptr`) |

## See also

- [C++ Core Guidelines — Profiles](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-profile)
- [Core Guidelines digest](core-guidelines.md) ·
  [contracts](../examples/language/basic-concepts/contracts.md) ·
  [unique_ptr](../examples/memory/unique_ptr.md) ·
  [span](../examples/containers/views/span.md)
