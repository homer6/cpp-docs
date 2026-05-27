# std::hive

> A sequence container with **constant-time insert and erase** that **never moves
> or invalidates its other elements** — it reuses the memory of erased elements
> instead of shifting. **Header:** `<hive>` · **Since:** C++26

> Formerly known as `plf::colony` (proposal P0447). Order of elements is **not**
> guaranteed to be insertion order — the container picks where new elements go.

## Mental model

Storage is several **element blocks** (not one contiguous array). Erasing an
element doesn't relocate its neighbours; instead the slot is marked and **skipped
during iteration** (via an internal skipfield), and that freed slot can be
**reused** by a later insertion. So elements never move once placed — their
addresses and iterators stay put for life.

```
 block A            block B
 [a][·][c][d]       [e][f][·][·]      (· = erased slot, skipped on iteration)
     ↑ erased,                 ↑ reserved capacity
     reused by next insert

 iterate → a, c, d, e, f   (erased slots transparently skipped)
```

cppreference's terms: blocks holding elements are **active blocks**; empty ones
become **reserved blocks** (or are deallocated). `reserve()` pre-creates reserved
blocks. New active blocks grow by an **implementation-defined factor**.

## Declaration

```cpp
template<
    class T,
    class Allocator = std::allocator<T>
> class hive;                               // (1)

namespace pmr {
    template<class T>
    using hive = std::hive<T, std::pmr::polymorphic_allocator<T>>;   // (2)
}
```

It satisfies `Container` (**except `operator==`/`!=`**), `ReversibleContainer`,
`AllocatorAwareContainer`, and *some* of the `SequenceContainer` requirements.
"ReversibleContainer" means its iterators are **bidirectional** (not random
access — no `h[i]`).

You can cap the min/max capacity of element blocks via `std::hive_limits`
(set at construction or with `reshape()`), to tune memory vs. block count.

## Complexity

| Operation | Cost |
|---|---|
| `insert` / `emplace` | O(1) |
| `erase` (single element) | O(1) |
| iterate | O(n) (skips erased slots) |
| `h[i]` random access | **not supported** (bidirectional only) |

## Iterator & reference invalidation

This is the whole point of `hive`:

- **Insertion does not invalidate** iterators, pointers, or references to existing
  elements (elements never move; new ones go into reused or fresh slots).
- **Erasure invalidates only** the iterators/pointers/references to the **erased**
  element. Everything else stays valid.

Same stability guarantee as `std::list`, but with far better locality (block
storage, not one-node-per-allocation) and without `list`'s ordering.

## When to use it

- You **add and remove elements frequently** and need **stable pointers/iterators
  to the survivors** — classic for game entities, particle systems, object pools,
  simulation actors, observer registries.
- You'd otherwise reach for `std::list` for its stability but want better cache
  behavior and faster traversal.
- **Avoid** when you need: insertion order or any specific order (`vector`,
  `deque`), random access by index (`vector`, `deque`), or `==` comparison
  between containers (`hive` doesn't provide it).

## Examples

```cpp
#include <hive>

std::hive<int> h;
auto it = h.insert(42);     // returns an iterator to the new element
h.insert(7);
h.emplace(99);

// `it` (and every other live iterator/pointer) stays valid across inserts:
*it;                        // still 42

h.erase(it);                // O(1); only `it` is invalidated, others survive

for (int x : h) { /* survivors, in container-chosen order */ }
h.size();                   // number of live elements
```

### Pre-reserve and tune block sizes

```cpp
std::hive<int> g;
g.reserve(10'000);                          // pre-create reserved capacity

// Constrain element-block capacities (min, max):
std::hive<int> limited{std::hive_limits{/*min*/ 64, /*max*/ 1024}};
```

### Member operations (like list, since order isn't positional)

```cpp
h.sort();          // sort in place
h.unique();        // remove consecutive duplicates (after sorting)
// splice elements between hives, trim_capacity(), reshape(), etc. are also provided
```

### C++20-style uniform erase

```cpp
std::erase(h, 7);
std::erase_if(h, [](int x){ return x < 0; });
```

## Gotchas

- **No defined element order.** Insertion location is the container's choice and
  freed slots get reused, so iteration order is neither insertion order nor
  sorted. Don't rely on position.
- **No random access and no `operator[]`** — iterators are bidirectional. Reach
  elements by iterating or by holding the iterator `insert` handed you.
- **No `operator==` / `operator!=`** between hives (explicitly excluded from the
  `Container` requirements it meets).
- **`reserve` reserves *blocks*, not a flat element array** — memory stays
  block-structured; you won't get a contiguous buffer or `data()`.
- Brand-new in C++26; cppreference's own page still marks parts "incomplete," and
  library/compiler support is early. Check your toolchain.

## See also

- [std::hive — cppreference](https://en.cppreference.com/w/cpp/container/hive)
- [Standard library header `<hive>` — cppreference](https://en.cppreference.com/w/cpp/header/hive)
- [std::hive_limits — cppreference](https://en.cppreference.com/w/cpp/container/hive_limits)
