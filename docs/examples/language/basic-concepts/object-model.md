# The object model & `main`

> What an "object" is in C++, how its lifetime works, and how a program begins.

## Objects and lifetime

An **object** is a region of storage with a type and a lifetime. Lifetime begins
when construction completes and ends when the destructor runs (or, for trivial
types, when storage is reused/released). Using an object outside its lifetime is
undefined behavior.

```cpp body
{
    std::string s = "hi";    // lifetime begins (constructor)
    use(s);
}                            // lifetime ends (destructor) — RAII cleanup
```

This is the engine behind RAII: tie a resource to an object, and the destructor
releases it deterministically at scope exit (see
[special member functions](../classes/special-members.md)).

## Temporary objects & lifetime extension

```cpp
std::string make();
const std::string& r = make();   // a temporary bound to a const ref is lifetime-extended
                                 // to match r — valid until r goes out of scope
std::string_view sv = make();    // NOT extended — dangles immediately (view, not a ref)
```

A temporary normally dies at the end of the full-expression. Binding it
**directly** to a `const T&` or `T&&` (including through member access, as in
`const int& x = std::pair{1, 2}.first;`) extends it to the reference's
lifetime. Anything in between breaks the chain:

```cpp nocompile
const int& m = std::min(1, 2);   // dangles: min returns a reference, not the temporary
```

- **Returning** a reference to a temporary never extends it; in C++26 (P2748)
  `const std::string& f() { return std::string{}; }` is ill-formed. GCC 15 and
  Clang 22 reject it with `-std=c++26` and only warn in C++23.
- **Range-`for`** (C++23, P2718): every temporary in the range initializer
  lives until the loop ends, so `for (int x : make_holder().items())` no longer
  dangles. GCC 15 and Clang 22 implement it (`__cpp_range_based_for >= 202211L`).

## Implicit-lifetime types

Some types are simple enough that an object can come into existence
**without a constructor call**: scalars, arrays, aggregates (whose destructor
isn't user-provided), and classes with a trivial eligible constructor and a
trivial, non-deleted destructor. Operations that create storage
(`std::malloc`, `operator new`, `std::allocator::allocate`) or copy bytes
(`std::memcpy`, `std::memmove`) implicitly create such objects there, if that
gives the program defined behaviour (C++20, P0593, applied as a defect report).

```cpp body
struct Point { int x, y; };                                    // implicit-lifetime
auto* p = static_cast<Point*>(std::malloc(sizeof(Point)));     // a Point now exists
p->x = 1; p->y = 2;                                            // defined
std::free(p);
```

The same code with `std::string` is UB: it has a non-trivial constructor, so
construct it with placement `new` or `std::construct_at`. To check a type use
`std::is_implicit_lifetime` (C++23; in libc++ 22 and libstdc++ 16, not
libstdc++ 15). To reinterpret an existing buffer, use
[`std::start_lifetime_as`](../../memory/raw-memory.md#stdstart_lifetime_as-c23).

## `main` — the entry point

```cpp
int main() { /* ... */ }                      // implicit `return 0;` if you fall off the end
```

or, to receive the command-line arguments:

```cpp
int main(int argc, char* argv[]) { /* ... */ }// command-line args
```

- Returning from `main` (or `return 0;`) signals success; non-zero signals
  failure. `main` is the **only** function with an implicit `return 0`.
- `argv[0]` is (conventionally) the program name; `argv[argc]` is null.
- You cannot call `main` recursively, take its address, or overload it.
- In a [freestanding](freestanding.md) implementation (kernels, firmware),
  whether `main` is required at all is implementation-defined.

## Startup & shutdown order

- Namespace-scope objects with static storage are constructed **before** `main`
  (within a TU, in declaration order; **across** TUs, unspecified).
- They're destroyed in reverse order after `main` returns (or after
  `std::exit`).
- `std::abort()` and `std::quick_exit()` skip normal destruction; `std::atexit`
  registers cleanup handlers.

## Trivial vs non-trivial types

- **Trivially copyable** types (no custom copy/move/dtor; e.g. `int`, a plain
  struct of ints) can be copied with `memcpy` and have no lifecycle hooks.
- Types with constructors/destructors run that code — that's what enables RAII,
  but also why copying them isn't free.

## Gotchas

- **Don't return references/pointers/views to locals** — their lifetime ends at
  the return. A `string_view`/`span`/reference outliving its source is the most
  common dangling bug.
- **Temporary lifetime extension only applies to binding to a `const&` (or `&&`)
  directly** — not through a function, not to a `string_view`/`span`.
- **Uninitialized locals:** reading one is
  [erroneous behaviour](erroneous-behaviour.md) in C++26, not UB, but still a bug.
- **Static init order across TUs is unspecified.** Use the "construct on first
  use" idiom (a function-local `static`) for cross-TU dependencies.
- **`int main`, not `void main`.** `void main` is non-standard despite some
  compilers tolerating it.

## See also

- [Object — cppreference](https://en.cppreference.com/w/cpp/language/object)
- [Lifetime — cppreference](https://en.cppreference.com/w/cpp/language/lifetime)
- [Reference initialization (lifetime of a temporary) — cppreference](https://en.cppreference.com/w/cpp/language/reference_initialization#Lifetime_of_a_temporary)
- [std::is_implicit_lifetime — cppreference](https://en.cppreference.com/w/cpp/types/is_implicit_lifetime)
- [P0593R6: Implicit creation of objects for low-level object manipulation](https://wg21.link/p0593r6)
- [main function — cppreference](https://en.cppreference.com/w/cpp/language/main_function)
