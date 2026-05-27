# std::multimap

> Like [`std::map`](map.md), but **one key can map to many values** (duplicate
> keys allowed). **Header:** `<map>` · **Since:** C++98

## Mental model

Same balanced BST of `pair<const Key, T>` as `map`, but equivalent keys aren't
merged — each `key→value` pairing is its own node, kept sorted by key.

```
 "fruit" → apple
 "fruit" → cherry      ← same key, two entries
 "veg"   → carrot
```

## Declaration

```cpp
template<
    class Key,
    class T,
    class Compare = std::less<Key>,
    class Allocator = std::allocator<std::pair<const Key, T>>
> class multimap;
```

## Complexity

Same as `map`: O(log n) insert/erase/find. `count(key)` is O(log n + k).

## Iterator & reference invalidation

Same as `map`: insertion never invalidates; erasure only the erased element.
Equivalent keys preserve insertion order among themselves (since C++11).

## How it differs from `map`

- **No `operator[]` and no `at`.** They make no sense when a key isn't unique —
  which value would they return? Use `equal_range`/`find` instead.
- **`insert`/`emplace` always succeed** and return a plain iterator.
- **`erase(key)`** removes *all* entries with that key (returns the count);
  erase a single iterator to drop one pairing.
- **`equal_range(key)`** is the workhorse: it gives the `[lo, hi)` range of all
  entries for a key.

## Examples

```cpp
#include <map>

std::multimap<std::string, std::string> m;
m.emplace("fruit", "apple");
m.emplace("fruit", "cherry");
m.emplace("veg",   "carrot");

m.count("fruit");                 // 2

// Iterate all values for one key:
auto [lo, hi] = m.equal_range("fruit");
for (auto it = lo; it != hi; ++it)
    /* it->first == "fruit", it->second == apple / cherry */;

// Remove a single specific pairing:
auto it = m.find("fruit");        // some entry with key "fruit"
if (it != m.end()) m.erase(it);

m.erase("fruit");                 // removes ALL remaining "fruit" entries
```

## Gotchas

- **No `[]`, no `at`, no `try_emplace`/`insert_or_assign`** — those are unique-key
  conveniences. Lookups go through `find` / `equal_range`.
- **`erase(key)` removes the whole group.** Use an iterator for one entry.
- Otherwise it inherits `map`'s rules: keys are `const`, node-based stability,
  ordered by the comparator.

## See also

- [std::multimap — cppreference](https://en.cppreference.com/w/cpp/container/multimap)
- [std::multimap::equal_range — cppreference](https://en.cppreference.com/w/cpp/container/multimap/equal_range)
