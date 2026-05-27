# std::set

> A sorted collection of **unique** keys, with O(log n) lookup/insert/erase and
> ordered traversal. **Header:** `<set>` · **Since:** C++98

## Mental model

A self-balancing binary search tree (typically a red-black tree). Each key lives
in its own node; an in-order walk yields keys in sorted order. "Sorted" is
defined by the comparator (`std::less<Key>` by default), and uniqueness is
defined by **equivalence**: `!(a < b) && !(b < a)`, *not* `a == b`.

```
            (m)
           /   \
        (e)     (r)
        /  \      \
      (b)  (g)    (z)        in-order → b e g m r z
```

## Declaration

```cpp
template<
    class Key,
    class Compare = std::less<Key>,
    class Allocator = std::allocator<Key>
> class set;
```

- `Key` — the element type (the value *is* the key).
- `Compare` — strict-weak-ordering comparator; controls sort order and equivalence.

## Complexity

| Operation | Cost |
|---|---|
| `find` / `count` / `contains` | O(log n) |
| `insert` / `emplace` / `erase` | O(log n) |
| `lower_bound` / `upper_bound` / `equal_range` | O(log n) |
| ordered iteration | O(n) |

## Iterator & reference invalidation

Node-based, so it's very stable: **insertion never invalidates** iterators or
references; **erasure invalidates only iterators/references to the erased
element**. (Same guarantee across `set`/`map`/`multiset`/`multimap`.)

## When to use it

- You need keys **kept in sorted order**, ordered iteration, or **range queries**
  (`lower_bound`/`upper_bound`).
- You want stable iterators/addresses across inserts.
- **Avoid** when you only need membership/lookup and don't care about order —
  `unordered_set` is average O(1). For read-heavy, cache-sensitive sets, consider
  `flat_set` (C++23).

## Examples

### Basics

```cpp
#include <set>

std::set<int> s{3, 1, 4, 1, 5};      // duplicates dropped → {1,3,4,5}, sorted
s.insert(2);                         // {1,2,3,4,5}
s.erase(3);                          // {1,2,4,5}

s.contains(4);                       // true   (C++20)
s.count(4);                          // 0 or 1 (pre-C++20 membership idiom)
s.find(4) != s.end();                // iterator-based membership
```

### insert returns whether it happened

```cpp
auto [it, inserted] = s.insert(2);   // structured bindings (C++17)
// `inserted` is false if 2 was already present; `it` points at the element
```

### Ordered queries

```cpp
std::set<int> t{10, 20, 30, 40};
auto lo = t.lower_bound(20);   // first element >= 20  → 20
auto hi = t.upper_bound(30);   // first element  > 30  → 40
for (auto it = lo; it != hi; ++it) { /* 20, 30 */ }
```

### Custom ordering & transparent comparators

```cpp
std::set<std::string, std::greater<>> desc{"a", "c", "b"};  // c, b, a

// Heterogeneous lookup: search by string_view without building a std::string.
// Enabled by giving the comparator a member typedef is_transparent (std::less<>
// has it). Since C++14.
std::set<std::string, std::less<>> names{"alice", "bob"};
names.find(std::string_view{"bob"});   // no temporary std::string constructed
```

### Node extraction & merge (C++17)

```cpp
std::set<int> a{1, 2, 3}, b{3, 4, 5};
a.merge(b);                  // moves nodes from b into a; b keeps only dups → {3}
                             // a == {1,2,3,4,5}

auto node = a.extract(2);    // pop a node out without deallocating
node.value() = 20;           // even mutate the key (can't do via iterator!)
a.insert(std::move(node));   // reinsert
```

### C++20 uniform erase

```cpp
std::erase_if(s, [](int x){ return x % 2 == 0; });   // remove evens
```

## Gotchas

- **Uniqueness is by equivalence, not `==`.** Two keys are "the same" iff neither
  is less than the other. Define a sane `operator<` / comparator.
- **You can't modify a key in place** via an iterator (it would break the tree
  ordering) — `*it` is `const`. Use `extract` to change a key (C++17).
- **`count` is 0/1** for `set` (use it for presence pre-C++20; prefer `contains`
  in C++20+).
- **Not contiguous, not cache-friendly.** Each node is a separate allocation; for
  hot lookup paths a hash set or `flat_set` is usually faster.

## See also

- [std::set — cppreference](https://en.cppreference.com/w/cpp/container/set)
- [std::set::merge / extract — cppreference](https://en.cppreference.com/w/cpp/container/set/merge)
- [Transparent comparators (`is_transparent`) — cppreference](https://en.cppreference.com/w/cpp/utility/functional/less_void)
