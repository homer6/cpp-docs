# std::vector

> A growable, contiguous, dynamically-allocated array. **The default container** —
> reach for something else only when a measured need says so.
> **Header:** `<vector>` · **Since:** C++98

## Mental model

A single heap block holding elements back-to-back, plus two numbers: **size**
(how many elements) and **capacity** (how many fit before the next reallocation).

```
capacity = 8
size     = 5
          ┌───┬───┬───┬───┬───┬┄┄┄┬┄┄┄┬┄┄┄┐
 data ──▶ │ a │ b │ c │ d │ e │   │   │   │
          └───┴───┴───┴───┴───┴┄┄┄┴┄┄┄┴┄┄┄┘
            0   1   2   3   4    ↑ unused capacity
```

When `size` would exceed `capacity`, the vector allocates a bigger block
(typically ~1.5×–2×), moves the elements over, and frees the old block.
Because growth is geometric, `push_back` is **amortized O(1)**.

## Declaration

```cpp
template<class T, class Allocator = std::allocator<T>>
class vector;
```

- `T` — element type. Must be at least move-constructible/assignable.
- `Allocator` — how memory is obtained; you almost never specify it.

Special case: `std::vector<bool>` is a **space-optimized bit-packed
specialization**, not a real container of `bool`. Its `operator[]` returns a
proxy, not `bool&`. Avoid it when you want normal container semantics — use
`std::vector<char>`, `std::array<bool>`, or (C++20) `std::bitset`.

## Complexity

| Operation | Cost |
|---|---|
| `v[i]`, `at`, `front`, `back`, `data` | O(1) |
| `push_back` / `emplace_back` / `pop_back` | amortized O(1) |
| `insert` / `erase` in the middle | O(n) (shifts the tail) |
| `find` (linear scan) | O(n) |
| reserve-then-fill of n elements | O(n) |

## Iterator & reference invalidation

The big one. **Anything that reallocates invalidates *all* iterators, pointers,
and references** into the vector. Reallocation happens when an insert pushes
`size` past `capacity`.

- `push_back` / `emplace_back` / `insert`: invalidate everything **if** they
  reallocate; otherwise invalidate iterators/refs **at or after** the insertion
  point.
- `erase`: invalidates iterators/refs **at or after** the erased element.
- `reserve` / `resize` (growing past capacity) / `shrink_to_fit`: reallocate →
  invalidate everything.
- `clear`: does **not** free memory; capacity is unchanged, all iterators
  invalidated.

Classic bug: holding a pointer/reference to an element across a `push_back`.

## When to use it

- **Default sequence container.** Contiguous storage is cache-friendly and plays
  perfectly with C-APIs (`v.data()`), `std::span`, and algorithms.
- **You append/iterate far more than you insert/erase in the middle.**
- **Avoid** when you need: stable element addresses across growth (use `list`,
  `deque`, or store indices), heavy front insertion (`deque`), or a fixed
  compile-time size (`array`).

## Examples

### Construction (modern)

```cpp
#include <vector>

std::vector<int> a;                  // empty
std::vector<int> b(5);               // {0,0,0,0,0}
std::vector<int> c(5, 7);            // {7,7,7,7,7}
std::vector<int> d{1, 2, 3};         // initializer_list → {1,2,3}
std::vector e{1, 2, 3};              // CTAD → vector<int>  (C++17)
std::vector<int> f(d.begin(), d.end());  // from a range
```

### Growth and the reserve idiom

```cpp
std::vector<int> v;
v.reserve(1000);                 // one allocation; no reallocations below 1000
for (int i = 0; i < 1000; ++i)
    v.push_back(i);              // each is now truly O(1), refs stay valid

v.emplace_back(42);              // constructs in place (no temporary)
```

### Access

```cpp
v.front(); v.back();
v[0];                            // no bounds check (UB if out of range)
v.at(0);                         // bounds-checked → throws std::out_of_range
int* raw = v.data();             // contiguous block, for C APIs / spans
```

### Erase-remove, and the C++20 simplification

```cpp
#include <algorithm>
// pre-C++20: erase–remove idiom
v.erase(std::remove(v.begin(), v.end(), 7), v.end());

// C++20: one call, returns count removed
std::erase(v, 7);                       // remove all 7s
std::erase_if(v, [](int x){ return x % 2 == 0; });  // remove evens
```

### Iterating

```cpp
for (int x : v) { /* ... */ }                 // range-for
for (auto& x : v) x *= 2;                      // mutate in place
for (auto it = v.begin(); it != v.end(); ++it) {}
```

## Gotchas

- **`reserve` vs `resize`.** `reserve(n)` changes capacity only (size unchanged,
  no new elements). `resize(n)` changes the *size*, value-initializing or
  removing elements. Mixing them up is a common bug.
- **`clear()` keeps capacity.** To actually release memory: `v.clear();
  v.shrink_to_fit();` or `std::vector<T>().swap(v)`.
- **`vector<bool>` is not a container of bool.** See above.
- **Dangling across reallocation.** Don't keep raw pointers/iterators into a
  vector you're still growing — `reserve` up front, or re-fetch after.
- **`at` throws, `[]` doesn't.** Use `at` only when you actually want the check.

## See also

- [std::vector — cppreference](https://en.cppreference.com/w/cpp/container/vector)
- [std::vector\<bool\> — cppreference](https://en.cppreference.com/w/cpp/container/vector_bool)
- [std::erase, std::erase_if (vector) — cppreference](https://en.cppreference.com/w/cpp/container/vector/erase2)
