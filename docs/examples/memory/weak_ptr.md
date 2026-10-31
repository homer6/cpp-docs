# std::weak_ptr

> A non-owning observer of a [`shared_ptr`](shared_ptr.md)-managed object: it can
> tell whether the object is still alive and temporarily take ownership, but it
> never keeps it alive. **Header:** `<memory>` · **Since:** C++11

## Mental model

A `weak_ptr` points at the same control block as a `shared_ptr` but increments
only the *weak* count, not the strong count. So it doesn't prevent destruction.
To use the object you must `lock()` it, which atomically returns a `shared_ptr`
(promoting to owner) if the object still exists, or an empty one if it's gone.

```
 shared_ptr sp ──┐                      ┌── weak_ptr wp
                 ▼                      ▼
            ┌──────────────────────────────┐
            │ control block                │      object dies at strong == 0
            │ strong = 1    weak = 1       │──▶   control block dies at
            └──────────────────────────────┘      strong == 0 && weak == 0
```

## Declaration

```cpp
template<class T> class weak_ptr;
```

| Member | Meaning |
|---|---|
| `weak_ptr(const shared_ptr<Y>&)`, `= sp` | Observe what `sp` owns |
| `lock()` | `shared_ptr<T>` that owns the object, or empty if it's gone (atomic) |
| `expired()` | `use_count() == 0` — a hint, not a guarantee in threaded code |
| `use_count()` | Number of `shared_ptr` owners right now |
| `reset()`, `swap` | Stop observing; swap |
| `owner_before` (and C++26 `owner_hash`, `owner_equal` — GCC 16 has them, GCC 15 and Clang 22 don't) | Order/compare by control block, for keys in associative containers |

`shared_ptr<T>(wp)` is the throwing alternative to `lock()`: it throws
`std::bad_weak_ptr` if the object is gone.

## When to use it

- **Fits:** back/parent pointers in a `shared_ptr` graph, caches and registries
  that shouldn't keep entries alive, callbacks that must not extend the lifetime
  of the object they call back into.
- **Anti-fits:** objects with a single, clear owner — use
  [`unique_ptr`](unique_ptr.md) and a raw pointer or reference for observers
  whose lifetime is known to be shorter.

## Examples

### Observe without owning

```cpp body
auto sp = std::make_shared<Widget>();
std::weak_ptr<Widget> wp = sp;          // does NOT bump strong count

if (auto locked = wp.lock()) {          // returns shared_ptr<Widget> or empty
    locked->use();                      // safe: object is alive for this scope
} else {
    // object already destroyed
}

wp.expired();                           // true once the last shared_ptr is gone
```

### Breaking a reference cycle

```cpp
struct Node {
    std::shared_ptr<Node> next;     // forward edge: owns
    std::weak_ptr<Node>   prev;     // back edge: observes → no cycle, no leak
};
```

This is the canonical cure for the `shared_ptr` cycle leak: make the "back" or
"parent" pointer a `weak_ptr`.

### Caches

```cpp
struct Value {};
std::unordered_map<std::string, std::weak_ptr<Value>> cache;
// entries don't keep values alive; lock() to fetch, re-create on miss/expired
```

### A callback that doesn't keep its target alive

```cpp
#include <functional>
#include <memory>

struct Session : std::enable_shared_from_this<Session> {
    void on_data() {}
    std::function<void()> callback() {
        return [self = weak_from_this()] {     // weak_from_this: C++17
            if (auto s = self.lock()) s->on_data();
            // else: the session is gone — do nothing
        };
    }
};
```

## Gotchas

- **You can't dereference a `weak_ptr` directly.** There's no `operator*`/`->`;
  you must `lock()` (or construct a `shared_ptr` from it, which throws
  `std::bad_weak_ptr` if expired). `lock()` is the safe path.
- **`expired()` then use is a race.** In multithreaded code, check-then-use can
  let the object die in between. `lock()` is atomic — use its result directly.
- **It still pins the control block** (not the object). With `make_shared`, the
  object's storage lives until all weak_ptrs are gone too; for very large objects
  that can delay memory release.
- **`weak_from_this()` is empty unless a `shared_ptr` owns the object.** Called
  on a stack or `new`-allocated object, it returns an expired `weak_ptr`.

## See also

- [std::weak_ptr — cppreference](https://en.cppreference.com/w/cpp/memory/weak_ptr)
- [std::weak_ptr::lock — cppreference](https://en.cppreference.com/w/cpp/memory/weak_ptr/lock)
- [std::enable_shared_from_this — cppreference](https://en.cppreference.com/w/cpp/memory/enable_shared_from_this)
