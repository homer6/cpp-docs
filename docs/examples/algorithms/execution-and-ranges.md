# Execution policies & ranges algorithms

> Two modern upgrades to the classic algorithms: opt-in parallelism (C++17) and
> the range-based, projection-aware `std::ranges::` overloads (C++20).
> **Headers:** `<execution>`, `<algorithm>`

## Execution policies (C++17)

Pass a policy as the first argument to opt into parallel/vectorized execution:

```cpp
#include <execution>
#include <algorithm>
#include <vector>

void sort_all_ways(std::vector<int>& v) {
    auto f = [](int& x) { x *= 2; };
    std::sort(std::execution::seq,        v.begin(), v.end());   // sequential (default)
    std::sort(std::execution::par,        v.begin(), v.end());   // parallel threads
    std::sort(std::execution::par_unseq,  v.begin(), v.end());   // parallel + vectorized
    std::for_each(std::execution::unseq,  v.begin(), v.end(), f);// vectorized, single thread (C++20)
}
```

| Policy | Meaning |
|---|---|
| `seq` | no parallelism; ordered |
| `par` | may use multiple threads |
| `par_unseq` | threads **and** SIMD; element ops may interleave |
| `unseq` | SIMD within one thread (C++20) |

**Rules:** with `par`/`par_unseq` your element operations must be free of data
races; with the `_unseq` variants they must not acquire locks or otherwise depend
on sequential execution. An exception escaping an element op calls
`std::terminate` under every standard policy — even `seq`, unlike the overload
without a policy.

> Availability: parallel algorithms need a backend (e.g. Intel TBB) on some
> standard libraries, and may fall back to sequential. Measure — parallelism only
> pays off above a problem-size threshold.

## std::ranges algorithms (C++20)

The `std::ranges::` overloads take a whole range, accept **projections**, return
richer result types, and give clean concept-checked errors:

```cpp
#include <algorithm>
#include <ranges>
#include <string>
#include <vector>

struct Person { std::string name; int age; int score; };

void demo(std::vector<int>& v, std::vector<Person>& people) {
    auto fn       = [](int& x) { x *= 2; };
    auto is_adult = [](int age) { return age >= 18; };

    std::ranges::sort(v);                                   // no .begin()/.end()
    std::ranges::sort(people, {}, &Person::age);            // sort by a projected field
    //                         ↑ comparator (default <)  ↑ projection

    bool any = std::ranges::any_of(v, [](int x){ return x > 0; });
    auto it  = std::ranges::find(v, 42);
    std::ranges::for_each(v, fn);
    auto n   = std::ranges::count_if(people, is_adult, &Person::age);
}
```

### Projections

A projection is a callable applied to each element *before* the predicate/
comparator — usually a pointer-to-member or a lambda. It removes the boilerplate
lambda that just reaches into a field:

```cpp body
struct Person { std::string name; int score; };
std::vector<Person> people{{"Ann", 90}, {"Bob", 75}};
auto best = std::ranges::max_element(people, std::less<>{}, &Person::score);  // person with max score
```

### Borrowed iterators & dangling

When you pass a temporary range, ranges algorithms return `std::ranges::dangling`
instead of a would-be-dangling iterator, turning a runtime bug into a compile
error:

```cpp body
auto make_vector = [] { return std::vector<int>{1, 2, 3}; };
auto it = std::ranges::find(make_vector(), 1);   // it is std::ranges::dangling — can't deref
static_assert(std::same_as<decltype(it), std::ranges::dangling>);
```

### Folds (C++23)

`std::ranges::fold_left` is the ranges version of `std::accumulate`, with the
result type deduced from the operation rather than from `init` (so
`fold_left(doubles, 0, std::plus<>{})` is a `double`, where `accumulate` gives
an `int`). It has five siblings:

```cpp body
std::vector<int> v{1, 2, 3, 4};

int sum   = std::ranges::fold_left(v, 0, std::plus<>{});     // ((((0+1)+2)+3)+4) = 10
int left  = std::ranges::fold_left(v, 0, std::minus<>{});    // ((((0-1)-2)-3)-4) = -10
int right = std::ranges::fold_right(v, 0, std::minus<>{});   // 1-(2-(3-(4-0)))  = -2
auto first = std::ranges::fold_left_first(v, std::plus<>{}); // std::optional<int>{10}, no init
auto [it, total] = std::ranges::fold_left_with_iter(v, 0, std::plus<>{});  // it == v.end(), total == 10
```

`fold_right` calls `f(element, acc)`, so the arguments swap order.
`fold_left_first` and `fold_right_last` use the first or last element as the
initial value and return an empty `optional` for an empty range, and
`fold_left_first_with_iter` combines the two variations. The macro is
`__cpp_lib_ranges_fold` (`202207L`). GCC 15 and 16 have all six; libc++ in
Clang 22 has `fold_left` and `fold_left_with_iter` but not the others, so it
doesn't define the macro.

## Gotchas

- **`par` requires race-free element ops.** Shared mutable state without
  synchronization is UB; prefer pure transforms / `reduce`.
- **Don't assume parallel is faster.** Thread/SIMD setup has overhead and a
  backend dependency; benchmark.
- **Ranges algorithms can't all be parallelized** — execution policies are mostly
  on the classic iterator overloads. Mixing the two styles is fine.
- **Folds take no projection.** Unlike most ranges algorithms, `fold_left`
  and friends have no projection parameter; pipe through `views::transform`
  first.
- **Projections apply to the predicate, not the result** — `find` still returns
  an iterator to the *element*, not the projected value.

## See also

- [Execution policies — cppreference](https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t)
- [Constrained (ranges) algorithms — cppreference](https://en.cppreference.com/w/cpp/algorithm/ranges)
- [std::ranges::fold_left (C++23) — cppreference](https://en.cppreference.com/w/cpp/algorithm/ranges/fold_left)
- [P2322R6: ranges::fold](https://wg21.link/P2322R6)
- [std::ranges::dangling — cppreference](https://en.cppreference.com/w/cpp/ranges/dangling)
