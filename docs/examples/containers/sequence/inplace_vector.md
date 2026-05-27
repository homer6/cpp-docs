# std::inplace_vector

> A dynamically-resizable vector with a **fixed compile-time capacity** and **no
> heap allocation** — the elements live inside the object.
> **Header:** `<inplace_vector>` · **Since:** C++26

## Mental model

`vector` semantics (push, pop, size grows and shrinks) but with `array`-style
storage: a buffer of room for `N` elements baked into the object, plus a runtime
`size` counter from 0…N. Think "a vector that can never reallocate because it
already owns its (capped) storage."

```
 inplace_vector<int, 6>, size = 3
 ┌───┬───┬───┬┄┄┄┬┄┄┄┬┄┄┄┐
 │ a │ b │ c │   │   │   │   capacity N = 6 (inline, no heap)
 └───┴───┴───┴┄┄┄┴┄┄┄┴┄┄┄┘
   0   1   2     ↑ unused
```

It fills the gap between `array` (size fixed *and* must equal N) and `vector`
(size dynamic, but heap-allocated).

## Declaration

```cpp
template<class T, std::size_t N>
struct inplace_vector;          // no allocator parameter
```

- `T` — element type.
- `N` — the **maximum** number of elements (the capacity). `N == 0` is allowed.

`capacity()` and `max_size()` both return `N` (constexpr, `noexcept`).

## Complexity

| Operation | Cost |
|---|---|
| `v[i]`, `at`, `front`, `back`, `data` | O(1) |
| `push_back` / `pop_back` (within capacity) | O(1) — **never reallocates** |
| `insert` / `erase` in the middle | O(n) |

## Inserting when full — three flavors

Because capacity is hard-capped at `N`, overflow has to be handled. The library
gives you three back-insert families with different overflow behavior:

| Function | On overflow (`size() == N`) | Returns |
|---|---|---|
| `push_back` / `emplace_back` | **throws `std::bad_alloc`** | `reference` to new element |
| `try_push_back` / `try_emplace_back` | does nothing, no throw | `pointer` — `nullptr` if full |
| `unchecked_push_back` / `unchecked_emplace_back` | **undefined behavior** (precondition `size() < N`) | `reference` |

Choose by context: `push_back` for "shouldn't happen, fail loudly",
`try_push_back` for "full is normal, branch on it", `unchecked_push_back` for hot
paths where you've already proven there's room.

## Iterator & reference invalidation

Like `vector`'s rules but **without the reallocation case** (it can't realloc):

- `push_back` etc.: invalidate `end()`; existing elements don't move, so
  iterators/refs before the new end stay valid.
- `insert`/`erase` in the middle: invalidate iterators/refs at and after the
  point (elements shift).
- Moving/swapping the whole `inplace_vector` invalidates iterators (the storage
  is inline, so it physically moves with the object — unlike `vector`, whose heap
  buffer is just re-pointed).

## When to use it

- **No-allocation environments**: embedded, real-time, kernel, hot paths where
  you must not touch the heap, or `constexpr` code.
- A buffer with a **known worst-case size** but a varying actual count (e.g. "up
  to 16 results").
- **Avoid** when the cap is unknown or large (inline storage bloats the object
  and can blow the stack) — use `vector`. When size always equals `N`, use
  `array`.

## Examples

```cpp
#include <inplace_vector>

std::inplace_vector<int, 4> v;     // capacity 4, size 0, no heap
v.push_back(1);
v.push_back(2);                    // size 2
v.capacity();                      // == 4 (constexpr)

// Overflow handling without exceptions:
std::inplace_vector<int, 2> small;
small.push_back(1);
small.push_back(2);
if (int* p = small.try_push_back(3)) {   // returns nullptr — it's full
    // not taken
} else {
    // handle "buffer full" without an exception
}

// push_back would throw here:
try { small.push_back(3); }
catch (const std::bad_alloc&) { /* size() == capacity() */ }
```

### constexpr use

```cpp
constexpr auto build = []{
    std::inplace_vector<int, 8> r;
    for (int i = 0; i < 5; ++i) r.push_back(i * i);
    return r;
}();
static_assert(build.size() == 5 && build[4] == 16);   // computed at compile time
```

## Gotchas

- **`push_back` throws `std::bad_alloc`, not a logic_error**, when full — it
  reuses the allocation-failure channel even though no allocation happened.
- **`unchecked_*` is a loaded gun.** No capacity check; overflowing is UB. Only
  use when `size() < N` is guaranteed.
- **`reserve(n)`** is a no-op for `n <= N` and **throws `std::bad_alloc`** for
  `n > N` (it can't grow past `N`). `shrink_to_fit` is a no-op.
- **Moves are not cheap like `vector`'s.** A `vector` move just steals a pointer;
  `inplace_vector` must move each element, because storage is inline.

## See also

- [std::inplace_vector — cppreference](https://en.cppreference.com/w/cpp/container/inplace_vector)
- [try_push_back — cppreference](https://en.cppreference.com/w/cpp/container/inplace_vector/try_push_back)
- [unchecked_push_back — cppreference](https://en.cppreference.com/w/cpp/container/inplace_vector/unchecked_push_back)
