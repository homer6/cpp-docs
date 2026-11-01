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
template<class A, class... Args>
void make_one(A& a, Args&&... args) {
    using Tr = std::allocator_traits<A>;
    auto p = Tr::allocate(a, 1);                       // Tr::pointer (T* for std::allocator)
    Tr::construct(a, p, std::forward<Args>(args)...);  // placement-construct
    Tr::destroy(a, p);
    Tr::deallocate(a, p, 1);
}
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

## scoped_allocator_adaptor: one allocator for nested containers

**Header:** `<scoped_allocator>` · **Since:** C++11. A
`vector<string>` with a custom allocator only uses it for the vector's own
array; each string still allocates however it's told. Wrapping the outer
allocator in `std::scoped_allocator_adaptor` makes the container pass it down
to every element it constructs (uses-allocator construction), so the strings
use it too.

```cpp
#include <memory>
#include <scoped_allocator>
#include <string>
#include <vector>

template<class T>
struct Arena {                                   // a stateful allocator
    using value_type = T;
    int id;
    explicit Arena(int i) : id(i) {}
    template<class U> Arena(const Arena<U>& o) noexcept : id(o.id) {}
    T* allocate(std::size_t n)          { return std::allocator<T>{}.allocate(n); }
    void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
    template<class U> bool operator==(const Arena<U>& o) const noexcept { return id == o.id; }
};

using Str = std::basic_string<char, std::char_traits<char>, Arena<char>>;
using Outer = std::scoped_allocator_adaptor<Arena<Str>>;

int demo() {
    std::vector<Str, Outer> v{Outer{Arena<Str>{7}}};
    v.emplace_back("a string long enough to allocate");
    return v[0].get_allocator().id;              // 7: the element got the vector's arena
}
```

[PMR](memory-resources.md) does this by default: a `pmr::vector<pmr::string>`
hands its resource to every string, with no adaptor. Reach for
`scoped_allocator_adaptor` when the allocator is a custom, non-PMR type.

## allocate_at_least (C++23)

Allocators often round a request up (to a size class or a page). With
`allocate_at_least(n)` they can say so: it returns a
`std::allocation_result{ptr, count}` with `count >= n`, and the container can
use the extra room instead of reallocating sooner. Free the block with the
`count` you got back. Call it through `std::allocator_traits`, which falls
back to `allocate(n)` (and `count == n`) for allocators that don't provide it.

```cpp needs=allocate_at_least
#include <memory>

void demo() {
    std::allocator<int> a;
    using Tr = std::allocator_traits<std::allocator<int>>;
    auto [p, count] = Tr::allocate_at_least(a, 5);   // count >= 5
    Tr::deallocate(a, p, count);                     // deallocate with count, not 5
}
```

Clang 22 (libc++) and GCC 16 have it; GCC 15 doesn't (`__cpp_lib_allocate_at_least`
is `202302L` where it's present). There's no free function
`std::allocate_at_least`; P2652R2 moved it into `allocator_traits` before C++23
shipped.

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
- [std::scoped_allocator_adaptor — cppreference](https://en.cppreference.com/w/cpp/memory/scoped_allocator_adaptor)
- [std::allocator_traits::allocate_at_least — cppreference](https://en.cppreference.com/w/cpp/memory/allocator_traits/allocate_at_least) · [P0401R6](https://wg21.link/P0401R6) · [P2652R2](https://wg21.link/P2652R2)
