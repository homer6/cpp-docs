# Algorithms library

Generic operations over ranges — the payoff for iterators. "Prefer an algorithm
to a hand-written loop": it's clearer, harder to get wrong, and often faster.
Mirrors [cppreference's algorithms library](https://en.cppreference.com/w/cpp/algorithm).

| Page | Header | What |
|---|---|---|
| [Non-modifying](non-modifying.md) | `<algorithm>` | find, count, search, all/any/none, equal |
| [Modifying](modifying.md) | `<algorithm>` | copy, transform, fill, replace, remove, unique, reverse, rotate |
| [Sorting & searching](sorting-searching.md) | `<algorithm>` | sort, partial_sort, nth_element, binary search, merge, set ops |
| [Partition, heap, min/max](partition-heap-minmax.md) | `<algorithm>` | partition, the heap functions, min/max/clamp |
| [Numeric](numeric.md) | `<numeric>` | accumulate, reduce, scans, iota, gcd/lcm |
| [Execution policies & ranges algorithms](execution-and-ranges.md) | `<execution>`, `<algorithm>` | parallel algorithms; `std::ranges::` versions + projections |

## Two flavors of every algorithm (C++20)

```cpp
std::sort(v.begin(), v.end());     // classic: iterator pair
std::ranges::sort(v);              // C++20: whole range, + projections, safer
```

The [ranges](../ranges/README.md) versions (`std::ranges::*`) take a range
directly, support **projections**, and are constrained by concepts (clean errors).
Prefer them in new C++20 code; the classic iterator-pair forms remain everywhere.

## Rules of thumb

- **Reach for a named algorithm before writing a raw loop.** Intent is in the
  name (`std::any_of` beats a flag-and-break loop).
- **Most algorithms don't change size** — they work within `[first, last)`. To
  actually remove elements, pair `std::remove`/`unique` with `erase` (or use
  `std::erase`/`erase_if`, C++20). See [modifying](modifying.md).
- **Sorted-range algorithms (binary search, set ops, merge) require sorted
  input** — and silently misbehave otherwise.
