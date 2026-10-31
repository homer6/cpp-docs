// Checks claims made by docs/examples/numerics/linalg.md.
//
// No local toolchain ships <linalg> (GCC 15/16, Clang 22 + libc++), so these
// skip until one defines __cpp_lib_linalg.
#include <cstddef>
#include <vector>
#include <version>

#ifdef __cpp_lib_linalg
#include <linalg>
#include <mdspan>
#include <numeric>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Example": scale multiplies in place.
TEST(Linalg, ScaleInPlace) {
    CPPDOCS_REQUIRE(__cpp_lib_linalg);
#ifdef __cpp_lib_linalg
    std::vector<double> data(42);
    std::iota(data.begin(), data.end(), 0.0);
    std::mdspan x{data.data(), data.size()};
    std::linalg::scale(2.0, x);
    EXPECT_EQ(data[41], 82.0);
#endif
}

// "Vector & matrix ops": dot, add, norm, matrix-vector and matrix-matrix products.
TEST(Linalg, VectorAndMatrixOps) {
    CPPDOCS_REQUIRE(__cpp_lib_linalg);
#ifdef __cpp_lib_linalg
    std::vector<double> xs{3, 4}, ys{1, 2}, zs(2), out(2);
    std::mdspan x{xs.data(), 2}, y{ys.data(), 2}, z{zs.data(), 2}, o{out.data(), 2};
    EXPECT_EQ(std::linalg::dot(x, y), 11.0);
    std::linalg::add(x, y, z);
    EXPECT_EQ(zs, (std::vector<double>{4, 6}));
    EXPECT_DOUBLE_EQ(std::linalg::vector_two_norm(x), 5.0);
    std::vector<double> a{1, 2, 3, 4}, c(4);  // A = [[1,2],[3,4]]
    std::mdspan A{a.data(), 2, 2}, C{c.data(), 2, 2};
    std::linalg::matrix_vector_product(A, y, o);
    EXPECT_EQ(out, (std::vector<double>{5, 11}));
    std::linalg::matrix_product(A, A, C);
    EXPECT_EQ(c, (std::vector<double>{7, 10, 15, 22}));
#endif
}

// "Views without copying": transposed(A) shares A's memory.
TEST(Linalg, TransposedIsAView) {
    CPPDOCS_REQUIRE(__cpp_lib_linalg);
#ifdef __cpp_lib_linalg
    std::vector<double> a{1, 2, 3, 4};
    std::mdspan A{a.data(), 2, 2};
    auto At = std::linalg::transposed(A);
    EXPECT_EQ((At[0, 1]), 3.0);
    a[2] = 30;
    EXPECT_EQ((At[0, 1]), 30.0);  // no copy
#endif
}
