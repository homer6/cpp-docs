# Iterator operations & traits

> The free functions for moving iterators and measuring distances, plus the
> trait/sentinel machinery algorithms rely on. **Header:** `<iterator>`

## Moving and measuring

These adapt to the iterator's category automatically (O(1) for random access,
O(n) otherwise):

```cpp
#include <iterator>

auto it = v.begin();
std::advance(it, 3);             // move forward 3 (or back, if negative & bidi)
auto j = std::next(it);          // it + 1, without mutating it
auto k = std::next(it, 5);       // it + 5
auto p = std::prev(it);          // it - 1 (bidirectional+)
auto d = std::distance(v.begin(), v.end());   // number of steps between
```

Use `next`/`prev` where you'd want pointer arithmetic but the iterator might not
be random-access — e.g. stepping through a `list` or `set`.

## Swapping/moving the pointed-to elements

```cpp
std::iter_swap(it1, it2);        // swap *it1 and *it2
auto val = std::ranges::iter_move(it);   // move-out *it as an rvalue (C++20)
```

## std::iterator_traits — uniform access to an iterator's types

A layer that exposes an iterator's associated types, working for both class-type
iterators and raw pointers:

```cpp
using It = std::vector<int>::iterator;
std::iterator_traits<It>::value_type;        // int
std::iterator_traits<It>::difference_type;   // signed integer (ptrdiff_t)
std::iterator_traits<It>::reference;         // int&
std::iterator_traits<It>::iterator_category; // random_access_iterator_tag

std::iterator_traits<int*>::value_type;      // int — works for pointers too
```

Generic code uses these instead of assuming `It::value_type`, so raw pointers
qualify as iterators.

## ranges::begin / end and sentinels (C++20)

The ranges customization points return iterators/sentinels for anything
iterable, and a **sentinel** is an end-marker that need not be the same type as
the iterator (enabling lazy/unbounded ranges):

```cpp
auto first = std::ranges::begin(rng);
auto last  = std::ranges::end(rng);          // may be a sentinel, not an iterator
std::ranges::distance(rng);                   // size or counted distance
```

`std::default_sentinel` marks "the end" for ranges whose end isn't a position
(e.g. a counted iterator that stops after N, or a stream that stops at EOF).

## Gotchas

- **`std::distance` on input iterators consumes them.** For single-pass
  iterators (streams), measuring distance advances and exhausts the range.
- **`advance`/`distance` with negative counts need bidirectional+** iterators;
  on forward-only iterators a negative step is undefined.
- **Prefer `std::next(it, n)` to `it + n` in generic code** — `+` requires random
  access; `next` works (just slower) on weaker iterators.
- **Specialize `iterator_traits`, not the iterator, for odd cases** — but in
  C++20 you usually define the associated types directly and satisfy the
  concepts instead.

## See also

- [std::advance / next / prev / distance — cppreference](https://en.cppreference.com/w/cpp/iterator/advance)
- [std::iterator_traits — cppreference](https://en.cppreference.com/w/cpp/iterator/iterator_traits)
- [std::default_sentinel_t — cppreference](https://en.cppreference.com/w/cpp/iterator/default_sentinel_t)
