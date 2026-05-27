# Non-modifying algorithms

> Inspect a range without changing it: search, count, test predicates, compare.
> **Header:** `<algorithm>`

## Searching

```cpp
#include <algorithm>

auto it = std::find(v.begin(), v.end(), 42);          // first == 42, or end()
auto it2 = std::find_if(v.begin(), v.end(),
                        [](int x){ return x > 10; });  // first matching predicate
auto it3 = std::find_if_not(v.begin(), v.end(), pred);

// subsequence / element-of-set searches:
std::search(hay.begin(), hay.end(), needle.begin(), needle.end());  // find subrange
std::find_first_of(v.begin(), v.end(), set.begin(), set.end());     // any of a set
std::adjacent_find(v.begin(), v.end());               // first equal-adjacent pair
```

## Counting & testing

```cpp
std::count(v.begin(), v.end(), 7);                    // how many == 7
std::count_if(v.begin(), v.end(), pred);              // how many satisfy pred

std::all_of (v.begin(), v.end(), pred);               // every element?
std::any_of (v.begin(), v.end(), pred);               // at least one?
std::none_of(v.begin(), v.end(), pred);               // zero?
```

These three replace the classic "loop with a flag and an early break" — say what
you mean.

## Comparing ranges

```cpp
std::equal(a.begin(), a.end(), b.begin());            // element-wise equal
std::equal(a.begin(), a.end(), b.begin(), b.end());   // safe 4-iterator form

auto [i, j] = std::mismatch(a.begin(), a.end(), b.begin());  // first differing pair
std::lexicographical_compare(a.begin(), a.end(),
                             b.begin(), b.end());      // dictionary-order <
```

## Traversal

```cpp
std::for_each(v.begin(), v.end(), [](int& x){ /* side effects */ });
std::for_each_n(v.begin(), 5, fn);                    // first 5 (C++17)
```

(For a pure transformation, prefer [`std::transform`](modifying.md) or a
[ranges](../ranges/README.md) pipeline over `for_each` with side effects.)

## Gotchas

- **Prefer the 4-iterator `std::equal`/`mismatch`** (with both ends of the second
  range). The 3-iterator forms assume the second range is at least as long — a
  buffer-overrun footgun if it isn't.
- **`std::find` on a sorted range is still O(n).** If the range is sorted, use
  `std::binary_search` / `lower_bound` (see
  [sorting & searching](sorting-searching.md)) for O(log n).
- **Associative containers have member `find`/`count`/`contains`** that are
  O(log n) or O(1) — use those instead of the linear `std::find` on a `set`/`map`.

## See also

- [std::find / find_if — cppreference](https://en.cppreference.com/w/cpp/algorithm/find)
- [std::all_of / any_of / none_of — cppreference](https://en.cppreference.com/w/cpp/algorithm/all_any_none_of)
- [std::equal — cppreference](https://en.cppreference.com/w/cpp/algorithm/equal)
