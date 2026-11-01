# Range factories & `ranges::to`

> Generating ranges from nothing, and materializing a view back into a container.
> **Header:** `<ranges>` · **Since:** C++20 (`ranges::to` and some factories: C++23)

## Factories — ranges out of thin air

```cpp body
#include <ranges>
namespace rv = std::views;

auto a = rv::iota(0);           // 0,1,2,3,...  (infinite — pair with take!)
auto b = rv::iota(1, 5);        // 1,2,3,4      (bounded)
auto c = rv::single(42);        // a one-element range: {42}
auto d = rv::empty<int>;        // an empty range of int
auto e = rv::repeat(7);         // 7,7,7,...    (infinite, C++23)
auto f = rv::repeat(7, 3);      // 7,7,7        (bounded, C++23)
auto g = rv::istream<int>(std::cin);   // lazily read ints from a stream
```

```cpp body
namespace rv = std::views;

// first 10 squares, lazily:
for (int sq : rv::iota(1) | rv::transform([](int n){ return n*n; }) | rv::take(10))
    /* 1,4,9,...,100 */;
```

`iota` + `take` is the idiomatic bounded counter; `iota` alone is infinite, so
always bound it before iterating to completion.

### views::indices — 0 to n of n's type (C++26)

`iota` refuses to mix a signed start with an unsigned bound, so the obvious
loop over a container's indices doesn't compile:

```cpp body compile-fail="iota|no match|no matching"
std::vector<double> v{1.5, 2.5, 3.5};
for (auto i : std::views::iota(0, v.size())) {}   // error: int vs std::size_t
```

`views::indices(n)` is `views::iota(T(0), n)` where `T` is `n`'s type, so it
counts from zero in the right type. The macro is `__cpp_lib_ranges_indices`
(`202506L`); GCC 16 and Clang 22 have it, GCC 15 doesn't:

```cpp body needs=indices
std::vector<double> v{1.5, 2.5, 3.5};
for (auto i : std::views::indices(v.size())) { /* i = 0, 1, 2 as std::size_t */ }
```

## ranges::to — view → container (C++23)

Collect a lazy pipeline into a real container in one call:

```cpp body
#include <ranges>
namespace rv = std::views;

std::vector<int> data{1, 2, 3, 4};
std::vector<std::pair<int, std::string>> pairs{{1, "one"}, {2, "two"}};

auto evens = data | rv::filter([](int x){ return x % 2 == 0; })
                  | std::ranges::to<std::vector>();      // std::vector<int>

auto squared = rv::iota(1, 6)
             | rv::transform([](int n){ return n*n; })
             | std::ranges::to<std::vector<int>>();      // {1,4,9,16,25}

// works for associative containers too:
auto m = pairs | std::ranges::to<std::map<int, std::string>>();
```

`ranges::to<C>()` deduces or takes the target container type and constructs it
from the range — the bridge from lazy views back to owning storage.

## Gotchas

- **Infinite factories must be bounded before full iteration.** `rv::iota(0)` or
  `rv::repeat(x)` will loop forever in a range-`for`; compose `take`/`take_while`
  first.
- **`ranges::to` is C++23.** Pre-C++23, materialize manually:
  `std::vector<int> out(view.begin(), view.end());` (needs a `common_range`, or
  wrap with `rv::common`).
- **`iota(0, v.size())` doesn't compile** — start and bound must agree on
  signedness. Use `iota(0uz, v.size())` or, in C++26, `indices(v.size())`.
- **`views::istream` is single-pass** (input range) — you can't restart it.
- **Materializing re-runs the pipeline once.** That's the point of `to` — to pay
  the lazy cost a single time and keep the results.

## See also

- [std::views::iota — cppreference](https://en.cppreference.com/w/cpp/ranges/iota_view)
- [std::views::repeat (C++23) — cppreference](https://en.cppreference.com/w/cpp/ranges/repeat_view)
- [std::ranges::to (C++23) — cppreference](https://en.cppreference.com/w/cpp/ranges/to)
