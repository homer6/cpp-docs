# The object model & `main`

> What an "object" is in C++, how its lifetime works, and how a program begins.

## Objects and lifetime

An **object** is a region of storage with a type and a lifetime. Lifetime begins
when construction completes and ends when the destructor runs (or, for trivial
types, when storage is reused/released). Using an object outside its lifetime is
undefined behavior.

```cpp
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

## `main` — the entry point

```cpp
int main() { /* ... */ }                      // implicit `return 0;` if you fall off the end
int main(int argc, char* argv[]) { /* ... */ }// command-line args
```

- Returning from `main` (or `return 0;`) signals success; non-zero signals
  failure. `main` is the **only** function with an implicit `return 0`.
- `argv[0]` is (conventionally) the program name; `argv[argc]` is null.
- You cannot call `main` recursively, take its address, or overload it.

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
- **Static init order across TUs is unspecified.** Use the "construct on first
  use" idiom (a function-local `static`) for cross-TU dependencies.
- **`int main`, not `void main`.** `void main` is non-standard despite some
  compilers tolerating it.

## See also

- [Object — cppreference](https://en.cppreference.com/w/cpp/language/object)
- [Lifetime — cppreference](https://en.cppreference.com/w/cpp/language/lifetime)
- [main function — cppreference](https://en.cppreference.com/w/cpp/language/main_function)
