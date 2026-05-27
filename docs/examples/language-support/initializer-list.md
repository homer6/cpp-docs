# std::initializer_list

> The type a braced list `{1, 2, 3}` becomes when a constructor or function wants
> a homogeneous sequence. **Header:** `<initializer_list>` · **Since:** C++11

## Mental model

When you write `{a, b, c}` and the target takes a `std::initializer_list<T>`, the
compiler creates a small, read-only, contiguous array of the elements and hands
you a lightweight view of it. It's how `std::vector<int>{1,2,3}` and friends get
their elements.

## Examples

### Accepting one

```cpp
#include <initializer_list>

struct Bag {
    std::vector<int> data;
    Bag(std::initializer_list<int> il) : data{il} {}   // enables Bag{1,2,3}
};

void take(std::initializer_list<int> il) {
    for (int x : il) { /* ... */ }      // iterate with begin()/end()
    il.size();                           // count
}
take({1, 2, 3, 4});
```

### Why containers feel built-in

```cpp
std::vector<int>  v{1, 2, 3};        // calls vector(initializer_list<int>)
std::map<int,int> m{{1,10}, {2,20}};
auto sum = [](std::initializer_list<int> xs){ return std::accumulate(xs.begin(), xs.end(), 0); };
sum({1, 2, 3});                       // 6
```

## Gotchas

- **It's the #1 overload-resolution surprise.** `std::vector<int> v(3, 0);` makes
  `{0,0,0}`, but `std::vector<int> v{3, 0};` makes `{3, 0}` — a braced list
  **prefers** the `initializer_list` constructor over others. Choose `()` vs `{}`
  deliberately.
- **It's a non-owning view of a temporary array.** The backing array lives only as
  long as the `initializer_list` object; don't stash an `initializer_list` for
  later use — copy the elements out.
- **All elements must share one type** (`T`) — it's homogeneous. For heterogeneous
  fixed lists, use [`std::tuple`](../utilities/tuple.md).
- **Elements are `const`.** You can read but not modify them through the list, and
  you can't move out of it (so `initializer_list<unique_ptr<T>>` is unusable).

## See also

- [std::initializer_list — cppreference](https://en.cppreference.com/w/cpp/utility/initializer_list)
- [List initialization — cppreference](https://en.cppreference.com/w/cpp/language/list_initialization)
