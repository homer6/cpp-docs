# std::weak_ptr

> A non-owning observer of a [`shared_ptr`](shared_ptr.md)-managed object: it can
> tell whether the object is still alive and temporarily take ownership, but it
> never keeps it alive. **Header:** `<memory>` · **Since:** C++11

## Mental model

A `weak_ptr` points at the same control block as a `shared_ptr` but increments
only the *weak* count, not the strong count. So it doesn't prevent destruction.
To use the object you must `lock()` it, which atomically returns a `shared_ptr`
(promoting to owner) if the object still exists, or an empty one if it's gone.

## Examples

### Observe without owning

```cpp
#include <memory>

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
std::unordered_map<Key, std::weak_ptr<Value>> cache;
// entries don't keep values alive; lock() to fetch, re-create on miss/expired
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

## See also

- [std::weak_ptr — cppreference](https://en.cppreference.com/w/cpp/memory/weak_ptr)
- [std::weak_ptr::lock — cppreference](https://en.cppreference.com/w/cpp/memory/weak_ptr/lock)
