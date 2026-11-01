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
libstdc++ 15).

Implicit creation happens only at those operations. Bytes that reach an
existing buffer some other way (a `read()` from a socket, a memory-mapped file)
need [`std::start_lifetime_as`](../../memory/raw-memory.md#stdstart_lifetime_as-c23),
the explicit form of the same idea. When storage already holds an object and
you build a new one over it, see
[`std::launder`](../../memory/raw-memory.md#stdlaunder-c17).

## Strict aliasing

You may read or write an object only through a glvalue of a type that can
*access* it: a type similar to the object's dynamic type (the same type,
ignoring cv-qualifiers), its signed or unsigned counterpart, or `char`,
`unsigned char` or `std::byte`. Any other access is undefined behaviour. This is
the **strict aliasing rule**, and it lets the optimizer assume that an `int*`
and a `float*` never point at the same object.

```cpp nocompile
float f = 1.0f;
auto bits = *reinterpret_cast<std::uint32_t*>(&f);   // UB: no uint32_t object lives there

int g(int* i, float* x) { *i = 1; *x = 0.0f; return *i; }
// Called with aliasing pointers, GCC 15 and Clang 22 at -O2 return 1, not 0:
// the store through x is assumed not to touch *i
```

To reinterpret the bits, copy them into a new object. `std::bit_cast` (C++20,
`<bit>`) does it in one `constexpr` expression; `std::memcpy` is the older
equivalent:

```cpp
#include <bit>
#include <cstring>

std::uint32_t via_bit_cast(float f) {
    return std::bit_cast<std::uint32_t>(f);   // 0x3f800000 for 1.0f
}
std::uint32_t via_memcpy(float f) {
    std::uint32_t bits;
    std::memcpy(&bits, &f, sizeof bits);      // same value; the pre-C++20 way
    return bits;
}
```

Reading an object's bytes through `unsigned char*` or `std::byte*` is the
rule's escape hatch, so `reinterpret_cast<const unsigned char*>(&n)[0]` is
fine. Punning through a union is UB too (see
[unions](../classes/unions-aggregates.md)). `-fno-strict-aliasing` (GCC, Clang)
turns the optimization off, but the code is still UB in standard C++.

## Pointer provenance

A pointer is more than an address: it points to (or just past) a particular
object, and it can reach only that object.

```cpp body
int a[3]{}, b = 0;
int* end = a + 3;                          // fine to form, compare and subtract
// *end = 1;                               // UB: past the end isn't an element
bool same = (end == &b);                   // unspecified, even if b sits right after a
bool before = std::less<int*>{}(a, &b);    // a total order, where < is unspecified
```

- A **one-past-the-end** pointer never points to the unrelated object that may
  live at that address, even when the two compare equal.
- Comparing pointers to **different objects** with `<` gives an unspecified
  result, and `==` against a one-past-the-end pointer of another object is
  unspecified too. `std::less<T*>` gives a strict total order, so use it for
  sorting or keying by address.
- Integer round trips through `std::uintptr_t`, and how far provenance follows
  a pointer, are still being specified (see P2434).

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
- **`reinterpret_cast` punning breaks strict aliasing.** Copy the bits with
  `std::bit_cast` or `std::memcpy` instead.
- **Equal addresses aren't the same pointer.** A one-past-the-end pointer that
  compares equal to `&b` still can't be used to reach `b`.
- **`int main`, not `void main`.** `void main` is non-standard despite some
  compilers tolerating it.

## See also

- [Object — cppreference](https://en.cppreference.com/w/cpp/language/object)
- [Lifetime — cppreference](https://en.cppreference.com/w/cpp/language/lifetime)
- [Reference initialization (lifetime of a temporary) — cppreference](https://en.cppreference.com/w/cpp/language/reference_initialization#Lifetime_of_a_temporary)
- [std::is_implicit_lifetime — cppreference](https://en.cppreference.com/w/cpp/types/is_implicit_lifetime)
- [Object: strict aliasing — cppreference](https://en.cppreference.com/w/cpp/language/object#Strict_aliasing)
- [reinterpret_cast: type accessibility — cppreference](https://en.cppreference.com/w/cpp/language/reinterpret_cast#Type_accessibility)
- [Pointer total order — cppreference](https://en.cppreference.com/w/cpp/language/operator_comparison#Pointer_total_order)
- [\[basic.lval\]/11: type-accessible — C++ draft](https://eel.is/c++draft/basic.lval#11)
- [\[basic.compound\]/3: pointer values — C++ draft](https://eel.is/c++draft/basic.compound#3)
- [P2434R5: Nondeterministic pointer provenance](https://wg21.link/p2434r5)
- [P0593R6: Implicit creation of objects for low-level object manipulation](https://wg21.link/p0593r6)
- [main function — cppreference](https://en.cppreference.com/w/cpp/language/main_function)
