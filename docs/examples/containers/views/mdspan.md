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

```cpp needs=mdspan
#include <mdspan>

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

```cpp needs=mdspan
#include <mdspan>

int main() {
    std::extents<int, 2, 3>                 e1;        // both dims fixed at compile time
    std::extents<int, 2, std::dynamic_extent> e2{3};   // 2 × (runtime 3)
    std::dextents<int, 2>                   e3{2, 3};  // all dims dynamic (rank 2)
}
```

## Examples

### View a vector as a matrix

```cpp needs=mdspan
#include <numeric>
#include <mdspan>
#include <vector>

int main() {
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
}
```

### Compile-time shape

```cpp needs=mdspan
#include <mdspan>

int main() {
    double data[12];
    std::mdspan<double, std::extents<std::size_t, 3, 4>> mat{data};   // fixed 3×4
    mat[2, 3] = 1.0;
}
```

### Column-major view of the same buffer

```cpp needs=mdspan
#include <mdspan>
#include <vector>

int main() {
    std::vector<int> buf(6);
    std::mdspan<int, std::dextents<int, 2>, std::layout_left> col{buf.data(), 2, 3};
    // now col[i, j] maps with the first index varying fastest
}
```

### Slices: `std::submdspan` (C++26)

`submdspan(m, slices...)` returns an `mdspan` over part of `m`, with one slice
per dimension. Nothing is copied: the result aliases the same buffer.

| Slice | Selects | Rank |
|---|---|---|
| an index `i` | just `i` | drops the dimension |
| `std::full_extent` | the whole dimension | keeps it |
| `std::pair{first, last}` (any 2-element type) | `[first, last)` | keeps it |
| `std::extent_slice{.offset, .extent, .stride}` | `extent` indices from `offset`, every `stride` | keeps it |
| `std::range_slice{.first, .last, .stride}` | `[first, last)`, every `stride` | keeps it |

```cpp needs=submdspan
#include <mdspan>
#include <numeric>
#include <utility>

int main() {
    int buf[12];
    std::iota(buf, buf + 12, 0);
    std::mdspan m{buf, 3, 4};                                    // 3×4, row-major

    auto row1 = std::submdspan(m, 1, std::full_extent);          // 4 5 6 7 (rank 1)
    auto col2 = std::submdspan(m, std::full_extent, 2);          // 2 6 10, stride 4
    auto blk  = std::submdspan(m, std::pair{1, 3}, std::pair{1, 3});   // [[5 6] [9 10]]
    auto evens = std::submdspan(m, 0, std::extent_slice{.offset = 0, .extent = 2, .stride = 2}); // 0 2
    auto odds  = std::submdspan(m, 0, std::range_slice{.first = 1, .last = 4, .stride = 2});     // 1 3
    blk[0, 0] = 99;                                              // writes buf[5]
}
```

The result's layout is the cheapest one that still fits: `row1` keeps
`layout_right`, while `col2` needs `layout_stride`. GCC 16 has it
(`__cpp_lib_submdspan` `202603L`); Clang 22 (libc++) and GCC 15 don't.

### Promise alignment: `std::aligned_accessor` (C++26)

`aligned_accessor<T, N>` is an accessor policy that promises every data handle
is aligned to `N` bytes; each access goes through `std::assume_aligned<N>`, so
the compiler may emit aligned vector loads. Check the pointer first with
`std::is_sufficiently_aligned<N>(p)` (`<memory>`, C++26): handing it a
misaligned pointer is undefined behaviour.

```cpp needs=mdspan
#include <mdspan>
#include <memory>

using Aligned = std::aligned_accessor<float, 32>;

void scale(std::mdspan<float, std::dims<1>> v) {
    if (std::is_sufficiently_aligned<32>(v.data_handle())) {
        std::mdspan<float, std::dims<1>, std::layout_right, Aligned> a{
            v.data_handle(), v.mapping(), Aligned{}};       // explicit: you vouch for it
        for (std::size_t i = 0; i < a.extent(0); ++i) a[i] *= 2;
    } else {
        for (std::size_t i = 0; i < v.extent(0); ++i) v[i] *= 2;
    }
}
```

Going the other way, an aligned `mdspan` converts implicitly to one with
`default_accessor`. Clang 22 (libc++) and GCC 16 have it
(`__cpp_lib_aligned_accessor` `202411L`).

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
- **`strided_slice` is gone.** Earlier C++26 drafts had
  `strided_slice{offset, extent, stride}`, whose `extent` was the length of the
  input span. P3982 replaced it with `extent_slice`, whose `extent` is the
  **number of elements selected**, and added `range_slice`. GCC 16 has only the
  new names. Code written against older papers breaks.
- **`aligned_accessor` isn't implicit.** A plain `mdspan` doesn't convert to an
  aligned one, because the compiler can't check the promise. And
  `aligned_accessor<double, 4>` doesn't compile: `N` must be a power of two
  and at least `alignof(T)`.
- **Library support.** GCC 15.2's libstdc++ has no `<mdspan>`; GCC 16 and
  Clang 22 with libc++ do. The examples are tagged `needs=mdspan` so the
  GCC 15 snippet check skips them; they compile and run with GCC 16.2 and
  Clang 22 (libc++).

## See also

- [std::mdspan — cppreference](https://en.cppreference.com/w/cpp/container/mdspan)
- [std::extents — cppreference](https://en.cppreference.com/w/cpp/container/mdspan/extents)
- [Layout policies (layout_right / left / stride) — cppreference](https://en.cppreference.com/w/cpp/container/mdspan/layout_right)
- [[mdspan.sub] submdspan](https://eel.is/c++draft/mdspan.sub) and
  [[mdspan.accessor.aligned] aligned_accessor](https://eel.is/c++draft/mdspan.accessor.aligned)
  in the draft (cppreference has no pages for them yet)
- [P2630R4: Submdspan](https://wg21.link/P2630R4) and
  [P3982R2: Split strided_slice into extent_slice and range_slice](https://wg21.link/P3982R2)
- [P2897R7: aligned_accessor](https://wg21.link/P2897R7)
