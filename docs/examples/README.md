# C++ examples — master index

Example-driven notes for learning all of modern C++, mirroring
[cppreference.com](https://cppreference.com/). We're working through it topic by
topic; **containers first**.

## Containers library

The standard containers, grouped as cppreference groups them.

### Sequence containers

Elements in a linear order you control.

| Container | Header | Since | Notes |
|---|---|---|---|
| [`array`](containers/sequence/array.md) | `<array>` | C++11 | Fixed-size, stack-allocated |
| [`vector`](containers/sequence/vector.md) | `<vector>` | C++98 | Growable contiguous array — the default choice |
| [`inplace_vector`](containers/sequence/inplace_vector.md) | `<inplace_vector>` | C++26 | Fixed-capacity vector, no heap |
| [`deque`](containers/sequence/deque.md) | `<deque>` | C++98 | Fast push/pop at both ends |
| [`list`](containers/sequence/list.md) | `<list>` | C++98 | Doubly-linked list |
| [`forward_list`](containers/sequence/forward_list.md) | `<forward_list>` | C++11 | Singly-linked list |

### Associative containers (ordered)

Sorted by key; balanced-tree based.

| Container | Header | Since | Notes |
|---|---|---|---|
| [`set`](containers/associative/set.md) | `<set>` | C++98 | Unique sorted keys |
| [`multiset`](containers/associative/multiset.md) | `<set>` | C++98 | Sorted keys, duplicates allowed |
| [`map`](containers/associative/map.md) | `<map>` | C++98 | Sorted key→value, unique keys |
| [`multimap`](containers/associative/multimap.md) | `<map>` | C++98 | Sorted key→value, duplicate keys |
| [`flat_set` / `flat_multiset`](containers/associative/flat_set.md) | `<flat_set>` | C++23 | Sorted set backed by a contiguous array |
| [`flat_map` / `flat_multimap`](containers/associative/flat_map.md) | `<flat_map>` | C++23 | Sorted map backed by two arrays |

### Unordered associative containers (hash-based)

Average O(1) lookup; no ordering.

| Container | Header | Since | Notes |
|---|---|---|---|
| [`unordered_set`](containers/unordered/unordered_set.md) | `<unordered_set>` | C++11 | Hashed unique keys |
| [`unordered_multiset`](containers/unordered/unordered_multiset.md) | `<unordered_set>` | C++11 | Hashed keys, duplicates allowed |
| [`unordered_map`](containers/unordered/unordered_map.md) | `<unordered_map>` | C++11 | Hashed key→value, unique keys |
| [`unordered_multimap`](containers/unordered/unordered_multimap.md) | `<unordered_map>` | C++11 | Hashed key→value, duplicate keys |

### Container adaptors

Wrap another container to give a restricted interface.

| Adaptor | Header | Since | Notes |
|---|---|---|---|
| [`stack`](containers/adaptors/stack.md) | `<stack>` | C++98 | LIFO |
| [`queue`](containers/adaptors/queue.md) | `<queue>` | C++98 | FIFO |
| [`priority_queue`](containers/adaptors/priority_queue.md) | `<queue>` | C++98 | Heap; largest-first by default |

### Views (related — not containers)

Non-owning windows over existing storage.

| View | Header | Since | Notes |
|---|---|---|---|
| [`span`](containers/views/span.md) | `<span>` | C++20 | Contiguous 1-D view |
| [`mdspan`](containers/views/mdspan.md) | `<mdspan>` | C++23 | Multidimensional view |

---

## Which container should I use? (cheat sheet)

- **Need a resizable list of things?** → `vector` (default). Reach for something
  else only when a measured need says so.
- **Fixed number of elements known at compile time?** → `array` (or
  `inplace_vector` if the count varies up to a fixed cap, C++26).
- **Lots of insert/remove at *both* ends?** → `deque`.
- **Splicing nodes / stable element addresses / O(1) middle insert with an
  iterator?** → `list` (or `forward_list` to save memory).
- **Keep keys sorted, need ordered traversal / range queries?** → `set` / `map`
  (or `flat_set` / `flat_map` for cache-friendly read-heavy use, C++23).
- **Just fast membership / lookup, order doesn't matter?** → `unordered_set` /
  `unordered_map`.
- **Restricted LIFO/FIFO/priority access?** → `stack` / `queue` /
  `priority_queue`.
- **Pass "a contiguous block of N elements" without copying or templating on the
  container?** → `span` (1-D) / `mdspan` (N-D).

---

## Progress

| Area | Status |
|---|---|
| Containers — sequence | ✅ done (array, vector, inplace_vector, deque, list, forward_list) |
| Containers — associative | ✅ done (set, multiset, map, multimap, flat_set, flat_map) |
| Containers — unordered | ✅ done (unordered_set/multiset/map/multimap) |
| Containers — adaptors | ✅ done (stack, queue, priority_queue) |
| Containers — views | ✅ done (span, mdspan) |
| Strings | ⬜ planned |
| Algorithms | ⬜ planned |
| Ranges | ⬜ planned |
| Utilities | ⬜ planned |
| Language | ⬜ planned |

> Legend: ✅ done · ⏳ in progress · ⬜ planned
