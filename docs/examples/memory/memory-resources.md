# Polymorphic memory resources (PMR)

> Choose allocation *strategy* at runtime without changing the container's type —
> the practical, low-boilerplate alternative to template allocators.
> **Header:** `<memory_resource>` · **Since:** C++17

## Mental model

A `std::pmr::memory_resource` is an abstract base with virtual `allocate` /
`deallocate`. `std::pmr::polymorphic_allocator<T>` holds a *pointer* to one and
forwards to it. So all `std::pmr::vector<int>` share **one** type regardless of
which resource backs them — the strategy is a runtime object, not a template
parameter.

```
 pmr::vector<int> ──holds──▶ polymorphic_allocator ──▶ memory_resource*
                                                          ├─ monotonic_buffer_resource
                                                          ├─ (un)synchronized_pool_resource
                                                          └─ new_delete_resource (default)
```

## The standard resources

| Resource | Behavior |
|---|---|
| `std::pmr::new_delete_resource()` | default; wraps `new`/`delete` |
| `std::pmr::monotonic_buffer_resource` | allocate-only; frees everything at once on destruction — extremely fast |
| `std::pmr::synchronized_pool_resource` | pooled, thread-safe |
| `std::pmr::unsynchronized_pool_resource` | pooled, single-thread (faster) |
| `std::pmr::null_memory_resource()` | always throws — caps a buffer so overflow fails loudly |

## Example: a stack buffer + monotonic arena

```cpp
#include <memory_resource>
#include <vector>

std::byte buffer[4096];
std::pmr::monotonic_buffer_resource arena{buffer, sizeof buffer};

std::pmr::vector<int> v{&arena};      // allocates out of `buffer` — no heap, no per-element free
for (int i = 0; i < 100; ++i) v.push_back(i);

std::pmr::string s{"hello", &arena};  // same arena; all reclaimed when `arena` dies
```

When `arena` is destroyed, every allocation it made is released in one shot — no
per-object deallocation, ideal for request-scoped or frame-scoped work.

## Gotchas

- **`pmr` containers default to `new_delete_resource`** if you don't pass one —
  so forgetting the `&arena` argument silently uses the heap.
- **Lifetime: the resource must outlive every container using it.** A
  `monotonic_buffer_resource` over a stack buffer is a dangling trap if the
  buffer goes out of scope first.
- **`monotonic_buffer_resource` never frees individual allocations** — memory
  grows until the resource dies. That's the point (speed), but watch total usage
  in long-lived arenas.
- **`pmr::vector<T>` ≠ `std::vector<T>`.** They're different types (different
  allocator), so they don't implicitly interconvert.

## See also

- [std::pmr::memory_resource — cppreference](https://en.cppreference.com/w/cpp/memory/memory_resource)
- [std::pmr::polymorphic_allocator — cppreference](https://en.cppreference.com/w/cpp/memory/polymorphic_allocator)
- [std::pmr::monotonic_buffer_resource — cppreference](https://en.cppreference.com/w/cpp/memory/monotonic_buffer_resource)
