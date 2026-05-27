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

auto p = std::make_unique<Widget>(args...);   // preferred: exception-safe, no naked new
std::unique_ptr<Widget> q{new Widget};        // works, but prefer make_unique

p->method();                     // use like a pointer
*p;                              // deref
Widget* raw = p.get();           // borrow without transferring ownership
```

### Ownership transfer (move-only)

```cpp
std::unique_ptr<Widget> a = std::make_unique<Widget>();
// std::unique_ptr<Widget> b = a;          // ERROR: not copyable
std::unique_ptr<Widget> b = std::move(a);  // OK: a is now null, b owns it

sink(std::move(b));              // pass ownership into a function
return p;                        // returning by value moves out (no std::move needed)
```

### As a class member = automatic cleanup

```cpp
class Owner {
    std::unique_ptr<Impl> impl_;     // freed automatically when Owner is destroyed
public:
    Owner() : impl_(std::make_unique<Impl>()) {}
    // no destructor, no delete, no leak — the pImpl idiom
};
```

### Arrays and custom deleters

```cpp
auto arr = std::make_unique<int[]>(100);     // unique_ptr<int[]>: uses delete[]
arr[0] = 1;

std::unique_ptr<FILE, decltype(&std::fclose)> f{std::fopen("x","r"), &std::fclose};
// custom deleter → RAII for C resources
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

## See also

- [std::unique_ptr — cppreference](https://en.cppreference.com/w/cpp/memory/unique_ptr)
- [std::make_unique — cppreference](https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique)
