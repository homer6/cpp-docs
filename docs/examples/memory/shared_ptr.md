# std::shared_ptr

> A reference-counted smart pointer for **shared ownership**: the object is freed
> when the last `shared_ptr` to it goes away. **Header:** `<memory>` · **Since:** C++11

## Mental model

The pointer plus a heap-allocated **control block** holding a *strong* count
(owners) and a *weak* count (observers). Copying a `shared_ptr` bumps the strong
count; destroying one decrements it; at zero, the object is destroyed.

```
 sp1 ┐
 sp2 ┼──▶ [ control block: strong=3, weak=1 ] ──▶ the object
 sp3 ┘
```

The count is **atomic** (thread-safe to copy/destroy from multiple threads), but
that makes copies more expensive than a `unique_ptr` move.

## Examples

### Create with make_shared

```cpp
#include <memory>

auto p = std::make_shared<Widget>(args...);   // one allocation for object + control block
std::shared_ptr<Widget> q = p;                 // strong count → 2
p.use_count();                                  // 2 (approximate; for debugging)

p->method();
q.reset();                                      // count → 1; object still alive via p
```

### Sharing into structures/threads

```cpp
std::vector<std::shared_ptr<Node>> graph;
auto n = std::make_shared<Node>();
graph.push_back(n);              // both `n` and the vector co-own the node
// the Node lives until BOTH are gone
```

### Custom deleter / aliasing

```cpp
std::shared_ptr<FILE> f{std::fopen("x","r"), &std::fclose};   // deleter stored in control block

// aliasing constructor: share ownership of `parent`, but point at a sub-object
std::shared_ptr<Field> fld{parent, &parent->field};
```

## Gotchas

- **It's not free — don't default to it.** Atomic refcount traffic, a control
  block, and larger size than a raw/`unique_ptr`. Use `unique_ptr` unless
  ownership is *genuinely* shared. Passing `shared_ptr` by value everywhere is a
  common performance smell — pass `const T&`/raw pointer when you don't take
  ownership.
- **Cycles leak.** Two objects holding `shared_ptr` to each other never reach
  count 0. Break the cycle with [`weak_ptr`](weak_ptr.md) for the back-edge.
- **Prefer `make_shared`** (single allocation, exception-safe) — but note it
  keeps the *object* storage alive as long as any `weak_ptr` exists (object and
  control block share one block). With huge objects + long-lived weak_ptrs, the
  separate-allocation `shared_ptr<T>(new T)` can release object memory sooner.
- **The refcount is atomic; the pointed-to object is not.** Concurrent reads are
  fine; concurrent mutation of `*sp` still needs your own synchronization.
- **Never make two independent control blocks for one object**
  (`shared_ptr<T> a{raw}; shared_ptr<T> b{raw};`) → double free. Copy the
  `shared_ptr`, or use `enable_shared_from_this`.

## See also

- [std::shared_ptr — cppreference](https://en.cppreference.com/w/cpp/memory/shared_ptr)
- [std::make_shared — cppreference](https://en.cppreference.com/w/cpp/memory/shared_ptr/make_shared)
- [std::enable_shared_from_this — cppreference](https://en.cppreference.com/w/cpp/memory/enable_shared_from_this)
