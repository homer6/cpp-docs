# std::unordered_multimap

> Like [`std::unordered_map`](unordered_map.md), but **one key can map to many
> values** (duplicate keys allowed). **Header:** `<unordered_map>` · **Since:** C++11

## Mental model

Same hash-bucket structure as `unordered_map`; equivalent keys aren't merged —
each `key→value` pairing is its own node. Entries with equal keys are grouped
together within the table (so `equal_range` can return them as a contiguous run).

## Declaration

```cpp
template<
    class Key,
    class T,
    class Hash = std::hash<Key>,
    class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<std::pair<const Key, T>>
> class unordered_multimap;
```

## Complexity

Average O(1) `insert`/`erase`/`find`; `count(key)` and `equal_range(key)` are
average O(1) plus O(k) for the k matches. Worst case O(n) under poor hashing.

## Iterator & reference invalidation

Same as `unordered_map`: **references stable across rehash; iterators invalidated
by rehash.** Erasing affects only the erased element.

## How it differs from `unordered_map`

- **No `operator[]` and no `at`** (a key isn't unique).
- **`insert`/`emplace` always succeed**, returning a plain iterator.
- **`count(key)`** can exceed 1; **`erase(key)`** removes *all* matching entries.
- **`equal_range(key)`** is the way to visit all values for a key.

## Examples

```cpp
#include <unordered_map>

std::unordered_multimap<std::string, int> m;
m.emplace("a", 1);
m.emplace("a", 2);          // both kept
m.emplace("b", 3);

m.count("a");               // 2

auto [lo, hi] = m.equal_range("a");
for (auto it = lo; it != hi; ++it) { /* it->second: 1 and 2 */ }

// Remove one specific pairing vs. all:
auto it = m.find("a");
if (it != m.end()) m.erase(it);   // one
m.erase("a");                     // all remaining "a"
```

## Gotchas

- **No `[]`, `at`, `try_emplace`, or `insert_or_assign`** — those are unique-key
  conveniences. Use `find` / `equal_range`.
- **`erase(key)` removes the whole group**; pass an iterator for a single entry.
- Inherits the unordered rules: iterators die on rehash, references survive, no
  ordering.

## See also

- [std::unordered_multimap — cppreference](https://en.cppreference.com/w/cpp/container/unordered_multimap)
