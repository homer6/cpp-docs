# Range factories & `ranges::to`

> Generating ranges from nothing, and materializing a view back into a container.
> **Header:** `<ranges>` · **Since:** C++20 (`ranges::to` and some factories: C++23)

## Factories — ranges out of thin air

```cpp
#include <ranges>
namespace rv = std::views;

rv::iota(0)             // 0,1,2,3,...  (infinite — pair with take!)
rv::iota(1, 5)          // 1,2,3,4      (bounded)
rv::single(42)          // a one-element range: {42}
rv::empty<int>          // an empty range of int
rv::repeat(7)           // 7,7,7,...    (infinite, C++23)
rv::repeat(7, 3)        // 7,7,7        (bounded, C++23)
rv::istream<int>(std::cin)   // lazily read ints from a stream
```

```cpp
// first 10 squares, lazily:
for (int sq : rv::iota(1) | rv::transform([](int n){ return n*n; }) | rv::take(10))
    /* 1,4,9,...,100 */;
```

`iota` + `take` is the idiomatic bounded counter; `iota` alone is infinite, so
always bound it before iterating to completion.

## ranges::to — view → container (C++23)

Collect a lazy pipeline into a real container in one call:

```cpp
#include <ranges>

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
- **`views::istream` is single-pass** (input range) — you can't restart it.
- **Materializing re-runs the pipeline once.** That's the point of `to` — to pay
  the lazy cost a single time and keep the results.

## See also

- [std::views::iota — cppreference](https://en.cppreference.com/w/cpp/ranges/iota_view)
- [std::views::repeat (C++23) — cppreference](https://en.cppreference.com/w/cpp/ranges/repeat_view)
- [std::ranges::to (C++23) — cppreference](https://en.cppreference.com/w/cpp/ranges/to)
