# Numeric algorithms (`<numeric>`)

> Fold/scan a range to a value or a running sequence: sums, products, prefix
> sums, inner products, and the parallel-friendly `reduce` family.
> **Header:** `<numeric>`

## accumulate — the classic left fold

```cpp
#include <numeric>

int sum  = std::accumulate(v.begin(), v.end(), 0);              // 0 + each
int prod = std::accumulate(v.begin(), v.end(), 1, std::multiplies<>{});
std::string joined = std::accumulate(words.begin(), words.end(), std::string{},
        [](std::string acc, const std::string& w){ return acc + w; });
```

The init value's **type drives the accumulation** — `accumulate(v, 0)` over a
`vector<double>` truncates to `int`. Use `0.0` for doubles.

## reduce / transform_reduce — same idea, parallelizable (C++17)

`reduce` is like `accumulate` but the operation must be **associative and
commutative**, which lets it run out of order (and in parallel with a policy):

```cpp
int s = std::reduce(v.begin(), v.end());                          // sum, may reorder
int s2 = std::reduce(std::execution::par, v.begin(), v.end());    // parallel sum

// map-then-reduce in one pass (dot product, etc.):
double dot = std::transform_reduce(a.begin(), a.end(), b.begin(), 0.0);  // Σ aᵢ·bᵢ
int n = std::transform_reduce(v.begin(), v.end(), 0,
                              std::plus<>{}, [](int x){ return x > 0; });  // count positives
```

`std::inner_product` is the older, sequential dot-product equivalent.

## Scans — running totals

```cpp
std::partial_sum(v.begin(), v.end(), out.begin());        // inclusive prefix sums
std::inclusive_scan(v.begin(), v.end(), out.begin());      // C++17, parallelizable
std::exclusive_scan(v.begin(), v.end(), out.begin(), 0);   // each excludes itself
std::adjacent_difference(v.begin(), v.end(), out.begin()); // out[i] = v[i] - v[i-1]
```

## Small but handy

```cpp
std::iota(v.begin(), v.end(), 1);     // fill 1,2,3,...
std::gcd(12, 18);                      // 6   (C++17)
std::lcm(4, 6);                        // 12  (C++17)
std::midpoint(a, b);                   // overflow-safe average (C++20)
```

## Gotchas

- **The accumulator/init type matters.** `accumulate(doubles, 0)` accumulates in
  `int` and loses fractions; pass `0.0`. Likewise watch for overflow — pick a
  wide enough init type.
- **`reduce` may reorder operations** → wrong results for non-associative ops
  (e.g. subtraction) or floating-point where order changes rounding. Use
  `accumulate` when order matters.
- **Parallel `reduce`/`transform_reduce` require `<execution>` and a linkable
  parallel backend** (e.g. TBB on some stdlibs) — see
  [execution policies](execution-and-ranges.md).

## See also

- [std::accumulate — cppreference](https://en.cppreference.com/w/cpp/algorithm/accumulate)
- [std::reduce — cppreference](https://en.cppreference.com/w/cpp/algorithm/reduce)
- [std::transform_reduce — cppreference](https://en.cppreference.com/w/cpp/algorithm/transform_reduce)
- [std::inclusive_scan — cppreference](https://en.cppreference.com/w/cpp/algorithm/inclusive_scan)
