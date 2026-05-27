# std::unordered_map

> A hash table mapping **unique keys** to values: **average O(1)** lookup/insert,
> no ordering. **Header:** `<unordered_map>` · **Since:** C++11

## Mental model

An array of **buckets**; a hash of the key picks a bucket, and entries that land
in the same bucket form a small chain (the standard mandates separate chaining,
so node addresses are stable). Iteration order is unspecified and can change when
the table rehashes.

```
 hash("bob") % bucket_count = 2

 buckets: [0] → ∅
          [1] → ("amy",30)
          [2] → ("bob",25) → ("cid",9)   ← collision chain
          [3] → ∅
```

## Declaration

```cpp
template<
    class Key,
    class T,
    class Hash = std::hash<Key>,
    class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<std::pair<const Key, T>>
> class unordered_map;
```

- `Hash` — produces the hash; specialize `std::hash` for your own key types.
- `KeyEqual` — equality used to resolve collisions (must agree with `Hash`).

## Complexity

| Operation | Average | Worst case |
|---|---|---|
| `find` / `contains` / `[]` / `at` | **O(1)** | O(n) (all keys collide) |
| `insert` / `emplace` / `erase` | **O(1)** | O(n) |
| iterate | O(n + bucket_count) | — |

Worst case degrades to O(n) under adversarial or poor hashing.

## Iterator & reference invalidation

Two separate rules — this trips people up:

- **References & pointers to elements are *always* stable** (nodes never move),
  even across rehash. Only erasing an element invalidates references to *that*
  element.
- **Iterators are invalidated by rehashing** (which `insert` can trigger when the
  load factor is exceeded). So: keep references across inserts, but **not
  iterators**.

## When to use it

- **Pure lookup / membership / dictionary** where you don't need ordering — the
  go-to associative container when order is irrelevant.
- **Avoid** when you need sorted iteration or range queries (`map`), or when keys
  are few/cheap-to-compare and cache locality dominates (`flat_map`, or even a
  sorted `vector`).

## Examples

### Basics — same insertion API as map

```cpp
#include <unordered_map>

std::unordered_map<std::string, int> m;
m["alice"] = 30;                 // operator[] inserts then assigns
m.try_emplace("bob", 25);        // C++17: skip if present (no value overwrite)
m.insert_or_assign("alice", 31); // C++17: insert or overwrite

m.contains("bob");               // C++20
m.at("alice");                   // throws std::out_of_range if missing
for (const auto& [k, v] : m) { /* arbitrary order */ }
```

### Reserve to avoid rehashing

```cpp
std::unordered_map<int, int> m;
m.reserve(10'000);               // pre-size buckets for ~10k elements
m.max_load_factor(0.5f);         // trade memory for fewer collisions
```

### Custom key type needs a hash

```cpp
struct Point { int x, y; bool operator==(const Point&) const = default; };

template<> struct std::hash<Point> {
    std::size_t operator()(const Point& p) const noexcept {
        return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
    }
};

std::unordered_map<Point, std::string> grid;   // now works
```

### C++20 uniform erase

```cpp
std::erase_if(m, [](const auto& kv){ return kv.second == 0; });
```

## Gotchas

- **Iterators die on rehash; references don't.** `reserve` up front if you need
  iterator stability while inserting.
- **`operator[]` inserts** a default value for missing keys (same as `map`); use
  `find`/`contains`/`at` to avoid that.
- **A custom key type needs both `Hash` and equality.** A bad hash (many
  collisions) silently turns O(1) into O(n).
- **No ordering at all**, and order can change between runs/insertions — never
  rely on it.

## See also

- [std::unordered_map — cppreference](https://en.cppreference.com/w/cpp/container/unordered_map)
- [std::hash — cppreference](https://en.cppreference.com/w/cpp/utility/hash)
