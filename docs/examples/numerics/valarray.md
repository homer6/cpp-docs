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

## Declaration

```cpp
template<class T> class valarray;
```

| Piece | Meaning |
|---|---|
| `+ - * / % & \| ^ << >>`, unary ops | Element-wise, between two valarrays or a valarray and a `T` |
| `== < …` | Element-wise too — they return `valarray<bool>` |
| `sum()`, `min()`, `max()` | Reductions |
| `shift(n)`, `cshift(n)` | Shift elements left by `n` (fill with `T()`), or rotate |
| `apply(f)` | New array with `f` applied to each element |
| `abs`, `exp`, `sqrt`, `pow`, `sin`, … | Element-wise math functions |
| `v[std::slice(start, size, stride)]` | A strided view (`slice_array`) |
| `v[std::gslice(…)]`, `v[mask]`, `v[indices]` | Multi-dimensional strides, `valarray<bool>` mask, `valarray<size_t>` index list |
| `std::begin(v)`, `std::end(v)` | Iterators (C++11), so range-for works |

## When to use it

- **Fits:** short programs and exercises that want `a + b`-style math on 1-D
  numeric arrays without a dependency.
- **Anti-fits:** anything performance-critical or multi-dimensional, and new
  library code — use `vector` + [algorithms](../algorithms/numeric.md), a
  linear-algebra library, or C++26 `mdspan`/`linalg`/SIMD.

## Examples

### Slicing

`valarray` supports strided/indexed access via `std::slice`, `std::gslice`,
masks, and indirect arrays:

```cpp
#include <valarray>

void demo() {
    std::valarray<int> v{0,1,2,3,4,5,6,7};
    std::valarray<int> evens = v[std::slice(0, 4, 2)];  // start 0, count 4, stride 2 → {0,2,4,6}
    v[v > 3] = 0;                            // mask assignment: zero elements > 3
}
```

### Shift, rotate, apply

```cpp
#include <valarray>

void shifts() {
    std::valarray<int> v{0, 1, 2, 3, 4, 5, 6, 7};
    std::valarray<int> s = v.shift(2);                         // {2,3,4,5,6,7,0,0}
    std::valarray<int> r = v.cshift(2);                        // {2,3,4,5,6,7,0,1}
    std::valarray<int> sq = v.apply([](int x) { return x * x; });  // {0,1,4,9,…,49}
}
```

## Gotchas

- **It never took off and is effectively frozen.** Implementations vary in
  quality and it lacks the optimizations (expression templates) that make modern
  array libraries fast.
- **`auto` may not give you a `valarray`.** The standard lets operators (and
  `apply`) return a proxy expression type instead; libstdc++ and libc++ both do
  for `a + b`. The proxy can refer to its operands, so name the type —
  `std::valarray<double> c = a + b;` — whenever the result outlives them.
- **Mismatched sizes are undefined behaviour.** `a + b` with different lengths
  isn't checked.
- **Don't reach for it in new code.** For numeric arrays use `vector`/`array` +
  [algorithms](../algorithms/numeric.md), or a real linear-algebra library;
  C++26 brings `mdspan`/`linalg`/SIMD for portable performance.
- **Slice objects are powerful but obscure.** `gslice`/mask semantics are rarely
  worth learning given the alternatives.

## See also

- [std::valarray — cppreference](https://en.cppreference.com/w/cpp/numeric/valarray)
- [std::slice — cppreference](https://en.cppreference.com/w/cpp/numeric/valarray/slice)
- [Numerics](README.md)
