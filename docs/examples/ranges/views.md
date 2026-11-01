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

### join_with — flatten with a separator

`join` flattens a range of ranges; `join_with(sep)` puts `sep` (one element or a
range) between the pieces. It's the lazy "join these strings with a comma":

```cpp body
namespace rv = std::views;
std::vector<std::string> words{"a", "bc", "d"};

auto csv  = words | rv::join_with(',') | std::ranges::to<std::string>();                        // "a,bc,d"
auto text = words | rv::join_with(std::string_view{", "}) | std::ranges::to<std::string>();    // "a, bc, d"
```

### cartesian_product — every combination

`cartesian_product(r1, r2, …)` yields a tuple for every combination, with the
last range varying fastest. It replaces nested loops, and its `size()` is the
product of the sizes:

```cpp body
namespace rv = std::views;
std::vector<int> xs{1, 2};
std::string ys = "ab";

for (auto [x, y] : rv::cartesian_product(xs, ys)) { /* (1,a) (1,b) (2,a) (2,b) */ }
auto n = rv::cartesian_product(xs, ys).size();   // 4
```

## New in C++26

### concat — one range after another

`concat(r1, r2, …)` presents ranges of compatible element types as one sequence.
Unlike `join` it takes ranges of *different* types, and it's random-access and
sized when every input is:

```cpp body
namespace rv = std::views;
std::vector<int>   a{1, 2};
std::array<int, 2> b{3, 4};

for (int x : rv::concat(a, b, rv::single(5))) { /* 1 2 3 4 5 */ }
auto ab = rv::concat(a, b);   // random_access_range, ab.size() == 4
```

### cache_latest — compute each element once

`filter` dereferences an element twice when it passes (once for the predicate,
once for the loop), so an expensive `transform` before it runs twice for those
elements. `cache_latest` stores the most recent element so it's computed once:

```cpp body
namespace rv = std::views;
std::vector<int> v{1, 2, 3, 4};
int calls = 0;
auto square = [&](int x) { ++calls; return x * x; };
auto even   = [](int x) { return x % 2 == 0; };

for (int x : v | rv::transform(square) | rv::filter(even)) {}                    // calls == 6
calls = 0;
for (int x : v | rv::transform(square) | rv::cache_latest | rv::filter(even)) {} // calls == 4
```

The result is an input range only (single pass), because the cache belongs to
the view, not the iterator.

### as_input — downgrade to an input range

`views::as_input` presents any range as input-only and non-common, so the
adaptors after it can skip the overhead of supporting stronger iterators. It
was added as `to_input` (P3137) and renamed `as_input` by P3828R1 (2026); the
macro became `__cpp_lib_ranges_as_input`, still `202502L`. GCC 16 has only the
new name, GCC 15 only `to_input` (`__cpp_lib_ranges_to_input`), Clang 22 neither:

```cpp body needs=as_input
namespace rv = std::views;
std::vector<int> v{1, 2, 3};
auto in = v | rv::as_input;
static_assert(std::ranges::input_range<decltype(in)>);
static_assert(!std::ranges::forward_range<decltype(in)>);
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
- **C++23 and C++26 adaptors need a recent stdlib.** Support is uneven even
  today: libc++ in Clang 22 has `zip`, `adjacent`, `chunk_by`, `as_rvalue` and
  `join_with` but not `enumerate`, `chunk`, `slide`, `stride`, `as_const`,
  `cartesian_product`, `concat` or `cache_latest`; GCC 15 and 16 have all of
  those. Check the `__cpp_lib_ranges_*` feature-test macros.
- **`cartesian_product` grows multiplicatively.** Three ranges of 1000 elements
  make a billion tuples; it's lazy, but `ranges::to` on it isn't.
- **`concat` elements are a common reference.** Mixing an `int` range with a
  `long` range yields `long` values, not references into either source.

## See also

- [Range adaptors — cppreference](https://en.cppreference.com/w/cpp/ranges#Range_adaptors)
- [std::views::filter / transform — cppreference](https://en.cppreference.com/w/cpp/ranges/filter_view)
- [std::views::zip / enumerate (C++23) — cppreference](https://en.cppreference.com/w/cpp/ranges/zip_view)
- [std::views::join_with (C++23) — cppreference](https://en.cppreference.com/w/cpp/ranges/join_with_view)
- [std::views::cartesian_product (C++23) — cppreference](https://en.cppreference.com/w/cpp/ranges/cartesian_product_view)
- [std::views::concat (C++26) — cppreference](https://en.cppreference.com/w/cpp/ranges/concat_view)
- [std::views::cache_latest (C++26) — cppreference](https://en.cppreference.com/w/cpp/ranges/cache_latest_view)
- [std::views::as_input (C++26) — cppreference](https://en.cppreference.com/w/cpp/ranges/as_input_view)
- [P3828R1: Rename the to_input view to as_input](https://wg21.link/P3828R1)
