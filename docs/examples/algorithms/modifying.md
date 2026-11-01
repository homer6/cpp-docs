# Modifying algorithms

> Produce or rearrange elements: copy, transform, fill, replace, remove, unique,
> reverse, rotate. **Header:** `<algorithm>`

## Copy / move / transform

```cpp
#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>

void copy_and_transform(std::vector<int>& in, std::vector<int>& out,
                        const std::vector<int>& a, const std::vector<int>& b) {
    auto pred = [](int x) { return x % 2 == 0; };

    std::copy(in.begin(), in.end(), std::back_inserter(out));   // append-copy
    std::copy_if(in.begin(), in.end(), std::back_inserter(out), pred);
    std::move(in.begin(), in.end(), out.begin());               // move elements over

    std::transform(in.begin(), in.end(), out.begin(),
                   [](int x){ return x * x; });                  // map one range
    std::transform(a.begin(), a.end(), b.begin(), out.begin(),
                   std::plus<>{});                               // zip two ranges
}
```

## Fill / generate

```cpp body
std::vector<int> v(10);
std::fill(v.begin(), v.end(), 0);
std::fill_n(v.begin(), 5, -1);
std::generate(v.begin(), v.end(), [n = 0]() mutable { return n++; });  // 0,1,2,...
std::iota(v.begin(), v.end(), 1);              // 1,2,3,... (from <numeric>)
```

## Replace

```cpp body
std::vector<int> v{0, 5, 0, -3};
auto pred = [](int x) { return x < 0; };
std::replace(v.begin(), v.end(), 0, -1);                  // every 0 → -1
std::replace_if(v.begin(), v.end(), pred, 0);
```

## Remove — and the erase-remove idiom

`std::remove`/`remove_if` **don't shrink the container**. They shuffle the kept
elements to the front and return an iterator to the new logical end; the tail is
left as unspecified leftovers. You must `erase` that tail.

```cpp body
std::vector<int> v{0, 1, 0, 2, -3};
auto pred = [](int x) { return x < 0; };

// pre-C++20:
v.erase(std::remove(v.begin(), v.end(), 0), v.end());        // drop all 0s
v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());

// C++20 — say it in one line:
std::erase(v, 0);
std::erase_if(v, pred);
```

## unique — collapse *consecutive* duplicates

```cpp body
std::vector<int> v{3, 1, 3, 2, 1};
std::sort(v.begin(), v.end());                                // unique needs them adjacent
v.erase(std::unique(v.begin(), v.end()), v.end());            // dedup a sorted range
```

## Reorder

```cpp
#include <algorithm>
#include <iterator>
#include <random>
#include <vector>

void reorder(std::vector<int>& v, const std::vector<int>& in, std::vector<int>& out) {
    std::mt19937 rng{std::random_device{}()};

    std::reverse(v.begin(), v.end());
    std::rotate(v.begin(), v.begin() + 3, v.end());   // bring element 3 to the front
    std::shuffle(v.begin(), v.end(), rng);             // random permutation (needs a generator)
    std::sample(in.begin(), in.end(), std::back_inserter(out), 5, rng);  // pick 5 (C++17)
}
```

### shift_left / shift_right (C++20)

`std::shift_left(first, last, n)` moves the elements `n` places toward the
front and returns the new end; `shift_right` moves them toward the back and
returns the new beginning. Unlike `rotate`, nothing wraps around: the vacated
slots hold moved-from values.

```cpp body
std::vector<int> v{1, 2, 3, 4, 5};
auto end = std::shift_left(v.begin(), v.end(), 2);     // v starts 3,4,5; end == v.begin() + 3

std::vector<int> w{1, 2, 3, 4, 5};
auto beg = std::shift_right(w.begin(), w.end(), 2);    // w ends 1,2,3; beg == w.begin() + 2

auto same = std::shift_left(v.begin(), v.end(), 9);    // n >= size: nothing moves, returns v.begin()
```

The range forms `std::ranges::shift_left` / `shift_right` (C++23, P2440,
`__cpp_lib_shift` `202202L`) return the shifted subrange. GCC 16 has them; GCC 15
and Clang 22 don't.

## Gotchas

- **`remove`/`unique` only mark, never resize.** Forgetting the `erase` leaves
  the container its original size with garbage at the end — the #1 algorithm bug.
  Prefer `std::erase`/`erase_if` (C++20).
- **`unique` only removes *adjacent* duplicates.** Sort first to remove all
  duplicates.
- **`transform` can write in place** (`out` == `in`), but writing into a *shorter*
  destination is UB — size the output (or use `back_inserter`).
- **`shift_left`/`shift_right` leave a moved-from tail.** Only `[first, end)`
  (or `[beg, last)`) holds meaningful values; erase or overwrite the rest.
- **`std::move` (algorithm) leaves sources moved-from.** Don't reuse the source
  values afterward.

## See also

- [std::transform — cppreference](https://en.cppreference.com/w/cpp/algorithm/transform)
- [std::remove / remove_if — cppreference](https://en.cppreference.com/w/cpp/algorithm/remove)
- [std::unique — cppreference](https://en.cppreference.com/w/cpp/algorithm/unique)
- [std::rotate — cppreference](https://en.cppreference.com/w/cpp/algorithm/rotate)
- [std::shift_left / shift_right — cppreference](https://en.cppreference.com/w/cpp/algorithm/shift)
