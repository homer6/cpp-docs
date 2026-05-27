# std::unordered_set

> A hash table of **unique keys**: **average O(1)** membership/insert/erase, no
> ordering. **Header:** `<unordered_set>` · **Since:** C++11

## Mental model

Same hash-bucket structure as [`unordered_map`](unordered_map.md), but the value
*is* the key (no mapped value). A hash picks a bucket; collisions chain.

```
 buckets: [0] → ∅
          [1] → "amy"
          [2] → "bob" → "cid"   ← collision chain
```

## Declaration

```cpp
template<
    class Key,
    class Hash = std::hash<Key>,
    class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<Key>
> class unordered_set;
```

## Complexity

| Operation | Average | Worst case |
|---|---|---|
| `find` / `contains` / `count` | **O(1)** | O(n) |
| `insert` / `emplace` / `erase` | **O(1)** | O(n) |
| iterate | O(n + bucket_count) | — |

## Iterator & reference invalidation

Same split as `unordered_map`: **references/pointers to elements stay valid**
(nodes don't move, even on rehash); **iterators are invalidated by rehash**
(which `insert` can trigger). Erasing invalidates only the erased element.

## When to use it

- **Fast set membership / dedup** where order is irrelevant — the default "is X in
  this collection?" container.
- **Avoid** when you need sorted iteration or range queries (`set`), or when the
  set is small and cache-friendly contiguous storage wins (`flat_set`, or a
  sorted `vector`).

## Examples

```cpp
#include <unordered_set>

std::unordered_set<std::string> seen{"a", "b", "c"};

auto [it, inserted] = seen.insert("b");   // inserted == false (already present)
seen.contains("a");                       // C++20
seen.count("z");                          // 0 or 1
seen.erase("b");

for (const auto& s : seen) { /* arbitrary order */ }
```

### Dedup a vector in O(n) average

```cpp
std::vector<int> v{1, 2, 2, 3, 3, 3};
std::unordered_set<int> uniq(v.begin(), v.end());   // {1,2,3} in some order
```

### Reserve for known size

```cpp
std::unordered_set<int> s;
s.reserve(1'000'000);            // avoid repeated rehashing while filling
```

### Custom element type

```cpp
struct Id { int v; bool operator==(const Id&) const = default; };
template<> struct std::hash<Id> {
    std::size_t operator()(const Id& i) const noexcept { return std::hash<int>{}(i.v); }
};
std::unordered_set<Id> ids;
```

### C++20 uniform erase

```cpp
std::erase_if(seen, [](const std::string& s){ return s.empty(); });
```

## Gotchas

- **Iterators die on rehash; references survive.** `reserve` if you need stability
  while inserting.
- **A custom type needs a good `std::hash` and `operator==`.** A weak hash
  collapses performance to O(n).
- **Equivalence here is `KeyEqual` (`==`)**, unlike ordered `set` which uses
  `!(a<b) && !(b<a)`.
- **No ordering** — and it can change across rehashes/runs.

## See also

- [std::unordered_set — cppreference](https://en.cppreference.com/w/cpp/container/unordered_set)
- [std::hash — cppreference](https://en.cppreference.com/w/cpp/utility/hash)
