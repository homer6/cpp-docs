# std::unique_ptr

> A smart pointer with **sole ownership** of a heap object, freed automatically
> when the pointer dies. Zero runtime overhead. **Header:** `<memory>` · **Since:** C++11

## Mental model

A raw pointer that owns what it points to and runs `delete` (or your custom
deleter) in its destructor. It's **move-only**: copying is forbidden, so there's
always exactly one owner. The default-deleter version is the same size as a raw
pointer and just as fast — RAII for free.

## Examples

### Create with make_unique

```cpp
#include <memory>

void demo() {
    auto p = std::make_unique<Widget>(42);        // preferred: exception-safe, no naked new
    std::unique_ptr<Widget> q{new Widget};        // works, but prefer make_unique

    p->use();                        // use like a pointer
    Widget& w = *p;                  // deref
    Widget* raw = p.get();           // borrow without transferring ownership
}
```

### Ownership transfer (move-only)

```cpp
void sink(std::unique_ptr<Widget> w);      // takes ownership

std::unique_ptr<Widget> transfer() {
    std::unique_ptr<Widget> a = std::make_unique<Widget>();
    // std::unique_ptr<Widget> b = a;          // ERROR: not copyable
    std::unique_ptr<Widget> b = std::move(a);  // OK: a is now null, b owns it

    sink(std::move(b));              // pass ownership into a function

    auto p = std::make_unique<Widget>();
    return p;                        // returning by value moves out (no std::move needed)
}
```

### As a class member = automatic cleanup

```cpp
struct Impl { std::vector<int> data; };

class Owner {
    std::unique_ptr<Impl> impl_;     // freed automatically when Owner is destroyed
public:
    Owner() : impl_(std::make_unique<Impl>()) {}
    // no destructor, no delete, no leak
};
```

For the pImpl idiom proper, `Impl` is only forward-declared in the header, so
declare `~Owner();` there and define it (`Owner::~Owner() = default;`) in the
`.cpp` where `Impl` is complete — `unique_ptr`'s deleter needs the full type.

### Arrays and custom deleters

```cpp body
auto arr = std::make_unique<int[]>(100);     // unique_ptr<int[]>: uses delete[]
arr[0] = 1;

std::unique_ptr<FILE, decltype(&std::fclose)> f{std::fopen("x","r"), &std::fclose};
// custom deleter → RAII for C resources (not called if fopen returned null)
```

### C APIs that return through T**: out_ptr and inout_ptr (C++23)

Many C APIs hand back a resource through an output parameter
(`int create(handle** out)`). `std::out_ptr(p)` makes a temporary that converts
to `T**` (or `void**`); when the full expression ends, it `reset()`s `p` with
whatever the API wrote. `std::inout_ptr(p)` is for APIs that also *read* the
old pointer and may free or replace it (`realloc`-style): it releases `p`
first, passes the old value in, and adopts the result.

```cpp
#include <cstdlib>
#include <memory>

struct handle { int v; };
int handle_create(handle** out, int v) {             // C API: fills *out
    *out = static_cast<handle*>(std::malloc(sizeof(handle)));
    (*out)->v = v;
    return 0;
}
int handle_grow(handle** io) {                        // C API: frees *io, writes a new one
    auto* n = static_cast<handle*>(std::malloc(sizeof(handle)));
    n->v = (*io)->v * 10;
    std::free(*io);
    *io = n;
    return 0;
}
struct handle_free { void operator()(handle* h) const { std::free(h); } };

int demo() {
    std::unique_ptr<handle, handle_free> h;
    handle_create(std::out_ptr(h), 7);   // h owns the new handle; h->v == 7
    handle_grow(std::inout_ptr(h));      // h owns the replacement; h->v == 70
    return h->v;
}
```

`out_ptr` works with `shared_ptr` too, but you must pass the deleter
(`std::out_ptr(sp, handle_free{})`), because `shared_ptr` can't learn it later.
`inout_ptr` refuses `shared_ptr` outright: other owners may share the old
pointer, so it can't be released.

```cpp compile-fail="inout_ptr"
#include <memory>

int api(int** io);
void f(std::shared_ptr<int>& sp) { api(std::inout_ptr(sp)); }   // static_assert fires
```

## Gotchas

- **Always prefer `make_unique` over `new`.** It's exception-safe (no leak if an
  argument expression throws) and never lets a raw `new` escape.
- **`release()` vs `reset()`:** `release()` returns the raw pointer and gives up
  ownership *without* deleting (you must free it); `reset()` deletes the current
  object (and optionally adopts a new one). Mixing them up leaks or double-frees.
- **A custom deleter can change the type's size.** A function-pointer deleter
  makes the `unique_ptr` bigger than a raw pointer; a stateless functor deleter
  keeps it pointer-sized.
- **Don't build two owners from one raw pointer.** `unique_ptr<T> a{raw}, b{raw};`
  double-frees. One raw pointer, one owner.
- **Don't keep an `out_ptr` temporary around.** It updates the smart pointer in
  its destructor, at the end of the full expression; until then the smart
  pointer doesn't hold the new value. Use it only as a call argument. It also
  resets the smart pointer before the call, so the old object is freed first.
- **Need a copyable `unique_ptr`?** That's [`std::indirect`](indirect-polymorphic.md)
  (C++26): heap-allocated, but copies deep-copy.

## See also

- [std::unique_ptr — cppreference](https://en.cppreference.com/w/cpp/memory/unique_ptr)
- [std::make_unique — cppreference](https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique)
- [std::out_ptr — cppreference](https://en.cppreference.com/w/cpp/memory/out_ptr_t/out_ptr) · [std::inout_ptr — cppreference](https://en.cppreference.com/w/cpp/memory/inout_ptr_t/inout_ptr)
- [P1132R8: out_ptr — a scalable output pointer abstraction](https://wg21.link/P1132R8)
