# Ranges library

> Compose lazy, readable data pipelines over ranges. **Header:** `<ranges>` ·
> **Since:** C++20 (more views in C++23)

Mirrors [cppreference's ranges library](https://en.cppreference.com/w/cpp/ranges).

| Page | What |
|---|---|
| [Range concepts & access](range-concepts.md) | what a *range* and a *view* are; `begin`/`end`/`size`; dangling |
| [Views & pipelines](views.md) | `filter`, `transform`, `take`, `zip`, `enumerate`, the `\|` operator |
| [Factories & `ranges::to`](factories.md) | `iota`, `repeat`, `single`; materializing a view back into a container |

## The one-paragraph pitch

A **view** is a cheap, non-owning, lazy adaptor over a range. Chain views with
`|` to express a transformation declaratively; nothing is computed until you
iterate, and no intermediate containers are allocated.

```cpp
#include <ranges>
namespace rv = std::views;

auto result = data
    | rv::filter([](int x){ return x % 2 == 0; })   // keep evens
    | rv::transform([](int x){ return x * x; })      // square them
    | rv::take(3);                                    // first 3

for (int x : result) { /* lazily: filter→transform→take, element by element */ }
```

Compare to the imperative loop or the temporary-vector-per-step style — the
pipeline reads top-to-bottom in data-flow order.
