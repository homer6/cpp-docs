# Raw memory utilities

> Low-level helpers for working with uninitialized storage and object lifetime —
> the tools containers and allocators are built from.
> **Header:** `<memory>` (mostly) · **Since:** C++11+

> You rarely need these directly; they matter when you implement containers,
> allocators, or other storage abstractions.

## std::addressof — the real address

Returns an object's true address even if it overloaded `operator&`.

```cpp
T* p = std::addressof(obj);     // safe even when &obj is hijacked
```

## Constructing into raw storage

`std::construct_at` / `std::destroy_at` (C++17/20) placement-construct and
destroy a single object — and unlike a bare `new (p) T{...}`, `construct_at` is
usable in `constexpr`.

```cpp
alignas(T) std::byte buf[sizeof(T)];
T* p = reinterpret_cast<T*>(buf);

std::construct_at(p, args...);   // like placement new, constexpr-friendly
p->use();
std::destroy_at(p);              // run the destructor (does not free buf)
```

## Bulk uninitialized algorithms

Construct/move/copy ranges into raw memory, with proper cleanup if an element's
constructor throws:

```cpp
std::uninitialized_copy(first, last, raw_out);     // copy-construct into raw region
std::uninitialized_move(first, last, raw_out);     // move-construct (C++17)
std::uninitialized_fill(raw_first, raw_last, val);
std::uninitialized_default_construct(raw_first, raw_last);  // C++17
std::destroy(first, last);                          // destroy a range (C++17)
```

These are exactly what a `vector` uses when it grows and relocates elements.

## std::start_lifetime_as (C++23)

Begins an object's lifetime in existing suitably-aligned storage *without*
running a constructor — the well-defined way to interpret a byte buffer (e.g.
from I/O) as a trivially-copyable type, replacing UB-prone `reinterpret_cast`.

```cpp
auto* hdr = std::start_lifetime_as<PacketHeader>(buffer);   // no ctor, defined behavior
```

## Gotchas

- **`construct_at`/placement `new` do not allocate**, and `destroy_at` does not
  deallocate — storage management is separate. Pair them with the matching
  allocate/deallocate.
- **Don't `delete` placement-constructed objects.** Call the destructor
  (`destroy_at`) and release the raw storage however you obtained it.
- **`reinterpret_cast`-ing a byte buffer to a `T*` and reading is UB** until an
  object of `T` exists there — that's why `start_lifetime_as` / `construct_at`
  / `std::bit_cast` exist. Reach for those instead.
- **Alignment matters.** Raw storage must satisfy `alignof(T)` (use `alignas` or
  `std::aligned_storage`'s successor patterns) before constructing a `T` in it.

## See also

- [std::construct_at — cppreference](https://en.cppreference.com/w/cpp/memory/construct_at)
- [std::uninitialized_copy — cppreference](https://en.cppreference.com/w/cpp/memory/uninitialized_copy)
- [std::start_lifetime_as — cppreference](https://en.cppreference.com/w/cpp/memory/start_lifetime_as)
- [std::addressof — cppreference](https://en.cppreference.com/w/cpp/memory/addressof)
