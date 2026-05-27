# Execution policies & ranges algorithms

> Two modern upgrades to the classic algorithms: opt-in parallelism (C++17) and
> the range-based, projection-aware `std::ranges::` overloads (C++20).
> **Headers:** `<execution>`, `<algorithm>`

## Execution policies (C++17)

Pass a policy as the first argument to opt into parallel/vectorized execution:

```cpp
#include <execution>
#include <algorithm>

std::sort(std::execution::seq,        v.begin(), v.end());   // sequential (default)
std::sort(std::execution::par,        v.begin(), v.end());   // parallel threads
std::sort(std::execution::par_unseq,  v.begin(), v.end());   // parallel + vectorized
std::for_each(std::execution::unseq,  v.begin(), v.end(), f);// vectorized, single thread (C++20)
```

| Policy | Meaning |
|---|---|
| `seq` | no parallelism; ordered |
| `par` | may use multiple threads |
| `par_unseq` | threads **and** SIMD; element ops may interleave |
| `unseq` | SIMD within one thread (C++20) |

**Rules:** with `par`/`par_unseq` your element operations must be free of data
races; with the `_unseq` variants they must not acquire locks or otherwise depend
on sequential execution. Throwing from a parallel element op calls
`std::terminate`.

> Availability: parallel algorithms need a backend (e.g. Intel TBB) on some
> standard libraries, and may fall back to sequential. Measure — parallelism only
> pays off above a problem-size threshold.

## std::ranges algorithms (C++20)

The `std::ranges::` overloads take a whole range, accept **projections**, return
richer result types, and give clean concept-checked errors:

```cpp
#include <algorithm>
#include <ranges>

std::ranges::sort(v);                                   // no .begin()/.end()
std::ranges::sort(people, {}, &Person::age);            // sort by a projected field
//                         ↑ comparator (default <)  ↑ projection

bool any = std::ranges::any_of(v, [](int x){ return x > 0; });
auto it  = std::ranges::find(v, 42);
std::ranges::for_each(v, fn);
auto n   = std::ranges::count_if(people, is_adult, &Person::age);
```

### Projections

A projection is a callable applied to each element *before* the predicate/
comparator — usually a pointer-to-member or a lambda. It removes the boilerplate
lambda that just reaches into a field:

```cpp
std::ranges::max_element(people, std::less<>{}, &Person::score);  // person with max score
```

### Borrowed iterators & dangling

When you pass a temporary range, ranges algorithms return `std::ranges::dangling`
instead of a would-be-dangling iterator, turning a runtime bug into a compile
error:

```cpp
auto it = std::ranges::find(make_vector(), 1);   // it is std::ranges::dangling — can't deref
```

## Gotchas

- **`par` requires race-free element ops.** Shared mutable state without
  synchronization is UB; prefer pure transforms / `reduce`.
- **Don't assume parallel is faster.** Thread/SIMD setup has overhead and a
  backend dependency; benchmark.
- **Ranges algorithms can't all be parallelized** — execution policies are mostly
  on the classic iterator overloads. Mixing the two styles is fine.
- **Projections apply to the predicate, not the result** — `find` still returns
  an iterator to the *element*, not the projected value.

## See also

- [Execution policies — cppreference](https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t)
- [Constrained (ranges) algorithms — cppreference](https://en.cppreference.com/w/cpp/algorithm/ranges)
- [std::ranges::dangling — cppreference](https://en.cppreference.com/w/cpp/ranges/dangling)
