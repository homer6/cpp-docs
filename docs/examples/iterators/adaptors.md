# Iterator adaptors

> Wrappers that turn one iterator (or a stream, or a container) into an iterator
> with different behavior — inserting, reversing, moving, counting.
> **Header:** `<iterator>`

## Insert iterators — write = grow the container

Assigning through these calls `push_back`/`push_front`/`insert` instead of
overwriting, so algorithms can *append* to a container.

```cpp
#include <iterator>
#include <vector>

std::vector<int> out;
std::copy(in.begin(), in.end(), std::back_inserter(out));   // push_back each
std::front_inserter(lst);                                    // push_front
std::inserter(s, s.end());                                   // insert at a position
```

`std::back_inserter` is the everyday one — "send results here, growing as needed."

## reverse_iterator — traverse backwards

```cpp
for (auto it = v.rbegin(); it != v.rend(); ++it) { /* last → first */ }
std::vector<int> rev(v.rbegin(), v.rend());      // reversed copy
```

`rbegin()/rend()` hand you `reverse_iterator`s; `++` on them moves toward the
front. (Note the off-by-one in `base()`: a reverse iterator points one *before*
its `base()`.)

## move_iterator — turn copies into moves (C++11)

Dereferencing yields an rvalue, so algorithms move elements instead of copying.

```cpp
std::vector<std::string> src = {...}, dst;
std::copy(std::make_move_iterator(src.begin()),
          std::make_move_iterator(src.end()),
          std::back_inserter(dst));              // elements moved out of src
```

## Stream iterators — treat I/O as a range

```cpp
#include <iterator>
#include <iostream>

// read all ints from stdin:
std::vector<int> nums{std::istream_iterator<int>{std::cin},
                      std::istream_iterator<int>{}};        // default = end

// write space-separated to stdout:
std::copy(nums.begin(), nums.end(),
          std::ostream_iterator<int>{std::cout, " "});
```

## C++20 ranges-related iterators

For the ranges library: `std::counted_iterator` (pairs an iterator with a
remaining count), `std::common_iterator` (unifies an iterator/sentinel pair into
one type for legacy APIs), and `std::default_sentinel` (a stateless end marker,
e.g. for counted ranges).

## Gotchas

- **`back_inserter` requires `push_back`.** It won't work on `array`/`set`
  (`set` uses `inserter`); pick the adaptor that matches the container's
  insertion method.
- **`reverse_iterator::base()` is offset by one** from where the reverse iterator
  "points." Converting between the two trips people up — `&*ri == &*(ri.base() -
  1)`.
- **`move_iterator` leaves the source moved-from.** Only use it when you're done
  with the source range.
- **`istream_iterator` skips whitespace and stops at the first failed extraction**
  — handy, but it silently ends on malformed input rather than erroring.

## See also

- [std::back_insert_iterator — cppreference](https://en.cppreference.com/w/cpp/iterator/back_insert_iterator)
- [std::reverse_iterator — cppreference](https://en.cppreference.com/w/cpp/iterator/reverse_iterator)
- [std::move_iterator — cppreference](https://en.cppreference.com/w/cpp/iterator/move_iterator)
- [std::istream_iterator — cppreference](https://en.cppreference.com/w/cpp/iterator/istream_iterator)
