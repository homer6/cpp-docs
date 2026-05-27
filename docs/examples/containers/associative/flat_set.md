# std::flat_set / std::flat_multiset

> A sorted associative **adaptor** backed by a contiguous sequence container
> (a sorted `vector` by default) — set semantics with cache-friendly storage.
> **Header:** `<flat_set>` · **Since:** C++23

`flat_set` keeps **unique** keys; `flat_multiset` allows **duplicates**.
Otherwise they share the same design.

## Mental model

Not a tree — a **single sorted array**. Lookups are binary searches; iteration is
a linear walk over contiguous memory (great locality). Inserts/erases shift
elements like a `vector`, so they're O(n).

```
 flat_set<int>{5, 1, 3, 2}   →   underlying vector: [1, 2, 3, 5]  (kept sorted)
                                  lookup = binary search
```

It's a **container adaptor**: it wraps an underlying container you can choose,
rather than owning bespoke node storage.

## Declaration

```cpp
template<
    class Key,
    class Compare = std::less<Key>,
    class KeyContainer = std::vector<Key>
> class flat_set;          // flat_multiset has the same parameters
```

- `KeyContainer` — the backing sequence container (`std::vector<Key>` by default;
  could be e.g. a `std::deque<Key>` or a small-buffer vector type).
- There is **no allocator parameter** on the adaptor itself — that belongs to the
  underlying container.

## Complexity

| Operation | Cost |
|---|---|
| `find` / `contains` / `count` / `lower_bound` | O(log n) (binary search) |
| iterate (sorted) | O(n), contiguous — very cache-friendly |
| `insert` / `erase` (single) | **O(n)** — shifts the tail, like `vector` |
| bulk build from a range | O(n log n) to sort |

## Iterator & reference invalidation

Because storage is a sequence container, treat invalidation like
[`vector`](../sequence/vector.md): **insert or erase can move elements and
invalidate iterators, pointers, and references.** This is the big behavioral
difference from node-based `set` (where nodes are stable). Don't hold iterators
across modifications.

## When to use it

- **Read-mostly** sets: build once (or rarely), then do many lookups and ordered
  scans. The contiguous layout beats `std::set` on cache and memory overhead.
- You want sorted/range-query semantics but `set`'s per-node allocation is too
  costly.
- **Avoid** for insert/erase-heavy workloads (each is O(n)) — use `set` (node
  stability, O(log n) mutation) or `unordered_set` (O(1) average, no order).

## Examples

### Basics

```cpp
#include <flat_set>

std::flat_set<int> s{5, 1, 3, 2};   // sorted into [1,2,3,5]
s.insert(4);                        // [1,2,3,4,5]  (O(n) shift)
s.contains(3);                      // true
s.erase(2);                         // [1,3,4,5]

for (int x : s) { /* ascending; contiguous walk */ }
```

### Skip the sort when input is already ordered

```cpp
#include <flat_set>
// Promise the data is sorted+unique so the constructor doesn't re-sort:
std::vector<int> sorted{1, 2, 3, 5};
std::flat_set<int> s2(std::sorted_unique, sorted.begin(), sorted.end());
```

(`std::sorted_unique` is for unique-key flat containers; `flat_multiset` uses
`std::sorted_equivalent` for sorted-but-not-necessarily-unique input.)

### flat_multiset keeps duplicates

```cpp
std::flat_multiset<int> ms{3, 1, 3, 2, 3};   // [1,2,3,3,3]
ms.count(3);                                 // 3
auto [lo, hi] = ms.equal_range(3);           // the run of 3s
```

## Gotchas

- **Mutation is O(n).** A `flat_set` that's inserted/erased one element at a time
  in a loop is O(n²). Build in bulk, or batch your changes.
- **Iterators invalidate like a vector's**, not like a tree's — a frequent
  surprise when migrating from `std::set`.
- **Strong exception guarantee is weaker** than node containers: an insert that
  throws mid-shift can leave the container altered. Read cppreference's notes for
  the exact guarantees.
- **`sorted_unique` is a promise, not a check.** Passing unsorted/duplicate data
  with that tag is undefined behavior.

## See also

- [std::flat_set — cppreference](https://en.cppreference.com/w/cpp/container/flat_set)
- [std::flat_multiset — cppreference](https://en.cppreference.com/w/cpp/container/flat_multiset)
- [std::sorted_unique / std::sorted_equivalent — cppreference](https://en.cppreference.com/w/cpp/container/sorted_unique)
