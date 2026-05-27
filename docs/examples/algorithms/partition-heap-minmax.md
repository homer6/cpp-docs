# Partition, heap, and min/max

> Splitting ranges by predicate, managing a binary heap by hand, and extremum
> helpers. **Header:** `<algorithm>`

## Partitioning

Reorder so all elements satisfying a predicate come first:

```cpp
#include <algorithm>

auto mid = std::partition(v.begin(), v.end(), pred);   // [begin,mid) satisfy pred
std::stable_partition(v.begin(), v.end(), pred);        // keeps relative order
bool ok = std::is_partitioned(v.begin(), v.end(), pred);
auto pp = std::partition_point(v.begin(), v.end(), pred);  // boundary of a partitioned range
```

`std::partition` is the core of quicksort-style splits and "separate the matching
ones" tasks.

## Heap operations

A heap is a `vector`-backed structure with the max at the front. (For a ready-made
priority queue, prefer [`std::priority_queue`](../containers/adaptors/priority_queue.md);
use these when you need direct control.)

```cpp
std::make_heap(v.begin(), v.end());        // O(n): turn the range into a max-heap
std::push_heap(v.begin(), v.end());        // after push_back, sift the new element up
std::pop_heap(v.begin(), v.end());         // move max to the back; then pop_back it
std::sort_heap(v.begin(), v.end());        // turn a heap into a sorted range (destroys heap)
bool h = std::is_heap(v.begin(), v.end());
```

Typical use: `make_heap`, then repeated `pop_heap` + `pop_back` to extract in
descending order.

## Min / max / clamp

```cpp
std::min(a, b);   std::max(a, b);
std::min({3, 1, 4, 1, 5});                 // min of an initializer_list
auto [lo, hi] = std::minmax(a, b);          // both at once (C++11)

std::min_element(v.begin(), v.end());       // iterator to smallest
std::max_element(v.begin(), v.end());
auto [mn, mx] = std::minmax_element(v.begin(), v.end());

std::clamp(x, lo, hi);                       // C++17: pin x into [lo, hi]
```

## Gotchas

- **`std::min`/`max` return by `const&`** — `auto m = std::max(f(), g());` can
  dangle if you bind to a reference and the temporaries die. Bind to a value.
- **`std::minmax(a, b)` returns references to the arguments** — same dangling
  caution with temporaries; `minmax_element` returns iterators (safe while the
  range lives).
- **Heap functions operate on the underlying range, not a heap type.** It's on
  you to keep it a valid heap between calls — easy to corrupt by mutating the
  middle.
- **`clamp(x, lo, hi)` with `lo > hi` is UB.** Ensure the bounds are ordered.

## See also

- [std::partition — cppreference](https://en.cppreference.com/w/cpp/algorithm/partition)
- [Heap operations — cppreference](https://en.cppreference.com/w/cpp/algorithm/make_heap)
- [std::clamp — cppreference](https://en.cppreference.com/w/cpp/algorithm/clamp)
- [std::minmax_element — cppreference](https://en.cppreference.com/w/cpp/algorithm/minmax_element)
