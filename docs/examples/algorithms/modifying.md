# Modifying algorithms

> Produce or rearrange elements: copy, transform, fill, replace, remove, unique,
> reverse, rotate. **Header:** `<algorithm>`

## Copy / move / transform

```cpp
#include <algorithm>

std::copy(in.begin(), in.end(), std::back_inserter(out));   // append-copy
std::copy_if(in.begin(), in.end(), std::back_inserter(out), pred);
std::move(in.begin(), in.end(), out.begin());               // move elements over

std::transform(in.begin(), in.end(), out.begin(),
               [](int x){ return x * x; });                  // map one range
std::transform(a.begin(), a.end(), b.begin(), out.begin(),
               std::plus<>{});                               // zip two ranges
```

## Fill / generate

```cpp
std::fill(v.begin(), v.end(), 0);
std::fill_n(v.begin(), 5, -1);
std::generate(v.begin(), v.end(), [n = 0]() mutable { return n++; });  // 0,1,2,...
std::iota(v.begin(), v.end(), 1);              // 1,2,3,... (from <numeric>)
```

## Replace

```cpp
std::replace(v.begin(), v.end(), 0, -1);                  // every 0 → -1
std::replace_if(v.begin(), v.end(), pred, 0);
```

## Remove — and the erase-remove idiom

`std::remove`/`remove_if` **don't shrink the container**. They shuffle the kept
elements to the front and return an iterator to the new logical end; the tail is
left as unspecified leftovers. You must `erase` that tail.

```cpp
// pre-C++20:
v.erase(std::remove(v.begin(), v.end(), 0), v.end());        // drop all 0s
v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());

// C++20 — say it in one line:
std::erase(v, 0);
std::erase_if(v, pred);
```

## unique — collapse *consecutive* duplicates

```cpp
std::sort(v.begin(), v.end());                                // unique needs them adjacent
v.erase(std::unique(v.begin(), v.end()), v.end());            // dedup a sorted range
```

## Reorder

```cpp
std::reverse(v.begin(), v.end());
std::rotate(v.begin(), v.begin() + 3, v.end());   // bring element 3 to the front
std::shuffle(v.begin(), v.end(), rng);             // random permutation (needs a generator)
std::sample(in.begin(), in.end(), std::back_inserter(out), 5, rng);  // pick 5 (C++17)
```

## Gotchas

- **`remove`/`unique` only mark, never resize.** Forgetting the `erase` leaves
  the container its original size with garbage at the end — the #1 algorithm bug.
  Prefer `std::erase`/`erase_if` (C++20).
- **`unique` only removes *adjacent* duplicates.** Sort first to remove all
  duplicates.
- **`transform` can write in place** (`out` == `in`), but writing into a *shorter*
  destination is UB — size the output (or use `back_inserter`).
- **`std::move` (algorithm) leaves sources moved-from.** Don't reuse the source
  values afterward.

## See also

- [std::transform — cppreference](https://en.cppreference.com/w/cpp/algorithm/transform)
- [std::remove / remove_if — cppreference](https://en.cppreference.com/w/cpp/algorithm/remove)
- [std::unique — cppreference](https://en.cppreference.com/w/cpp/algorithm/unique)
- [std::rotate — cppreference](https://en.cppreference.com/w/cpp/algorithm/rotate)
