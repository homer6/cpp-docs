// Checks claims made by docs/examples/containers/views/mdspan.md.
#include <cstddef>
#include <numeric>
#include <type_traits>
#include <vector>
#include <version>

#if __has_include(<mdspan>)
#include <mdspan>
#endif

#include "support/feature.hpp"

#include <gtest/gtest.h>

#ifdef __cpp_lib_mdspan
namespace {
template <class M>
concept HasMultiSubscript = requires(M m) { m[0, 0]; };
template <class M>
concept HasCallOperator = requires(M m) { m(0, 0); };
}  // namespace
#endif

// "Declaration": default layout is layout_right, default accessor is default_accessor<T>.
TEST(Mdspan, DeclarationDefaults) {
    CPPDOCS_REQUIRE(__cpp_lib_mdspan);
#ifdef __cpp_lib_mdspan
    using M = std::mdspan<int, std::extents<int, 2, 3>>;
    static_assert(std::is_same_v<M::layout_type, std::layout_right>);
    static_assert(std::is_same_v<M::accessor_type, std::default_accessor<int>>);
#endif
}

// "Extents helpers": static vs dynamic extents and dextents.
TEST(Mdspan, ExtentsHelpers) {
    CPPDOCS_REQUIRE(__cpp_lib_mdspan);
#ifdef __cpp_lib_mdspan
    std::extents<int, 2, 3> e1;
    std::extents<int, 2, std::dynamic_extent> e2{3};
    std::dextents<int, 2> e3{2, 3};

    static_assert(decltype(e1)::rank() == 2 && decltype(e1)::rank_dynamic() == 0);
    static_assert(decltype(e1)::static_extent(0) == 2 && decltype(e1)::static_extent(1) == 3);
    static_assert(decltype(e2)::rank_dynamic() == 1);
    static_assert(decltype(e2)::static_extent(1) == std::dynamic_extent);
    static_assert(decltype(e3)::rank() == 2 && decltype(e3)::rank_dynamic() == 2);
    EXPECT_EQ(e2.extent(0), 2);
    EXPECT_EQ(e2.extent(1), 3);
    EXPECT_EQ(e3.extent(0), 2);
    EXPECT_EQ(e3.extent(1), 3);
#endif
}

// "Mental model" / "View a vector as a matrix": CTAD gives a dynamic 2x3 layout_right
// view; m[1, 2] is flat index 1*3 + 2 = 5.
TEST(Mdspan, ViewVectorAsMatrix) {
    CPPDOCS_REQUIRE(__cpp_lib_mdspan);
#ifdef __cpp_lib_mdspan
    std::vector<int> buf(6);
    std::iota(buf.begin(), buf.end(), 0);

    std::mdspan m{buf.data(), 2, 3};
    static_assert(
        std::is_same_v<decltype(m), std::mdspan<int, std::dextents<std::size_t, 2>>>);
    static_assert(std::is_same_v<decltype(m)::layout_type, std::layout_right>);
    EXPECT_EQ(m.extent(0), 2u);
    EXPECT_EQ(m.extent(1), 3u);
    EXPECT_EQ((m[1, 2]), 5);  // mental-model diagram: value 5

    m[1, 2] = 99;
    EXPECT_EQ(buf[5], 99);

    // Row-major: last index varies fastest.
    for (std::size_t i = 0; i < m.extent(0); ++i)
        for (std::size_t j = 0; j < m.extent(1); ++j)
            EXPECT_EQ((&m[i, j]), buf.data() + i * 3 + j);
#endif
}

// "Compile-time shape": a fixed 3x4 extent; mat[2, 3] is the last element.
TEST(Mdspan, CompileTimeShape) {
    CPPDOCS_REQUIRE(__cpp_lib_mdspan);
#ifdef __cpp_lib_mdspan
    double data[12]{};
    std::mdspan<double, std::extents<std::size_t, 3, 4>> mat{data};
    static_assert(decltype(mat)::rank_dynamic() == 0);
    static_assert(decltype(mat)::static_extent(0) == 3 && decltype(mat)::static_extent(1) == 4);
    mat[2, 3] = 1.0;
    EXPECT_EQ(data[11], 1.0);
#endif
}

// "Column-major view of the same buffer": layout_left — first index varies fastest.
TEST(Mdspan, ColumnMajor) {
    CPPDOCS_REQUIRE(__cpp_lib_mdspan);
#ifdef __cpp_lib_mdspan
    std::vector<int> buf(6);
    std::mdspan<int, std::dextents<int, 2>, std::layout_left> col{buf.data(), 2, 3};
    EXPECT_EQ((&col[1, 0]), buf.data() + 1);
    EXPECT_EQ((&col[0, 1]), buf.data() + 2);
    EXPECT_EQ((&col[1, 2]), buf.data() + 5);  // 1 + 2*2
#endif
}

// "Gotchas": indexing is m[i, j], not m(i, j).
TEST(Mdspan, SubscriptNotCallOperator) {
    CPPDOCS_REQUIRE(__cpp_lib_mdspan);
#ifdef __cpp_lib_mdspan
    using M = std::mdspan<int, std::dextents<int, 2>>;
    static_assert(HasMultiSubscript<M>);
    static_assert(!HasCallOperator<M>);
#endif
}

// "Gotchas" ("Library support"): GCC 15's libstdc++ has no <mdspan>; GCC 16 and
// Clang 22 with libc++ do.
TEST(Mdspan, LibrarySupport) {
#if __has_include(<mdspan>)
    constexpr bool has_header = true;
#else
    constexpr bool has_header = false;
#endif
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 220000
    EXPECT_TRUE(has_header);
#elif defined(__GLIBCXX__) && !defined(__clang__) && __GNUC__ == 15
    EXPECT_FALSE(has_header);
#elif defined(__GLIBCXX__) && !defined(__clang__) && __GNUC__ >= 16
    EXPECT_TRUE(has_header);
#else
    (void)has_header;
    GTEST_SKIP() << "toolchain not covered by the page's support note";
#endif
}
