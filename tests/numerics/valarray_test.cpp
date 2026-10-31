// Checks claims made by docs/examples/numerics/valarray.md.
#include <cmath>
#include <valarray>
#include <vector>

#include <gtest/gtest.h>

namespace {
template <class T>
std::vector<T> vec(const std::valarray<T>& v) { return {std::begin(v), std::end(v)}; }
}  // namespace

// "Mental model": element-wise arithmetic, sqrt, sum and max.
TEST(Valarray, ElementWise) {
    std::valarray<double> a{1, 2, 3, 4};
    std::valarray<double> b{10, 20, 30, 40};
    std::valarray<double> c = a + b;
    std::valarray<double> d = a * 2.0;
    std::valarray<double> e = std::sqrt(a);
    EXPECT_EQ(vec(c), (std::vector<double>{11, 22, 33, 44}));
    EXPECT_EQ(vec(d), (std::vector<double>{2, 4, 6, 8}));
    EXPECT_DOUBLE_EQ(e[3], 2.0);
    EXPECT_EQ(a.sum(), 10);
    EXPECT_EQ(a.max(), 4);
}

// "Slicing": slice(0, 4, 2) picks {0,2,4,6}; mask assignment zeroes elements > 3.
TEST(Valarray, SliceAndMask) {
    std::valarray<int> v{0, 1, 2, 3, 4, 5, 6, 7};
    std::valarray<int> evens = v[std::slice(0, 4, 2)];
    EXPECT_EQ(vec(evens), (std::vector<int>{0, 2, 4, 6}));
    v[v > 3] = 0;
    EXPECT_EQ(vec(v), (std::vector<int>{0, 1, 2, 3, 0, 0, 0, 0}));
}
