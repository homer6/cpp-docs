# Data-parallel types (SIMD)

> Portable, explicit vectorization: a value type that holds several elements and
> operates on all of them at once, mapping to SIMD registers/instructions.
> **Header:** `<simd>` · **Namespace:** `std::simd` · **Since:** C++26

## Mental model

Where a `T` holds one value, `std::simd::vec<T>` holds **W** values (the *width*,
chosen by the implementation for the target) and every operation acts
**element-wise and unsequenced** — the compiler turns that into SIMD
instructions. Masks (`std::simd::mask<T>`) carry per-element booleans for
conditional work.

```
 vec<int>  a = {1, 1, 1, 1}
 vec<int>  b = {-2,-1, 0, 1}
 a + b        = {-1, 0, 1, 2}        ← one instruction, four lanes
```

## Core types

| Type | Role |
|---|---|
| `std::simd::basic_vec<T, Abi>` / `vec<T>` / `vec<T, W>` | data-parallel vector of `T` |
| `std::simd::basic_mask<...>` / `mask<T>` | per-element booleans |

The `Abi` tag controls width/representation per target; `vec<T>` uses the native
width, `vec<T, W>` requests a specific width.

## Examples

```cpp
#include <simd>
namespace simd = std::simd;

simd::vec<int> a = 1;                                   // broadcast: {1,1,1,1,...}
simd::vec<int> b([](int i){ return i - 2; });            // generator: {-2,-1,0,1,...}
auto c = a + b;                                          // element-wise add

// branch per lane with select + mask:
auto abs = simd::select(c < 0, -c, c);                   // where c<0 use -c, else c

// horizontal reductions (the marked exceptions to element-wise):
int total = simd::reduce(abs);                           // sum across lanes
bool any  = simd::any_of(c < 0);

// math functions from <cmath> are overloaded for vec:
simd::vec<double, 16> x([](int i){ return i; });
auto one = std::pow(std::cos(x), 2) + std::pow(std::sin(x), 2);   // ≈ 1 per lane
```

Loads/stores move between contiguous memory and `vec` (`unchecked_load`,
`partial_store`, …); operations are `constexpr` (except non-constexpr math).

## Gotchas

- **Width is implementation-defined.** `vec<T>` (native width) is the portable
  default; don't hardcode a lane count — query `.size()`. Use `vec<T, W>` only
  when you truly need a fixed width.
- **Element operations are *unsequenced*.** No data dependencies between lanes, no
  per-lane side effects with ordering assumptions.
- **Branch with `select`/masks, not `if`.** You can't `if` on individual lanes;
  compute both sides and `select`, or use masked loads/stores.
- **Brand-new (C++26), early support.** Feature-test with `__cpp_lib_simd`. It
  descends from `std::experimental::simd` (the Parallelism TS) — old code used the
  `experimental` namespace.

## See also

- [Data-parallel types (SIMD) — cppreference](https://en.cppreference.com/w/cpp/numeric/simd)
- [std::simd::vec — cppreference](https://en.cppreference.com/w/cpp/numeric/simd/vec)
- [std::simd::select — cppreference](https://en.cppreference.com/w/cpp/numeric/simd/select)
