# Raw memory utilities

> Low-level helpers for working with uninitialized storage and object lifetime —
> the tools containers and allocators are built from.
> **Header:** `<memory>` (mostly) · **Since:** C++11+

> You rarely need these directly; they matter when you implement containers,
> allocators, or other storage abstractions.

## std::addressof — the real address

Returns an object's true address even if it overloaded `operator&`.

```cpp
template<class T>
T* real_address(T& obj) {
    return std::addressof(obj);     // safe even when &obj is hijacked
}
```

## Constructing into raw storage

`std::construct_at` / `std::destroy_at` (C++17/20) placement-construct and
destroy a single object — and unlike a bare `new (p) T{...}`, `construct_at` is
usable in `constexpr`.

```cpp
template<class T, class... Args>
void in_place(Args&&... args) {
    alignas(T) std::byte buf[sizeof(T)];

    // like placement new, constexpr-friendly; use the returned pointer
    T* p = std::construct_at(reinterpret_cast<T*>(buf), std::forward<Args>(args)...);
    p->use();
    std::destroy_at(p);              // run the destructor (does not free buf)
}
```

## std::launder (C++17)

`std::launder(p)` (`<new>`) returns a pointer to the object that now lives at
`p`'s address, for when `p` itself still points to an older object there. You
need it only when the old object isn't *transparently replaceable* by the new
one:

- the new object has a different type, or you reach it through a pointer that
  `reinterpret_cast` made from the buffer rather than the one placement `new`
  returned;
- the old object was a `const` complete object;
- the old object was a base-class subobject.

```cpp
#include <new>

int reuse() {
    alignas(int) std::byte buf[sizeof(int)];
    new (buf) int{7};                                    // result thrown away
    int a = *std::launder(reinterpret_cast<int*>(buf));  // 7; without launder, UB
    int* p = new (buf) int{8};                           // simpler: keep this pointer
    return a + *p;                                       // 15
}
```

You **don't** need it when you use the pointer that placement `new` or
`construct_at` returned, or when you replace a non-`const` object with one of
the same type. Since C++20 (P1971R0), that includes classes with `const` or
reference members, so the old name keeps working:

```cpp
struct Id { const int n; };

int replace() {
    Id id{1};
    std::destroy_at(&id);
    std::construct_at(&id, 2);   // same type, storage exactly overlaid
    return id.n;                 // 2, no launder needed since C++20
}
```

`launder` doesn't create an object (that's `construct_at` or
`start_lifetime_as`), and it doesn't make a strict-aliasing violation legal: an
object of type `T` must already be at the address.

## Bulk uninitialized algorithms

Construct/move/copy ranges into raw memory, with proper cleanup if an element's
constructor throws:

```cpp
template<class It, class T>
void bulk(It first, It last, T* raw_out, T* raw_first, T* raw_last, const T& val) {
    std::uninitialized_copy(first, last, raw_out);     // copy-construct into raw region
    std::uninitialized_move(first, last, raw_out);     // move-construct (C++17)
    std::uninitialized_fill(raw_first, raw_last, val);
    std::uninitialized_default_construct(raw_first, raw_last);  // C++17
    std::destroy(first, last);                          // destroy a range (C++17)
}
```

These are exactly what a `vector` uses when it grows and relocates elements.

## std::start_lifetime_as (C++23)

Begins an object's lifetime in existing suitably-aligned storage *without*
running a constructor — the well-defined way to interpret a byte buffer (e.g.
from I/O) as a trivially-copyable type, replacing UB-prone `reinterpret_cast`.

```cpp needs=start_lifetime_as
struct PacketHeader { std::uint16_t type, length; };     // trivially copyable
alignas(PacketHeader) std::byte buffer[512];              // filled by I/O

auto* hdr = std::start_lifetime_as<PacketHeader>(buffer); // no ctor, defined behavior
```

It is the explicit form of [implicit object
creation](../language/basic-concepts/object-model.md#implicit-lifetime-types),
for bytes that arrived without `malloc` or `memcpy`. The object keeps the bytes
already in the buffer. GCC 16 ships it (`__cpp_lib_start_lifetime_as`
`202207L`); GCC 15 and Clang 22 with libc++ don't.

## Gotchas

- **`construct_at`/placement `new` do not allocate**, and `destroy_at` does not
  deallocate — storage management is separate. Pair them with the matching
  allocate/deallocate.
- **Don't `delete` placement-constructed objects.** Call the destructor
  (`destroy_at`) and release the raw storage however you obtained it.
- **`reinterpret_cast`-ing a byte buffer to a `T*` and reading is UB** until an
  object of `T` exists there — that's why `start_lifetime_as` / `construct_at`
  / `std::bit_cast` exist. Reach for those instead.
- **Reusing storage through the old pointer may need `std::launder`.** Keep
  the pointer that placement `new` returns and you never will.
- **Alignment matters.** Raw storage must satisfy `alignof(T)` (use `alignas` or
  `std::aligned_storage`'s successor patterns) before constructing a `T` in it.

## See also

- [std::construct_at — cppreference](https://en.cppreference.com/w/cpp/memory/construct_at)
- [std::uninitialized_copy — cppreference](https://en.cppreference.com/w/cpp/memory/uninitialized_copy)
- [std::start_lifetime_as — cppreference](https://en.cppreference.com/w/cpp/memory/start_lifetime_as)
- [std::launder — cppreference](https://en.cppreference.com/w/cpp/utility/launder)
- [Lifetime: storage reuse — cppreference](https://en.cppreference.com/w/cpp/language/lifetime#Storage_reuse)
- [\[ptr.launder\] — C++ draft](https://eel.is/c++draft/ptr.launder)
- [\[basic.life\]/9: transparently replaceable — C++ draft](https://eel.is/c++draft/basic.life#9)
- [P1971R0: Core NB comment resolutions (RU007: const and reference members)](https://wg21.link/p1971r0)
- [std::addressof — cppreference](https://en.cppreference.com/w/cpp/memory/addressof)
