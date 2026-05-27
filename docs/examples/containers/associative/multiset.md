# std::multiset

> Like [`std::set`](set.md), but **duplicate (equivalent) keys are allowed**.
> **Header:** `<set>` · **Since:** C++98

## Mental model

Same balanced BST as `set`, except equivalent keys aren't rejected — they coexist
as separate nodes. An in-order walk yields all keys sorted, duplicates adjacent.

```
 multiset<int>{1, 3, 3, 3, 5}   →   1  3  3  3  5   (sorted, dups kept)
```

## Declaration

```cpp
template<
    class Key,
    class Compare = std::less<Key>,
    class Allocator = std::allocator<Key>
> class multiset;
```

## Complexity

Same as `set`: O(log n) insert/erase/find, O(n) ordered iteration. `count` is
O(log n + k) where k is the number of matches.

## Iterator & reference invalidation

Same as `set`: insertion never invalidates; erasure only the erased element.
Since C++11, `insert` keeps equivalent elements in **insertion order** relative
to each other (stable).

## How it differs from `set`

- **`insert` always succeeds** and returns a plain iterator (not a
  `pair<iterator,bool>`) — there's no "already present" rejection.
- **`count(key)`** can exceed 1; **`contains`** (C++20) still just answers yes/no.
- **`erase(key)`** removes **all** equivalent elements and returns how many; to
  remove a single instance, erase a specific iterator.
- **`equal_range` / `lower_bound` / `upper_bound`** are the tools for working with
  a run of equal keys.

## Examples

```cpp
#include <set>

std::multiset<int> ms{1, 3, 3, 3, 5};
ms.insert(3);                 // now four 3s; always inserts

ms.count(3);                  // 4
ms.contains(3);               // true (C++20)

// Erase exactly one 3 (not all of them):
auto it = ms.find(3);
if (it != ms.end()) ms.erase(it);   // removes a single node

ms.erase(3);                  // removes ALL remaining 3s, returns the count

// Iterate over all elements equal to a key:
auto [lo, hi] = ms.equal_range(3);
for (auto i = lo; i != hi; ++i) { /* each 3 */ }
```

## Gotchas

- **`erase(key)` nukes every match.** If you mean "remove one occurrence," pass an
  *iterator*, not the key.
- **`find` returns *some* equivalent element**, not necessarily the first — use
  `lower_bound`/`equal_range` if order among equals matters.
- Everything else mirrors [`set`](set.md): keys are `const`, equivalence (not
  `==`) decides "equal," node-based stability.

## See also

- [std::multiset — cppreference](https://en.cppreference.com/w/cpp/container/multiset)
- [std::multiset::equal_range — cppreference](https://en.cppreference.com/w/cpp/container/multiset/equal_range)
