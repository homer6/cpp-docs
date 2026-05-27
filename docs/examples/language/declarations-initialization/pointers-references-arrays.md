# Pointers, references & arrays

> The built-in indirection and aggregation types — and why modern C++ wraps them
> in safer abstractions.

## Pointers

A pointer holds an address (or `nullptr`). It's nullable and rebindable.

```cpp
int x = 10;
int* p = &x;          // points at x
*p = 20;              // dereference to read/write
p = nullptr;          // null — always use nullptr, never 0 or NULL

int* arr_ptr = new int[5];     // raw owning pointer — avoid in modern code
delete[] arr_ptr;              // ...manual delete is a leak/double-free risk
```

Prefer **smart pointers** for ownership ([`unique_ptr`/`shared_ptr`](../../memory/README.md))
and a **raw pointer or reference only for non-owning "just point at it"**.

## References

An alias for an existing object — **not nullable, not rebindable** (it always
refers to whatever it was bound to).

```cpp
int& r = x;           // r is another name for x
r = 30;               // changes x

void inc(int& n) { ++n; }          // pass by reference: modify the caller's object
void read(const std::string& s);   // const ref: no copy, no mutation

int&& rr = 5;          // rvalue reference — binds to temporaries (move semantics)
```

References vs pointers: use a **reference** when it must always refer to a valid
object and never rebind; use a **pointer** when it can be null or repointed. See
[value categories](../expressions/value-categories.md) for `&` vs `&&`.

## Arrays (and why to avoid raw ones)

```cpp
int a[5] = {1, 2, 3};        // raw array; a[3]=a[4]=0
int n = std::size(a);         // 5 (C++17) — better than sizeof(a)/sizeof(a[0])

void f(int arr[], int n);     // arrays DECAY to pointers when passed — size is lost!
```

Raw arrays decay to pointers (losing the size) and don't bounds-check. Prefer:

- [`std::array<T, N>`](../../containers/sequence/array.md) — fixed size, no decay, value semantics.
- [`std::vector<T>`](../../containers/sequence/vector.md) — dynamic size.
- [`std::span<T>`](../../containers/views/span.md) — a non-owning (pointer, length) view to pass contiguous data safely.

## Gotchas

- **Use `nullptr`, not `0`/`NULL`.** `nullptr` has a real type (`std::nullptr_t`)
  and resolves overloads correctly; `NULL` is just `0` and picks `int` overloads.
- **Arrays decay to pointers on the slightest provocation** — passing to a
  function loses the length. Pass a `std::span` (or `array&`) to keep the size.
- **Dangling references/pointers** are the classic UB: never return a
  reference/pointer to a local, and don't keep one to an element of a container
  you then grow (see each container's invalidation rules).
- **Don't `new`/`delete` by hand.** Use `make_unique`/`make_shared` and containers;
  raw owning pointers leak on exceptions and double-free on mistakes.
- **A reference must be initialized** and can't be reseated — there are no "null
  references."

## See also

- [Pointer declaration — cppreference](https://en.cppreference.com/w/cpp/language/pointer)
- [Reference declaration — cppreference](https://en.cppreference.com/w/cpp/language/reference)
- [Array declaration — cppreference](https://en.cppreference.com/w/cpp/language/array)
