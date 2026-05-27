# std::list

> A doubly-linked list: O(1) insert/erase anywhere given an iterator, and stable
> element addresses, at the cost of no random access and poor locality.
> **Header:** `<list>` · **Since:** C++98

## Mental model

Independently heap-allocated nodes, each holding the element plus `prev`/`next`
pointers. Nothing is contiguous; you traverse by following links.

```
 nullptr ◀─┐                            ┌─▶ nullptr
        ┌──┴──┐   ┌─────┐   ┌─────┐   ┌──┴──┐
        │  a  │⇄ │  b  │⇄ │  c  │⇄ │  d  │
        └─────┘   └─────┘   └─────┘   └─────┘
```

Because each node is separate and never moves, an element's address is stable
for its whole lifetime, and you can splice nodes between lists without copying.

## Declaration

```cpp
template<class T, class Allocator = std::allocator<T>>
class list;
```

## Complexity

| Operation | Cost |
|---|---|
| `push_front` / `push_back` / `pop_*` | O(1) |
| `insert` / `erase` **at a known iterator** | O(1) |
| random access `list[i]` | **not supported** |
| `find` (must traverse) | O(n) |
| `splice` | O(1) (or O(n) for the count-taking overloads) |
| `size` | O(1) (since C++11) |

## Iterator & reference invalidation

`list`'s headline feature: **insertion never invalidates anything; erasure only
invalidates iterators/references to the erased element.** Splicing moves nodes
between lists *without* invalidating iterators to the moved elements — they keep
pointing at the same node, now in the other list.

## When to use it

- You need **O(1) splicing** — moving sub-ranges between lists, or reordering
  without moving elements.
- You need **stable addresses/iterators** to elements while inserting and erasing
  elsewhere.
- **Avoid** in most other cases. The per-node allocation and pointer chasing make
  it slower than `vector`/`deque` for almost everything, including traversal.
  "Linked list" is rarely the right default in modern C++ — measure first.

## Examples

```cpp
#include <list>

std::list<int> a{1, 2, 3};
a.push_front(0);          // {0,1,2,3}
a.push_back(4);           // {0,1,2,3,4}

auto it = std::next(a.begin(), 2);   // no a[2]; step manually
a.insert(it, 99);                    // O(1) given the iterator
a.erase(it);                         // O(1)
```

### Member operations that algorithms can't do as well

```cpp
std::list<int> x{1, 2, 3}, y{4, 5, 6};

x.splice(x.end(), y);     // move all of y onto the end of x in O(1); y now empty
x.sort();                 // list's own sort (std::sort needs random access)
x.merge(/* another sorted list */);
x.remove(2);              // remove all elements == 2
x.remove_if([](int n){ return n % 2 == 0; });
x.unique();               // collapse consecutive duplicates
x.reverse();
```

### C++20 uniform erase

```cpp
std::erase(x, 3);                                  // by value
std::erase_if(x, [](int n){ return n > 4; });      // by predicate
```

## Gotchas

- **No `operator[]` / `at`.** No random access at all — use `std::next` /
  `std::advance`, or pick a different container.
- **Prefer member `sort`/`remove`/`unique` over the `<algorithm>` ones.**
  `std::sort` requires random-access iterators (won't compile on `list`);
  `list::sort` is built for it. `list::remove` actually deletes nodes, whereas
  `std::remove` only shuffles values.
- **Memory & cache cost.** One allocation per element and pointers everywhere;
  traversal misses cache constantly. Usually slower than a `vector` even for
  "lots of middle inserts," once you account for finding the position.

## See also

- [std::list — cppreference](https://en.cppreference.com/w/cpp/container/list)
- [std::list::splice — cppreference](https://en.cppreference.com/w/cpp/container/list/splice)
