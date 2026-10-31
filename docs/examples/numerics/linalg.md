# Basic linear algebra (`<linalg>`)

> A BLAS-style set of free functions for vector/matrix operations, working over
> [`std::mdspan`](../containers/views/mdspan.md) views.
> **Header:** `<linalg>` · **Namespace:** `std::linalg` · **Since:** C++26

## Mental model

The standard doesn't add a matrix *type* — it adds **algorithms** that operate on
data you already own, viewed through `mdspan`. You bring the storage (a `vector`,
an array), wrap it in an `mdspan` of the right shape, and call BLAS-like
functions. Operations come in the three classic BLAS levels:

| Level | Cost | Examples |
|---|---|---|
| **BLAS 1** (vector) | O(n) | `dot`, `add`, `scale`, `vector_two_norm` |
| **BLAS 2** (matrix-vector) | O(n²) | `matrix_vector_product`, triangular solves |
| **BLAS 3** (matrix-matrix) | O(n³) | `matrix_product`, rank-k updates |

## Example

```cpp needs=linalg
#include <linalg>
#include <mdspan>
#include <vector>
#include <execution>

void demo() {
    std::vector<double> data(42);
    std::ranges::iota(data, 0.0);
    std::mdspan x{data.data(), data.size()};    // a length-42 vector view

    std::linalg::scale(2.0, x);                 // x[i] *= 2   (in place)
    std::linalg::scale(std::execution::par_unseq, 3.0, x);   // same, parallel
}
```

### Vector & matrix ops

```cpp needs=linalg
#include <linalg>
#include <mdspan>

using Vec = std::mdspan<double, std::dextents<std::size_t, 1>>;
using Mat = std::mdspan<double, std::dextents<std::size_t, 2>>;

void ops(Vec x, Vec y, Vec z, Vec v, Vec out, Mat A, Mat B, Mat C) {
    double d = std::linalg::dot(x, y);              // dot product
    std::linalg::add(x, y, z);                      // z = x + y (elementwise)
    double n = std::linalg::vector_two_norm(x);     // Euclidean norm

    // matrix-vector and matrix-matrix products take mdspan matrices:
    std::linalg::matrix_vector_product(A, v, out);  // out = A·v
    std::linalg::matrix_product(A, B, C);           // C = A·B
}
```

### Views without copying

`transposed(A)`, `scaled(alpha, A)`, and `conjugated(A)` return *new mdspans* (via
accessor/layout policies) — no data is copied; they re-describe the same storage:

```cpp needs=linalg
#include <linalg>
#include <mdspan>

using Mat = std::mdspan<double, std::dextents<std::size_t, 2>>;

void at_b(Mat A, Mat B, Mat C) {
    auto At = std::linalg::transposed(A);           // view of Aᵀ, shares A's memory
    std::linalg::matrix_product(At, B, C);          // C = Aᵀ·B
}
```

## Gotchas

- **No matrix type — it's algorithms over `mdspan`.** You manage the underlying
  storage (and its layout: `layout_right` row-major vs `layout_left`
  column-major); `linalg` just operates on the views.
- **`scaled`/`transposed`/`conjugated` are lazy views, not copies.** Cheap to
  form; the math happens when an algorithm reads through them.
- **Execution policies are accepted** (`std::execution::par_unseq`, …) on most
  algorithms for parallel/vectorized execution.
- **C++26, early support.** Feature-test with `__cpp_lib_linalg`. Performance
  depends on the implementation's backend; it standardizes the *interface*, not a
  specific optimized kernel.

## See also

- [Basic linear algebra algorithms — cppreference](https://en.cppreference.com/w/cpp/numeric/linalg)
- [std::linalg::matrix_product — cppreference](https://en.cppreference.com/w/cpp/numeric/linalg/matrix_product)
- [std::mdspan — cppreference](https://en.cppreference.com/w/cpp/container/mdspan)
