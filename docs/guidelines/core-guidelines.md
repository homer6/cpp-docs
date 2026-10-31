# C++ Core Guidelines — digest

> The rules from Stroustrup & Sutter's
> [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
> that pay off most often, with short examples. **Edition:** Jun 14, 2026 ·
> **Targets:** C++20 baseline, C++23/26 noted inline

Rule IDs match the Guidelines, so you can search the full text for any of them
(e.g. "R.11"). Where a newer standard gives a better tool than the Guidelines
mention, it's noted.

## Philosophy (P) — read these first

- **P.1: Express ideas directly in code.** Compilers don't read comments.
- **P.4: Ideally, a program should be statically type safe.** Avoid casts,
  unions, and varargs.
- **P.5: Prefer compile-time checking to run-time checking.** `static_assert`,
  `constexpr`, concepts.
- **P.8: Don't leak any resources.** Every resource has an owner (RAII).
- **P.10: Prefer immutable data to mutable data.** `const` by default.

## Resource management (R)

**R.1: Manage resources automatically using RAII**

```cpp nocompile
// BAD — leaks if anything in the middle throws
void f() {
    int* p = new int[100];
    // ...
    delete[] p;
}

// GOOD — freed on every path out of the scope
void f() {
    auto p = std::make_unique<int[]>(100);
    // ...
}
```

- **R.3: A raw pointer (`T*`) is non-owning.** Raw pointers only observe.
- **R.5: Prefer scoped objects; don't heap-allocate unnecessarily.**
  `Widget w{};` beats `auto p = new Widget{};`.
- **R.11: Avoid calling `new` and `delete` explicitly.** Use `std::make_unique`
  / `std::make_shared`.
- **R.20: Use `unique_ptr` or `shared_ptr` to represent ownership.**
- **R.21: Prefer `unique_ptr` over `shared_ptr` unless you need to share
  ownership.**

Reference: [unique_ptr](../examples/memory/unique_ptr.md) ·
[shared_ptr](../examples/memory/shared_ptr.md)

## Functions (F)

- **F.1–F.3:** Package meaningful operations as functions; one logical
  operation each; keep them short.
- **F.4: If a function might be evaluated at compile time, declare it
  `constexpr`.**
- **F.6: If your function must not throw, declare it `noexcept`.**

**F.15–F.17: Pass parameters the simple, conventional way**

```cpp
void f(int x);                    // "in", cheap to copy: by value
void f(const std::string& s);     // "in", expensive to copy: by const reference
void f(std::string_view sv);      // "in", read-only string: by view
void f(std::span<const int> xs);  // "in", read-only contiguous elements: by span
void f(std::string& s);           // "in-out": by non-const reference
void f(std::string s);            // "sink" (will be moved from): by value
```

**F.16 — what counts as "cheap"?** The Jun 2026 edition raised the threshold:
tools should warn when a by-value parameter is larger than `4 * sizeof(void*)`
(32 bytes on 64-bit), up from `2 * sizeof(void*)`. Below that, pass by value.

- **F.20: For "out" values, prefer return values to output parameters.**
- **F.21: To return several values, return a struct** (or tuple), and unpack
  with structured bindings.

```cpp nocompile
struct Parsed { int id; std::string name; };
Parsed parse(std::string_view line);

auto [id, name] = parse(line);
```

**I.6 / I.8 → contracts (C++26).** The Guidelines express preconditions with
GSL's `Expects()` / `Ensures()`. C++26 has them in the language:

```cpp needs=contracts
int pop(std::vector<int>& v)
    pre(!v.empty())
    post(r: r >= 0);
```

Reference: [contracts](../examples/language/basic-concepts/contracts.md)

## Classes (C)

- **C.2: Use `class` if there's an invariant; `struct` if members vary
  independently.**
- **C.4: Make a function a member only if it needs the representation.**
- **C.9: Minimize exposure of members.**

**C.13 (new in 2026): If member `B` uses member `A`, declare `A` before `B`**

Members are constructed in declaration order and destroyed in reverse. Declare
them the other way round and `B` touches `A` before it exists and after it's
gone.

```cpp nocompile
// BAD — the thread starts before data exists, and outlives it on destruction
class Job {
    std::jthread worker;                 // constructed first, destroyed last
    std::unique_ptr<int> data;
};

// GOOD — data outlives the thread that reads it
class Job {
    std::unique_ptr<int> data;
    std::jthread worker;
};
```

**C.20: Avoid defining default operations if you can (Rule of Zero)**

```cpp body
class Widget {
    std::string name;
    std::vector<int> data;
    // no destructor, no copy/move — the compiler's are correct
};
```

**C.21: If you define or `=delete` any copy, move, or destructor function,
define or `=delete` them all (Rule of Five).**

- **C.35: A base class destructor is either public and virtual, or protected
  and non-virtual.**
- **C.45 / C.48: Prefer default member initializers** to constructors that only
  set constants.

```cpp
class X {
    int i = 0;
    std::string s;
};
```

- **C.46: By default, declare single-argument constructors `explicit`.**
- **C.49: Prefer initialization to assignment in constructors.**

Reference: [special members](../examples/language/classes/special-members.md)

## Expressions and statements (ES)

- **ES.1: Prefer the standard library to handcrafted code.**
- **ES.5: Keep scopes small.**
- **ES.10: Declare one name per declaration.** `int* p, q;` makes `q` an `int`.
- **ES.11: Use `auto` to avoid repeating type names.**
- **ES.20: Always initialize an object.** In C++26 reading an uninitialized
  variable is *erroneous behaviour* rather than undefined — still a bug, but a
  diagnosable one. See [safety](safety.md#c26-erroneous-behaviour).
- **ES.21 / ES.22: Don't introduce a variable before you need it, or before you
  have a value for it.**

**ES.23: Prefer `{}` initialization** — it rejects narrowing.

```cpp nocompile
double d = 7.9;
int a = d;       // compiles, silently truncates
int b{d};        // error: narrowing
```

The 2026 edition notes that copying an *empty* base (a tag type) isn't slicing,
so tools shouldn't warn about it.

**ES.28: Use an immediately-invoked lambda for complex `const`
initialization**

```cpp nocompile
const auto config = [&] {
    if (use_defaults) return Config::defaults();
    return Config::from_file(path);
}();
```

- **ES.46: Avoid narrowing conversions.**
- **ES.47: Use `nullptr`, not `0` or `NULL`.**
- **ES.49: If you must cast, use a named cast** (`static_cast`, …), never a
  C-style cast.

Reference: [initialization](../examples/language/declarations-initialization/initialization.md)

## Error handling (E)

- **E.1: Decide the error-handling strategy early.**
- **E.2: Throw to signal that a function can't perform its task.**
- **E.3: Use exceptions for error handling only.**
- **E.6 / E.13: Use RAII so nothing leaks when an exception passes through.**
- **E.16: Destructors, deallocation, `swap`, and exception-type copy must
  never fail.**
- **E.17: Don't try to catch every exception in every function.**

Where exceptions aren't wanted (or failure is routine), return
[`std::expected`](../examples/utilities/expected.md) instead — see
[design principles](design-principles.md#3-errors-in-the-type-system).

## Constants and immutability (Con)

- **Con.1: By default, make objects immutable.**
- **Con.2: By default, make member functions `const`.**
- **Con.3: By default, pass pointers and references to `const`.**
- **Con.5: Use `constexpr` for values computed at compile time.**

## Concurrency (CP)

- **CP.1: Assume your code will run in a multi-threaded program.**
- **CP.2: Avoid data races.**
- **CP.3: Minimize sharing of writable data.**
- **CP.4: Think in terms of tasks, not threads.**

**CP.20: Use RAII, never plain `lock()`/`unlock()`**

```cpp nocompile
{
    std::scoped_lock lock{m};   // C++17; also locks several mutexes deadlock-free
    // ...
}                               // unlocked here, even on exception
```

- **CP.22: Never call unknown code while holding a lock.**
- **CP.25 / CP.26: Prefer joining threads; don't `detach()`.** `std::jthread`
  (C++20) joins in its destructor.
- **CP.44: Name your locks.** `std::scoped_lock{m};` locks and immediately
  unlocks a temporary.

Reference: [mutexes](../examples/concurrency/mutexes.md) ·
[senders/receivers](../examples/execution/senders-receivers.md) (C++26 task model)

## Templates (T)

- **T.1 / T.2: Use templates to raise the level of abstraction.**
- **T.10: Specify concepts for all template arguments.**

```cpp
template <std::integral T>
T gcd(T a, T b);
```

- **T.40: Pass operations to algorithms as function objects** (usually lambdas).
- **T.46 was removed in 2026.** It required template arguments to be
  semiregular; constrain on what the template actually uses instead.
- **T.61: Don't over-parameterize members.**
- **T.120: Use template metaprogramming only when you really need to.** In
  C++26, [reflection](../examples/language/expressions/reflection.md) replaces
  much of it.

Reference: [standard concepts](../examples/concepts/standard-concepts.md)

## Performance (Per)

- **Per.1 / Per.2: Don't optimize without reason, or prematurely.** Measure.
- **Per.4 / Per.5: Complicated or low-level code isn't automatically faster.**
- **Per.6: Don't make claims about performance without measurements.** How to
  measure — benchmarks, `perf`, flame graphs — is in
  [performance tooling](performance-tooling.md).
- **Per.7: Design to enable optimization** — value semantics, contiguous data,
  moves instead of copies.
- **Per.10: Rely on the static type system.**
- **Per.11: Move computation from run time to compile time.**

```cpp
constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}
static_assert(factorial(5) == 120);
```

## Standard library (SL)

- **SL.1: Use libraries wherever possible.**
- **SL.con.1: Prefer `std::array` or `std::vector` over C arrays.**
- **SL.con.2: Prefer `std::vector` by default** unless you have a reason to use
  another container.
- **SL.str.2: Use `std::string_view` to refer to character sequences.**
- **SL.io.50: Avoid `std::endl`** — `'\n'` doesn't flush.
- **SL.regex (2026 note): `std::regex` is slow.** For performance-critical work,
  the Guidelines now suggest a third-party regular-expression library.

## Quick reference: when to use what

| Need | Use |
|---|---|
| Single ownership | `std::unique_ptr<T>` |
| Shared ownership | `std::shared_ptr<T>` |
| Non-owning, never null | `T&` |
| Non-owning, maybe null | `T*` |
| Optional value | `std::optional<T>` |
| Optional reference (C++26) | `std::optional<T&>` |
| Value or error | `std::expected<T, E>` |
| One of a fixed set of types | `std::variant<A, B, C>` |
| Open set of types | `std::unique_ptr<Base>` (or a concept) |
| Read-only string | `std::string_view` |
| Contiguous elements | `std::span<T>` |
| Fixed capacity, no heap (C++26) | `std::inplace_vector<T, N>` |

## See also

- [Full C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Memory & safety](safety.md) · [Modern idioms](modern-idioms.md) ·
  [Design principles](design-principles.md)
