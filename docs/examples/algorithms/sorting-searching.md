# Sorting & searching

> Ordering ranges and querying sorted ranges in O(log n). **Header:** `<algorithm>`

## Sorting

```cpp
#include <algorithm>
#include <functional>
#include <vector>

struct Player { int score; };

void sorts(std::vector<int>& v, std::vector<Player>& players) {
    std::sort(v.begin(), v.end());                       // ascending, O(n log n), not stable
    std::sort(v.begin(), v.end(), std::greater<>{});     // descending
    std::sort(players.begin(), players.end(),
              [](auto& a, auto& b){ return a.score < b.score; });   // by a field

    std::stable_sort(v.begin(), v.end());                // preserves order of equals
    std::is_sorted(v.begin(), v.end());                  // check
}
```

### Partial / selection

```cpp body
std::vector<int> v(100);
std::iota(v.begin(), v.end(), 0);
std::size_t n = v.size() / 2;

std::partial_sort(v.begin(), v.begin() + 10, v.end());   // 10 smallest, sorted; rest unspecified order
std::nth_element(v.begin(), v.begin() + n, v.end());      // nth in place; partitions around it
// nth_element is O(n) — the way to get a median or k-th smallest without full sort
```

## Binary search (range must be sorted)

These are O(log n) on a sorted range:

```cpp body
std::vector<int> v{1, 2, 2, 2, 5};                          // sorted
int x = 2;

bool found = std::binary_search(v.begin(), v.end(), x);    // present?
auto lo = std::lower_bound(v.begin(), v.end(), x);          // first element >= x
auto hi = std::upper_bound(v.begin(), v.end(), x);          // first element  > x
auto [a, b] = std::equal_range(v.begin(), v.end(), x);      // the [lo, hi) run of x
```

`lower_bound` doubles as "where would x go?" for sorted insertion.

## Merge & set operations (sorted inputs)

```cpp body
std::vector<int> a{1, 2, 3, 4}, b{2, 4}, out;               // a and b sorted
std::vector<int> v{1, 4, 7, 2, 5, 8};                       // two sorted halves
auto mid = v.begin() + 3;

std::merge(a.begin(), a.end(), b.begin(), b.end(),
           std::back_inserter(out));                  // merge two sorted ranges
std::inplace_merge(v.begin(), mid, v.end());          // merge two sorted halves in place

std::set_union       (a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
std::set_difference  (a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
std::includes(a.begin(), a.end(), b.begin(), b.end());     // is b ⊆ a?
```

## Gotchas

- **Binary search and set/merge operations REQUIRE sorted input** — and give
  wrong answers (no error) on unsorted ranges. Sort first, or keep the data
  sorted.
- **`std::sort` needs random-access iterators** → won't compile on `list`/
  `forward_list`; use their member `sort`. `set`/`map` are already sorted.
- **The comparator must be a strict weak ordering.** Using `<=` (not `<`), or an
  inconsistent comparator, is UB — classic cause of crashes inside `sort`.
- **`binary_search` only says yes/no.** To get the position/iterator, use
  `lower_bound` and check `*it == x`.
- **`nth_element` partially orders only.** After it, the nth element is correct
  and the range is partitioned around it, but neither side is sorted.

## See also

- [std::sort — cppreference](https://en.cppreference.com/w/cpp/algorithm/sort)
- [std::nth_element — cppreference](https://en.cppreference.com/w/cpp/algorithm/nth_element)
- [std::lower_bound — cppreference](https://en.cppreference.com/w/cpp/algorithm/lower_bound)
- [std::set_union etc. — cppreference](https://en.cppreference.com/w/cpp/algorithm/set_union)
