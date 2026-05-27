# std::flat_map / std::flat_multimap

> A sorted dictionary **adaptor** backed by **two parallel contiguous containers**
> (keys and values), giving cache-friendly storage with map semantics.
> **Header:** `<flat_map>` · **Since:** C++23

`flat_map` keeps **unique** keys; `flat_multimap` allows **duplicates**.

## Mental model

Unlike `std::map` (one tree of `pair<const Key,T>` nodes), `flat_map` stores keys
and values in **two separate sorted arrays**, element *i* of one lining up with
element *i* of the other. The keys array stays sorted; lookup binary-searches it,
then indexes into the values array.

```
 keys:   [ apple | banana | cherry ]   ← sorted, binary-searched
 values: [   3   |    7   |    2   ]   ← parallel, same indices
```

This "struct of arrays" layout keeps each array densely packed — better for
cache and for scanning all keys or all values.

## Declaration

```cpp
template<
    class Key,
    class T,
    class Compare = std::less<Key>,
    class KeyContainer = std::vector<Key>,
    class MappedContainer = std::vector<T>
> class flat_map;          // flat_multimap has the same parameters
```

- `KeyContainer` / `MappedContainer` — the two backing sequence containers.
- `value_type` is `std::pair<key_type, mapped_type>`, but that pair is **not how
  data is stored** — it's the logical element type. Dereferencing an iterator
  yields a **pair of references** into the two arrays (a proxy), not a reference
  to a stored `pair`.

## Complexity

| Operation | Cost |
|---|---|
| `find` / `contains` / `count` / `at` / `lower_bound` | O(log n) (binary search) |
| iterate (sorted by key) | O(n), contiguous |
| `insert` / `erase` / `operator[]` (insertion) | **O(n)** — shifts both arrays |

## Iterator & reference invalidation

Like [`vector`](../sequence/vector.md): inserting or erasing can move elements
and **invalidate iterators, pointers, and references** into both arrays — unlike
node-based `map`. Don't cache references across modifications.

## When to use it

- **Read-mostly** maps where you build once and look up/scan many times; the
  packed layout beats `std::map` on cache and memory.
- You want all-keys or all-values scans (`keys()` / `values()` hand you the raw
  contiguous containers).
- **Avoid** for insert/erase-heavy use (O(n) each) — prefer `map` (stable nodes)
  or `unordered_map` (O(1) average).

## Examples

```cpp
#include <flat_map>

std::flat_map<std::string, int> m{{"banana", 7}, {"apple", 3}};
m["cherry"] = 2;                 // O(n) insert, keeps keys sorted
m.at("apple");                   // bounds-checked lookup
m.contains("banana");            // true

for (const auto& [k, v] : m) {   // ascending key order; structured bindings
    // k, v are references into the parallel arrays
}
```

### Access the underlying arrays directly

```cpp
const auto& ks = m.keys();       // const std::vector<std::string>&
const auto& vs = m.values();     // const std::vector<int>&
// e.g. hand `vs` to a numeric routine, or `ks` to a span — contiguous.
```

### Build fast from sorted input

```cpp
std::vector<std::string> keys{"apple", "banana", "cherry"};   // sorted, unique
std::vector<int>         vals{3, 7, 2};
std::flat_map<std::string, int> fm(std::sorted_unique,
                                   std::move(keys), std::move(vals));
```

### flat_multimap allows repeats; no operator[]

```cpp
std::flat_multimap<std::string, int> mm;
mm.emplace("x", 1);
mm.emplace("x", 2);              // both kept
auto [lo, hi] = mm.equal_range("x");   // iterate both
```

## Gotchas

- **The element is a proxy, not a real `pair&`.** `*it` gives you references into
  the two arrays, so you can't take a `std::pair<const K,V>&` to an element the
  way you can with `map`. Use structured bindings.
- **Mutation is O(n)** and shifts *both* arrays — looping single inserts is O(n²).
  Build in bulk or use the `sorted_unique` constructor.
- **`operator[]` still inserts** a default value for a missing key (and is O(n)),
  same trap as `map`. `flat_multimap` has **no `operator[]`/`at`** (non-unique
  keys).
- **Vector-like invalidation**, not tree-like — references die on insert/erase.

## See also

- [std::flat_map — cppreference](https://en.cppreference.com/w/cpp/container/flat_map)
- [std::flat_multimap — cppreference](https://en.cppreference.com/w/cpp/container/flat_multimap)
- [flat_map::keys / values — cppreference](https://en.cppreference.com/w/cpp/container/flat_map/keys)
