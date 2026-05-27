# Allocators

> The policy objects that decide *where* and *how* a container gets its raw
> storage, decoupling "what to store" from "how to allocate."
> **Header:** `<memory>` · **Since:** C++11 (modern model)

## Mental model

Every standard container has a trailing `Allocator` template parameter defaulting
to `std::allocator<T>` (which just calls `::operator new`/`delete`). An allocator
answers two questions: **allocate(n)** raw memory for n objects, and
**deallocate** it. Construction/destruction of the objects is handled separately
(via `std::allocator_traits`), so allocators deal in *storage*, not values.

```
 vector<T, A>   →   A::allocate(n)  →  raw bytes
                    construct each element into them
                    A::deallocate   ←  on destruction
```

## allocator_traits — the indirection layer

Containers never call an allocator directly; they go through
`std::allocator_traits<A>`. That lets you write a **minimal** allocator (just
`value_type`, `allocate`, `deallocate`) and have traits supply sensible defaults
for everything else (`construct`, `max_size`, rebind, propagation flags).

```cpp
using Tr = std::allocator_traits<A>;
T* p = Tr::allocate(a, 1);
Tr::construct(a, p, args...);    // placement-construct
Tr::destroy(a, p);
Tr::deallocate(a, p, 1);
```

## A minimal C++11+ allocator

```cpp
template<class T>
struct MyAlloc {
    using value_type = T;
    MyAlloc() = default;
    template<class U> MyAlloc(const MyAlloc<U>&) noexcept {}

    T* allocate(std::size_t n)            { return static_cast<T*>(::operator new(n * sizeof(T))); }
    void deallocate(T* p, std::size_t)    { ::operator delete(p); }

    template<class U> bool operator==(const MyAlloc<U>&) const noexcept { return true; }
};

std::vector<int, MyAlloc<int>> v;     // container uses your allocator
```

## Gotchas

- **Most code never writes one.** Custom allocators are for arenas, pools,
  shared memory, instrumentation, or embedded constraints. For "use a different
  allocation strategy at runtime," prefer [PMR](memory-resources.md) — far less
  template churn.
- **Stateful allocators add rules.** Whether the allocator is copied/moved with
  the container is governed by `propagate_on_container_*` traits and
  `is_always_equal`; get these wrong and you get surprising deep copies or UB.
- **The allocator type is part of the container type.** `vector<int>` and
  `vector<int, MyAlloc<int>>` are different, non-interoperable types — another
  reason PMR (which keeps one container type) is often nicer.
- **Pre-C++17 allocators carried lots of boilerplate** (`pointer`, `rebind`,
  etc.); `allocator_traits` made nearly all of it optional. Don't copy old
  examples wholesale.

## See also

- [std::allocator — cppreference](https://en.cppreference.com/w/cpp/memory/allocator)
- [std::allocator_traits — cppreference](https://en.cppreference.com/w/cpp/memory/allocator_traits)
- [Allocator named requirement — cppreference](https://en.cppreference.com/w/cpp/named_req/Allocator)
