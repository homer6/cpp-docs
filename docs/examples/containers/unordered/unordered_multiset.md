# std::unordered_multiset

> Like [`std::unordered_set`](unordered_set.md), but **duplicate (equivalent)
> keys are allowed**. **Header:** `<unordered_set>` · **Since:** C++11

## Mental model

Same hash-bucket structure as `unordered_set`; equal keys aren't rejected — each
is its own node, grouped with its equivalents in the table. Useful as a **hashed
multiset / bag** (counting occurrences without ordering).

## Declaration

```cpp
template<
    class Key,
    class Hash = std::hash<Key>,
    class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<Key>
> class unordered_multiset;
```

## Complexity

Average O(1) `insert`/`erase`/`find`; `count(key)` and `equal_range(key)` average
O(1) + O(k) for k matches. Worst case O(n).

## Iterator & reference invalidation

Same as `unordered_set`: references stable across rehash, iterators invalidated
by rehash, erasure affects only the erased element.

## How it differs from `unordered_set`

- **`insert` always succeeds**, returns a plain iterator (no "already present").
- **`count(key)`** can exceed 1.
- **`erase(key)`** removes *all* equivalent elements; erase an iterator for one.
- **`equal_range(key)`** visits all equal elements.

## Examples

```cpp
#include <unordered_set>

std::unordered_multiset<int> bag{1, 1, 2, 3, 3, 3};
bag.insert(3);              // now four 3s; always inserts
bag.count(3);               // 4

// Erase just one occurrence:
auto it = bag.find(3);
if (it != bag.end()) bag.erase(it);

bag.erase(3);               // erase ALL remaining 3s, returns count
```

## Gotchas

- **`erase(key)` removes every match.** Pass an iterator to remove a single one.
- Equivalence is `KeyEqual` (`==`), not ordering.
- Inherits the unordered rules: no ordering, iterators die on rehash, references
  survive.

## See also

- [std::unordered_multiset — cppreference](https://en.cppreference.com/w/cpp/container/unordered_multiset)
