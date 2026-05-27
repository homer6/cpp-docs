# Function objects (`<functional>`)

> Storing, wrapping, and composing callables: `std::function`, `std::invoke`,
> `std::bind`/lambdas, `std::ref`, and the standard operator functors.
> **Header:** `<functional>`

## std::function — a type-erased callable

Holds *any* callable matching a signature (free function, lambda, functor, bound
member). Costs type erasure (often a heap allocation + indirect call), so use it
where you genuinely need to store heterogeneous callables, not in hot loops.

```cpp
#include <functional>

std::function<int(int, int)> op = [](int a, int b){ return a + b; };
op = std::multiplies<int>{};           // reassign to a different callable
int r = op(6, 7);                       // 42
if (op) { /* non-empty */ }             // empty std::function → calling it throws std::bad_function_call
```

### std::move_only_function (C++23)

`std::function` requires the callable be *copyable*. `std::move_only_function`
drops that — it can hold move-only callables (e.g. a lambda capturing a
`unique_ptr`), and supports `const`/`noexcept`/ref qualifiers in its signature.

```cpp
std::move_only_function<void()> task =
    [p = std::make_unique<int>(5)]{ /* uses *p */ };   // capture is move-only
```

## std::invoke — call anything uniformly (C++17)

One spelling to call free functions, function objects, **and** pointers-to-member
(which use different syntax otherwise):

```cpp
struct S { int f(int) const; int data; };
S s;
std::invoke(&S::f, s, 10);       // calls s.f(10)
std::invoke(&S::data, s);        // accesses s.data
std::invoke(some_lambda, args);  // calls the lambda
```

`std::invoke` is the foundation the library calls callables through (e.g.
algorithms, `std::apply`). `std::invoke_r<R>` (C++23) fixes the return type.

## ref / cref — pass references through value-copying APIs

```cpp
int counter = 0;
auto inc = [](int& c){ ++c; };
std::invoke(inc, std::ref(counter));    // reference_wrapper → counter is modified
```

Many library facilities copy their arguments (e.g. `std::thread`, `std::bind`);
`std::ref`/`std::cref` wrap a reference so it survives the copy.

## Operator functors

Ready-made callables for algorithms and containers:

```cpp
std::plus<>{}(2, 3);             // 5  (transparent: deduces operand types, C++14)
std::less<>{};                   // default comparator for std::sort, std::set, ...
std::greater<int>{};             // descending sort / min-heap
// also: minus, multiplies, divides, modulus, negate,
//       equal_to, not_equal_to, logical_and/or/not, bit_and/or/xor
```

## std::bind vs. lambdas

`std::bind` (C++11) pre-binds arguments, but **lambdas are clearer and usually
preferred** today:

```cpp
auto add = [](int a, int b){ return a + b; };
auto add5 = [add](int b){ return add(5, b); };        // prefer this
// std::bind equivalent (placeholders), kept mostly for legacy code:
auto add5b = std::bind(add, 5, std::placeholders::_1);
```

## Gotchas

- **`std::function` is not free.** Type erasure usually means an allocation and an
  indirect call; for known callable types, take a template parameter or
  `auto&&`/a concept-constrained callable instead.
- **Calling an empty `std::function` throws `std::bad_function_call`.** Check it
  in a bool context first.
- **Prefer lambdas to `std::bind`.** `bind`'s placeholders, nested binds, and
  argument-decay rules are error-prone; a lambda is explicit.
- **Dangling captures / refs.** `std::ref` and reference-capturing lambdas don't
  extend lifetimes — the referent must outlive every call.

## See also

- [std::function — cppreference](https://en.cppreference.com/w/cpp/utility/functional/function)
- [std::move_only_function — cppreference](https://en.cppreference.com/w/cpp/utility/functional/move_only_function)
- [std::invoke — cppreference](https://en.cppreference.com/w/cpp/utility/functional/invoke)
- [std::ref / std::cref — cppreference](https://en.cppreference.com/w/cpp/utility/functional/ref)
