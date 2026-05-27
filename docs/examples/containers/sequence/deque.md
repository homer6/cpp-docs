# std::deque

> A **d**ouble-**e**nded **que**ue: fast insertion and removal at *both* ends,
> with random access. **Header:** `<deque>` · **Since:** C++98

## Mental model

Not one contiguous block like `vector`, but a sequence of fixed-size **chunks**
("blocks"/"pages") tracked by a central map of pointers. Elements within a chunk
are contiguous; chunks themselves are scattered on the heap. Growing at either
end allocates/links a new chunk without touching the existing ones.

```
 map ──▶ [ ptr ][ ptr ][ ptr ][ ptr ]
            │      │      │      │
            ▼      ▼      ▼      ▼
          [..][.] [....] [....] [.. ]     ← fixed-size chunks
           front                back
```

This is why `operator[]` is still O(1) (compute chunk + offset) but the data is
**not** one contiguous array — `data()` does not exist on `deque`.

## Declaration

```cpp
template<class T, class Allocator = std::allocator<T>>
class deque;
```

## Complexity

| Operation | Cost |
|---|---|
| `d[i]`, `at`, `front`, `back` | O(1) |
| `push_back` / `push_front` / `pop_back` / `pop_front` | O(1) |
| `insert` / `erase` in the middle | O(n) |
| iterate | O(n), but with more overhead than `vector` |

## Iterator & reference invalidation

Subtler than `vector` — and a frequent source of surprise:

- **`push_front` / `push_back`:** invalidate **all iterators**, but **references
  and pointers to existing elements stay valid** (elements don't move).
- **`pop_front` / `pop_back`:** invalidate only iterators/refs to the erased
  element.
- **`insert` / `erase` in the middle:** invalidate **all** iterators, pointers,
  and references.
- `clear` invalidates everything.

So: references survive end-insertions (unlike `vector`), but iterators generally
don't.

## When to use it

- A **FIFO queue or work buffer** where you push at one end and pop at the other.
  (`std::queue` and `std::stack` default to `deque` underneath.)
- You need **random access** *and* cheap growth at the **front**.
- **Avoid** when you need contiguous storage / `data()` (use `vector`), or when
  you mostly append at the back and iterate hot — `vector` has better locality
  and less per-element overhead.

## Examples

```cpp
#include <deque>

std::deque<int> d{1, 2, 3};

d.push_front(0);     // {0,1,2,3}   — cheap, unlike vector
d.push_back(4);      // {0,1,2,3,4}
d.pop_front();       // {1,2,3,4}

d[2];                // O(1) random access
d.at(2);             // bounds-checked
d.front(); d.back();

for (int x : d) { /* ... */ }
```

### Using it as a sliding window

```cpp
std::deque<int> window;
auto push = [&](int x, std::size_t cap) {
    window.push_back(x);
    if (window.size() > cap) window.pop_front();   // both ends, both O(1)
};
```

## Gotchas

- **No `data()`, no contiguity.** You can't hand a `deque` to a C API or build a
  `std::span` over it. If you need that, use `vector`.
- **References survive `push_front`/`push_back` but iterators don't.** Don't
  cache iterators across end-insertions.
- **Higher constant factors.** More allocations and pointer chasing than
  `vector`; iteration crosses chunk boundaries. Benchmark before assuming it's
  faster.
- **Memory isn't released eagerly** as you pop; like `vector`, `clear()` may keep
  capacity. Use `shrink_to_fit()` (non-binding request) to hint a release.

## See also

- [std::deque — cppreference](https://en.cppreference.com/w/cpp/container/deque)
