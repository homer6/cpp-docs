# Views & pipelines

> The lazy range adaptors and the `|` operator that chains them.
> **Header:** `<ranges>` · `namespace std::views` (alias for `std::ranges::views`)

## The pipe operator

`range | adaptor` applies the adaptor; chaining reads in data-flow order. Each
adaptor returns a view; nothing runs until iteration.

```cpp body
#include <ranges>
namespace rv = std::views;

std::vector<int> data{1, 2, 3, 4, 5, 6, 7, 8};
auto pred = [](int x) { return x % 2 == 0; };
auto fn   = [](int x) { return x * 10; };

auto pipeline = data | rv::filter(pred) | rv::transform(fn) | rv::take(5);
```

`adaptor(range, args)` and `range | adaptor(args)` are equivalent — the pipe just
reads better when chaining.

## Core adaptors (C++20)

```cpp body
namespace rv = std::views;
std::vector<int> v{1, 2, 3, 4, 5, 6};
std::vector<std::vector<int>> nested{{1, 2}, {3}};
std::map<std::string, int> m{{"a", 1}, {"b", 2}};
std::string csv = "a,b,c";
auto pred = [](int x) { return x < 4; };
auto fn   = [](int x) { return x * 10; };
int n = 2;

auto r1  = v | rv::filter(pred);       // keep elements satisfying pred
auto r2  = v | rv::transform(fn);      // apply fn to each (map)
auto r3  = v | rv::take(n);            // first n
auto r4  = v | rv::take_while(pred);   // prefix while pred holds
auto r5  = v | rv::drop(n);            // skip first n
auto r6  = v | rv::drop_while(pred);   // skip leading run satisfying pred
auto r7  = v | rv::reverse;            // back-to-front (bidirectional ranges)
auto r8  = nested | rv::join;          // flatten a range-of-ranges
auto r9  = csv | rv::split(',');       // split into subranges on a delimiter
auto r10 = m | rv::elements<1>;        // Nth element of each tuple-like
auto r11 = m | rv::keys;               // = elements<0>  (e.g. map keys)
auto r12 = m | rv::values;             // = elements<1>  (e.g. map values)
auto r13 = r4 | rv::common;            // make begin/end the same type (for legacy APIs)
```

```cpp body
namespace rv = std::views;
std::map<std::string,int> m = {{"a", 1}, {"b", 2}};
for (const auto& k : m | rv::keys)   { /* every key */ }
for (int v : m | rv::values)         { /* every value */ }
```

## New in C++23

```cpp body
namespace rv = std::views;
std::vector<int> a{1, 2, 3, 4, 5, 6}, b{10, 20, 30};
int n = 2;

auto r1  = a | rv::enumerate;                         // yields (index, element) pairs
auto r2  = rv::zip(a, b);                             // zip N ranges into tuples, stops at the shortest
auto r3  = rv::zip_transform(std::plus<>{}, a, b);    // zip then apply fn
auto r4  = a | rv::adjacent<2>;                       // sliding tuples of N consecutive elements
auto r5  = a | rv::chunk(n);                          // group into sub-ranges of size n
auto r6  = a | rv::slide(n);                          // sliding windows of width n
auto r7  = a | rv::chunk_by(std::ranges::less{});     // split where pred(a,b) is false
auto r8  = a | rv::stride(n);                         // every nth element
auto r9  = a | rv::as_const;
auto r10 = a | rv::as_rvalue;
```

```cpp body
namespace rv = std::views;
std::vector<std::string> names{"ann", "bob", "cy"};

for (auto [i, name] : names | rv::enumerate) { /* i = 0,1,2..., name = element */ }

std::vector<int> a{1,2,3}, b{10,20,30};
for (auto [x, y] : rv::zip(a, b)) { /* (1,10), (2,20), (3,30) */ }
```

## Examples

```cpp body
namespace rv = std::views;
std::vector<int> data{1, 2, 3, 4, 5, 6, 7, 8};

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
- **C++23 adaptors need a recent stdlib.** Support is uneven even today: libc++
  in Clang 22 has `zip`, `adjacent`, `chunk_by` and `as_rvalue` but not
  `enumerate`, `chunk`, `slide`, `stride` or `as_const`. Check the
  `__cpp_lib_ranges_*` feature-test macros.

## See also

- [Range adaptors — cppreference](https://en.cppreference.com/w/cpp/ranges#Range_adaptors)
- [std::views::filter / transform — cppreference](https://en.cppreference.com/w/cpp/ranges/filter_view)
- [std::views::zip / enumerate (C++23) — cppreference](https://en.cppreference.com/w/cpp/ranges/zip_view)
