// Checks claims made by docs/examples/numerics/valarray.md.
#include <cmath>
#include <type_traits>
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

// "Shift, rotate, apply": the values in the comments.
TEST(Valarray, ShiftRotateApply) {
    std::valarray<int> v{0, 1, 2, 3, 4, 5, 6, 7};
    EXPECT_EQ(vec<int>(v.shift(2)), (std::vector<int>{2, 3, 4, 5, 6, 7, 0, 0}));
    EXPECT_EQ(vec<int>(v.cshift(2)), (std::vector<int>{2, 3, 4, 5, 6, 7, 0, 1}));
    std::valarray<int> sq = v.apply([](int x) { return x * x; });
    EXPECT_EQ(sq[7], 49);
}

// "Declaration": comparisons are element-wise and return valarray<bool>.
TEST(Valarray, ComparisonsReturnMask) {
    std::valarray<int> a{1, 5}, b{2, 2};
    std::valarray<bool> lt = a < b;
    EXPECT_TRUE(lt[0]);
    EXPECT_FALSE(lt[1]);
}

// "Gotchas": auto may not give a valarray — libstdc++ and libc++ return a proxy for a + b.
TEST(Valarray, AutoGivesProxy) {
    std::valarray<double> a{1, 2}, b{3, 4};
    auto c = a + b;
    static_assert(!std::is_same_v<decltype(c), std::valarray<double>>);
    std::valarray<double> d = a + b;
    EXPECT_EQ(d[1], 6.0);
}
