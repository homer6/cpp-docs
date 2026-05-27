# std::mdspan

> A **non-owning, multidimensional view** over a contiguous block of memory — like
> [`span`](span.md), but for matrices/tensors, with pluggable layout and indexing.
> **Header:** `<mdspan>` · **Since:** C++23

> Not a container: it owns no memory. It reinterprets a flat 1-D buffer as an
> N-dimensional array.

## Mental model

You have a flat buffer of `rows*cols` numbers. `mdspan` lets you address it as a
2-D (or N-D) array `m[i, j]` without copying or reshaping — it just maps
multidimensional indices to a flat offset using a **layout policy**.

```
 flat buffer:  [ 0  1  2  3  4  5 ]   (6 ints)

 mdspan as 2×3 (row-major / layout_right):
        col0 col1 col2
 row0 [  0    1    2 ]      m[1, 2]  →  flat index 1*3 + 2 = 5  →  value 5
 row1 [  3    4    5 ]
```

## Declaration

```cpp
template<
    class T,
    class Extents,
    class LayoutPolicy   = std::layout_right,
    class AccessorPolicy = std::default_accessor<T>
> class mdspan;
```

- `Extents` — a `std::extents<IndexType, ...>` describing the shape. Each extent
  is either a compile-time constant or `std::dynamic_extent` (runtime).
- `LayoutPolicy` — how (i, j, …) maps to a flat offset:
  - `std::layout_right` (default) — **row-major** (C/C++ style, last index varies fastest).
  - `std::layout_left` — **column-major** (Fortran style).
  - `std::layout_stride` — arbitrary per-dimension strides.
- `AccessorPolicy` — how an offset becomes a reference (customizable for e.g.
  atomic or restricted access).

### Extents helpers

```cpp
std::extents<int, 2, 3>                 e1;        // both dims fixed at compile time
std::extents<int, 2, std::dynamic_extent> e2{3};   // 2 × (runtime 3)
std::dextents<int, 2>                   e3{2, 3};  // all dims dynamic (rank 2)
```

## Examples

### View a vector as a matrix

```cpp
#include <mdspan>
#include <vector>

std::vector<int> buf(6);
std::iota(buf.begin(), buf.end(), 0);          // 0,1,2,3,4,5

std::mdspan m{buf.data(), 2, 3};               // CTAD → 2×3, layout_right
// Multidimensional subscript (C++23):
m[1, 2] = 99;                                  // writes buf[5]
auto rows = m.extent(0);                       // 2
auto cols = m.extent(1);                        // 3

for (std::size_t i = 0; i < m.extent(0); ++i)
    for (std::size_t j = 0; j < m.extent(1); ++j)
        /* use m[i, j] */;
```

### Compile-time shape

```cpp
double data[12];
std::mdspan<double, std::extents<std::size_t, 3, 4>> mat{data};   // fixed 3×4
mat[2, 3] = 1.0;
```

### Column-major view of the same buffer

```cpp
std::mdspan<int, std::dextents<int, 2>, std::layout_left> col{buf.data(), 2, 3};
// now col[i, j] maps with the first index varying fastest
```

## Gotchas

- **Indexing is `m[i, j]`, not `m(i, j)` or `m[i][j]`.** It uses C++23's
  **multidimensional subscript operator** (multiple arguments inside one `[]`).
  Pre-standard implementations used `operator()` — final C++23 is `[]`.
- **Non-owning.** The backing buffer must outlive the `mdspan` and be large enough
  for the declared extents — there's no bounds checking by default.
- **Layout matters for performance.** Iterate in the order the layout stores
  elements (innermost index matching the fastest-varying dimension) for cache
  efficiency; `layout_right` ⇒ last index innermost.
- **`mdspan` is the view; `std::mdarray` (a separate, later proposal) owns.** If
  you need ownership, manage the buffer yourself (e.g. a `vector`) and view it.

## See also

- [std::mdspan — cppreference](https://en.cppreference.com/w/cpp/container/mdspan)
- [std::extents — cppreference](https://en.cppreference.com/w/cpp/container/mdspan/extents)
- [Layout policies (layout_right / left / stride) — cppreference](https://en.cppreference.com/w/cpp/container/mdspan/layout_right)
