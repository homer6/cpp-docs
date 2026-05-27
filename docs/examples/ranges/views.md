# Views & pipelines

> The lazy range adaptors and the `|` operator that chains them.
> **Header:** `<ranges>` · `namespace std::views` (alias for `std::ranges::views`)

## The pipe operator

`range | adaptor` applies the adaptor; chaining reads in data-flow order. Each
adaptor returns a view; nothing runs until iteration.

```cpp
#include <ranges>
namespace rv = std::views;

auto pipeline = data | rv::filter(pred) | rv::transform(fn) | rv::take(5);
```

`adaptor(range, args)` and `range | adaptor(args)` are equivalent — the pipe just
reads better when chaining.

## Core adaptors (C++20)

```cpp
rv::filter(pred)         // keep elements satisfying pred
rv::transform(fn)        // apply fn to each (map)
rv::take(n)              // first n
rv::take_while(pred)     // prefix while pred holds
rv::drop(n)              // skip first n
rv::drop_while(pred)     // skip leading run satisfying pred
rv::reverse              // back-to-front (bidirectional ranges)
rv::join                 // flatten a range-of-ranges
rv::split(delim)         // split into subranges on a delimiter
rv::elements<N>          // Nth element of each tuple-like
rv::keys                 // = elements<0>  (e.g. map keys)
rv::values               // = elements<1>  (e.g. map values)
rv::common               // make begin/end the same type (for legacy APIs)
```

```cpp
std::map<std::string,int> m = {...};
for (const auto& k : m | rv::keys)   { /* every key */ }
for (int v : m | rv::values)         { /* every value */ }
```

## New in C++23

```cpp
rv::enumerate            // yields (index, element) pairs
rv::zip                  // zip N ranges into tuples, stops at the shortest
rv::zip_transform(fn)    // zip then apply fn
rv::adjacent<N>          // sliding tuples of N consecutive elements
rv::chunk(n)             // group into sub-ranges of size n
rv::slide(n)             // sliding windows of width n
rv::chunk_by(pred)       // split where pred(a,b) is false
rv::stride(n)            // every nth element
rv::as_const, rv::as_rvalue
```

```cpp
for (auto [i, name] : names | rv::enumerate) { /* i = 0,1,2..., name = element */ }

std::vector<int> a{1,2,3}, b{10,20,30};
for (auto [x, y] : rv::zip(a, b)) { /* (1,10), (2,20), (3,30) */ }
```

## Examples

```cpp
// sum of squares of the first 3 even numbers in `data`:
int s = 0;
for (int x : data | rv::filter([](int n){ return n%2==0; })
                  | rv::transform([](int n){ return n*n; })
                  | rv::take(3))
    s += x;

// split a CSV line:
std::string line = "a,b,c";
for (auto field : line | rv::split(','))
    std::string_view sv{field.begin(), field.end()};   // each token
```

## Gotchas

- **Lazy means re-evaluated.** Iterating a `filter|transform` view twice runs the
  work twice. Materialize with [`ranges::to`](factories.md) if you need to reuse
  results or want a container.
- **`filter` + mutation / caching surprises.** `filter_view` caches its first
  element's position; mutating the underlying range through the view can break
  its invariants. Treat filtered views as read-mostly.
- **Views don't own.** A pipeline over a temporary container dangles — keep the
  source alive while iterating.
- **C++23 adaptors need a recent stdlib.** `zip`, `enumerate`, `chunk`, etc. may
  be missing on older toolchains; check support.

## See also

- [Range adaptors — cppreference](https://en.cppreference.com/w/cpp/ranges#Range_adaptors)
- [std::views::filter / transform — cppreference](https://en.cppreference.com/w/cpp/ranges/filter_view)
- [std::views::zip / enumerate (C++23) — cppreference](https://en.cppreference.com/w/cpp/ranges/zip_view)
