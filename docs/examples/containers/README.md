# Containers library

The standard containers, grouped as [cppreference](https://en.cppreference.com/w/cpp/container)
groups them. ✅ **Complete.**

## Sequence containers

Elements in a linear order you control.

| Container | Header | Since | Notes |
|---|---|---|---|
| [`array`](sequence/array.md) | `<array>` | C++11 | Fixed-size, stack-allocated |
| [`vector`](sequence/vector.md) | `<vector>` | C++98 | Growable contiguous array — the default choice |
| [`inplace_vector`](sequence/inplace_vector.md) | `<inplace_vector>` | C++26 | Fixed-capacity vector, no heap |
| [`hive`](sequence/hive.md) | `<hive>` | C++26 | O(1) insert/erase, stable refs, reuses erased slots |
| [`deque`](sequence/deque.md) | `<deque>` | C++98 | Fast push/pop at both ends |
| [`list`](sequence/list.md) | `<list>` | C++98 | Doubly-linked list |
| [`forward_list`](sequence/forward_list.md) | `<forward_list>` | C++11 | Singly-linked list |

## Associative containers (ordered)

Sorted by key; balanced-tree based.

| Container | Header | Since | Notes |
|---|---|---|---|
| [`set`](associative/set.md) | `<set>` | C++98 | Unique sorted keys |
| [`multiset`](associative/multiset.md) | `<set>` | C++98 | Sorted keys, duplicates allowed |
| [`map`](associative/map.md) | `<map>` | C++98 | Sorted key→value, unique keys |
| [`multimap`](associative/multimap.md) | `<map>` | C++98 | Sorted key→value, duplicate keys |

> The sorted `flat_*` containers are associative in spirit but cppreference
> classifies them as **container adaptors** — see below.

## Unordered associative containers (hash-based)

Average O(1) lookup; no ordering.

| Container | Header | Since | Notes |
|---|---|---|---|
| [`unordered_set`](unordered/unordered_set.md) | `<unordered_set>` | C++11 | Hashed unique keys |
| [`unordered_multiset`](unordered/unordered_multiset.md) | `<unordered_set>` | C++11 | Hashed keys, duplicates allowed |
| [`unordered_map`](unordered/unordered_map.md) | `<unordered_map>` | C++11 | Hashed key→value, unique keys |
| [`unordered_multimap`](unordered/unordered_multimap.md) | `<unordered_map>` | C++11 | Hashed key→value, duplicate keys |

## Container adaptors

Wrap one or more underlying containers to provide a restricted/different
interface. cppreference groups the sorted `flat_*` containers here too.

| Adaptor | Header | Since | Notes |
|---|---|---|---|
| [`stack`](adaptors/stack.md) | `<stack>` | C++98 | LIFO |
| [`queue`](adaptors/queue.md) | `<queue>` | C++98 | FIFO |
| [`priority_queue`](adaptors/priority_queue.md) | `<queue>` | C++98 | Heap; largest-first by default |
| [`flat_set` / `flat_multiset`](adaptors/flat_set.md) | `<flat_set>` | C++23 | Sorted set backed by a contiguous array |
| [`flat_map` / `flat_multimap`](adaptors/flat_map.md) | `<flat_map>` | C++23 | Sorted map backed by two arrays |

## Views (related — not containers)

Non-owning windows over existing storage.

| View | Header | Since | Notes |
|---|---|---|---|
| [`span`](views/span.md) | `<span>` | C++20 | Contiguous 1-D view |
| [`mdspan`](views/mdspan.md) | `<mdspan>` | C++23 | Multidimensional view |

---

## Which container should I use? (cheat sheet)

- **Need a resizable list of things?** → `vector` (default). Reach for something
  else only when a measured need says so.
- **Fixed number of elements known at compile time?** → `array` (or
  `inplace_vector` if the count varies up to a fixed cap, C++26).
- **Lots of insert/remove at *both* ends?** → `deque`.
- **Splicing nodes / stable element addresses / O(1) middle insert with an
  iterator?** → `list` (or `forward_list` to save memory).
- **Frequent insert/erase with *stable references to survivors*, order doesn't
  matter?** → `hive` (C++26) — like `list`'s stability but cache-friendly.
- **Keep keys sorted, need ordered traversal / range queries?** → `set` / `map`
  (or `flat_set` / `flat_map` for cache-friendly read-heavy use, C++23).
- **Just fast membership / lookup, order doesn't matter?** → `unordered_set` /
  `unordered_map`.
- **Restricted LIFO/FIFO/priority access?** → `stack` / `queue` /
  `priority_queue`.
- **Pass "a contiguous block of N elements" without copying or templating on the
  container?** → `span` (1-D) / `mdspan` (N-D).
