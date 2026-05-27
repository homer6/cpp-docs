# std::forward_list

> A singly-linked list, trimmed to the minimum: one pointer per node, no `size()`,
> no back pointer. **Header:** `<forward_list>` · **Since:** C++11

## Mental model

Like `std::list` but each node points only **forward**. There's no tail pointer
and no stored size, so the object is as small as a linked list can be. You can
only move forward, and insert/erase happen *after* a given position.

```
 head ─▶ [ a |·]─▶ [ b |·]─▶ [ c |·]─▶ nullptr
```

To insert "before" an element you actually operate on its predecessor — hence
the `_after` family of operations and the special `before_begin()` iterator.

## Declaration

```cpp
template<class T, class Allocator = std::allocator<T>>
class forward_list;
```

## Complexity

| Operation | Cost |
|---|---|
| `push_front` / `pop_front` | O(1) |
| `insert_after` / `erase_after` (given iterator) | O(1) |
| random access | not supported |
| `size` | **not provided** — compute with `std::distance` (O(n)) |

## Iterator & reference invalidation

Same guarantees as `list`: inserting never invalidates existing iterators or
references; erasing only affects the erased element. Splicing moves nodes without
invalidating them.

## When to use it

- You want a linked list and care about **per-element memory overhead** (one
  pointer instead of two), and you only ever traverse forward.
- Intrusive-style, memory-tight scenarios; long chains where you push/pop the
  front and splice.
- **Avoid** if you need size, bidirectional traversal, back insertion, or random
  access — that's `list`, `deque`, or `vector`. In practice `forward_list` is
  niche; most code is better served by `vector`.

## Examples

### The `_after` interface and `before_begin`

```cpp
#include <forward_list>

std::forward_list<int> fl{1, 2, 3};

fl.push_front(0);                       // {0,1,2,3}
fl.insert_after(fl.begin(), 99);        // insert after first → {0,99,1,2,3}

// To insert at the very front via the *_after API, use before_begin():
fl.insert_after(fl.before_begin(), -1); // {-1,0,99,1,2,3}

auto it = fl.begin();
fl.erase_after(it);                     // erase the element AFTER `it`
```

### No size(): count manually

```cpp
#include <iterator>
auto n = std::distance(fl.begin(), fl.end());   // O(n)
bool empty = fl.empty();                         // O(1), this one exists
```

### Member operations (like list)

```cpp
fl.sort();
fl.reverse();
fl.remove(99);
fl.unique();
fl.merge(/* another sorted forward_list */);

std::forward_list<int> other{7, 8};
fl.splice_after(fl.before_begin(), other);       // splice other to the front
```

### C++20 uniform erase

```cpp
std::erase(fl, 2);
std::erase_if(fl, [](int x){ return x < 0; });
```

## Gotchas

- **There is no `size()`.** By design — it would cost a stored counter that the
  "minimal list" doesn't want to pay for. Use `std::distance(begin(), end())` if
  you must, knowing it's O(n).
- **Everything is `_after`.** `insert_after`, `erase_after`, `emplace_after`,
  `splice_after`. To act at the front, anchor on `before_begin()`.
- **No `back()` / `push_back()`.** Only the front is cheap; reaching the end is
  O(n).
- **Same cache/allocation downsides as `list`.** Even more niche.

## See also

- [std::forward_list — cppreference](https://en.cppreference.com/w/cpp/container/forward_list)
