// Checks claims made by docs/examples/numerics/simd.md.
//
// No local toolchain defines __cpp_lib_simd: GCC 16.2 ships <simd> but only
// enables it on x86 with SSE2 (the gcc:16 image here is aarch64) and doesn't
// advertise the macro yet; GCC 15 and libc++ have no <simd>.
#include <cmath>
#include <version>

#ifdef __cpp_lib_simd
#include <simd>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Examples": broadcast, generator, element-wise add, select, reduce, any_of.
TEST(Simd, ElementWiseSelectReduce) {
    CPPDOCS_REQUIRE(__cpp_lib_simd);
#ifdef __cpp_lib_simd
    namespace simd = std::simd;
    simd::vec<int> a = 1;
    simd::vec<int> b([](int i) { return i - 2; });
    auto c = a + b;
    for (int i = 0; i < c.size(); ++i) EXPECT_EQ(c[i], i - 1);
    auto abs = simd::select(c < 0, -c, c);
    int expect = 0;
    for (int i = 0; i < abs.size(); ++i) expect += i == 0 ? 1 : i - 1;
    EXPECT_EQ(simd::reduce(abs), expect);
    EXPECT_TRUE(simd::any_of(c < 0));
#endif
}

// "Examples": <cmath> functions are overloaded for vec — cos² + sin² ≈ 1 per lane;
// vec<T, W> requests a fixed width.
TEST(Simd, MathOverloadsAndFixedWidth) {
    CPPDOCS_REQUIRE(__cpp_lib_simd);
#ifdef __cpp_lib_simd
    namespace simd = std::simd;
    simd::vec<double, 16> x([](int i) { return i; });
    static_assert(x.size() == 16);
    auto one = std::pow(std::cos(x), 2) + std::pow(std::sin(x), 2);
    for (int i = 0; i < 16; ++i) EXPECT_NEAR(one[i], 1.0, 1e-12);
#endif
}
