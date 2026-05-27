# std::map

> A sorted dictionary: **unique keys** mapped to values, with O(log n) lookup and
> ordered traversal. **Header:** `<map>` · **Since:** C++98

## Mental model

A balanced BST (red-black tree) of `std::pair<const Key, T>` nodes, ordered by
key. Same structure as `set`, but each node also carries a mapped value. The key
is `const` — you can change the value, never the key, through an iterator.

```
 key:   apple → 3     banana → 7     cherry → 2
 stored as nodes of pair<const Key, T>, ordered by key
```

## Declaration

```cpp
template<
    class Key,
    class T,
    class Compare = std::less<Key>,
    class Allocator = std::allocator<std::pair<const Key, T>>
> class map;
```

- `Key` / `T` — key type and mapped (value) type.
- `value_type` is `std::pair<const Key, T>`.

## Complexity

| Operation | Cost |
|---|---|
| `find` / `count` / `contains` / `at` / `operator[]` | O(log n) |
| `insert` / `emplace` / `erase` | O(log n) |
| `lower_bound` / `upper_bound` / `equal_range` | O(log n) |
| ordered iteration | O(n) |

## Iterator & reference invalidation

Node-based: **insertion never invalidates; erasure invalidates only the erased
element.** References to a *value* stay valid until that element is erased.

## When to use it

- A dictionary that must stay **sorted by key**, or where you need range queries
  over keys.
- Stable references to values across inserts/erases of other keys.
- **Avoid** when order doesn't matter and you want average O(1) — use
  `unordered_map`. For read-mostly, cache-friendly maps, see `flat_map` (C++23).

## Examples

### The many ways to insert (and why they differ)

```cpp
#include <map>

std::map<std::string, int> m;

m["alice"] = 30;                 // operator[]: inserts default (0) then assigns
m.insert({"bob", 25});           // no-op if "bob" exists
m.emplace("carol", 40);          // construct in place; no-op if exists
m.try_emplace("alice", 99);      // C++17: does NOT touch the value if key exists
m.insert_or_assign("alice", 31); // C++17: insert, or overwrite if present
```

`try_emplace` and `insert_or_assign` exist precisely to fix `operator[]`'s and
`insert`'s shortcomings — see Gotchas.

### Lookup

```cpp
m.at("alice");          // bounds-checked → throws std::out_of_range if missing
m["alice"];             // inserts a default-constructed value if missing (!)
m.contains("alice");    // C++20, no insertion, no throw
auto it = m.find("bob");
if (it != m.end()) { /* it->first, it->second */ }
```

### Iterating (sorted by key) with structured bindings

```cpp
for (const auto& [key, value] : m) {        // C++17 structured bindings
    // visited in ascending key order
}
for (auto& [key, value] : m) value += 1;     // mutate values, keys are const
```

### Counting words — the idiomatic snippet

```cpp
std::map<std::string, int> freq;
for (const auto& w : words) ++freq[w];        // operator[] default-inits to 0
```

### C++20 uniform erase

```cpp
std::erase_if(m, [](const auto& kv){ return kv.second == 0; });
```

## Gotchas

- **`operator[]` inserts.** Reading a missing key via `m[k]` **creates** it with
  a value-initialized value. It also requires the value type be
  default-constructible, and can't be used on a `const map`. Use `at`, `find`, or
  `contains` to look up without inserting.
- **`operator[]` overwrites silently** on assignment — if you only want to insert
  when absent, use `try_emplace`; if you want "insert or update", use
  `insert_or_assign`.
- **`insert`/`emplace` are no-ops on an existing key** (they return
  `{iterator, false}`), so they *won't* update the value. This surprises people
  coming from `[]`.
- **Keys are `const`.** Change a key only via `extract` (C++17), not assignment.

## See also

- [std::map — cppreference](https://en.cppreference.com/w/cpp/container/map)
- [std::map::try_emplace — cppreference](https://en.cppreference.com/w/cpp/container/map/try_emplace)
- [std::map::insert_or_assign — cppreference](https://en.cppreference.com/w/cpp/container/map/insert_or_assign)
