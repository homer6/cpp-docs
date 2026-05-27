# std::valarray

> A numeric array with built-in element-wise arithmetic and slicing — an early
> attempt at array math. **Header:** `<valarray>` · **Since:** C++98

> Largely **legacy**. It predates expression templates and modern numeric
> libraries; for serious array math prefer a dedicated library (Eigen, xtensor),
> or C++26's [`mdspan`](../containers/views/mdspan.md) + [`linalg`](linalg.md) /
> [SIMD](simd.md). Documented here for completeness.

## Mental model

A 1-D array where the operators apply element-wise across the whole array — `a +
b` adds element by element, `std::sqrt(a)` takes the root of each element.

```cpp
#include <valarray>

std::valarray<double> a{1, 2, 3, 4};
std::valarray<double> b{10, 20, 30, 40};

auto c = a + b;                  // {11, 22, 33, 44}  — element-wise
auto d = a * 2.0;                // {2, 4, 6, 8}
auto e = std::sqrt(a);           // element-wise sqrt
double total = a.sum();          // 10
double biggest = a.max();
```

## Slicing

`valarray` supports strided/indexed access via `std::slice`, `std::gslice`,
masks, and indirect arrays:

```cpp
std::valarray<int> v{0,1,2,3,4,5,6,7};
auto evens = v[std::slice(0, 4, 2)];     // start 0, count 4, stride 2 → {0,2,4,6}
v[v > 3] = 0;                             // mask assignment: zero elements > 3
```

## Gotchas

- **It never took off and is effectively frozen.** Implementations vary in
  quality and it lacks the optimizations (expression templates) that make modern
  array libraries fast.
- **Don't reach for it in new code.** For numeric arrays use `vector`/`array` +
  [algorithms](../algorithms/numeric.md), or a real linear-algebra library;
  C++26 brings `mdspan`/`linalg`/SIMD for portable performance.
- **Slice objects are powerful but obscure.** `gslice`/mask semantics are rarely
  worth learning given the alternatives.

## See also

- [std::valarray — cppreference](https://en.cppreference.com/w/cpp/numeric/valarray)
