// Checks claims made by docs/examples/containers/views/mdspan.md.
#include <cstddef>
#include <memory>
#include <numeric>
#include <type_traits>
#include <utility>
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

// "Slices: std::submdspan (C++26)": each slice kind, rank changes, result
// layouts, and aliasing.
TEST(Mdspan, Submdspan) {
    CPPDOCS_REQUIRE(__cpp_lib_submdspan);
#ifdef __cpp_lib_submdspan
    int buf[12];
    std::iota(buf, buf + 12, 0);
    std::mdspan m{buf, 3, 4};

    auto row1 = std::submdspan(m, 1, std::full_extent);
    static_assert(decltype(row1)::rank() == 1);
    static_assert(std::is_same_v<decltype(row1)::layout_type, std::layout_right>);
    ASSERT_EQ(row1.extent(0), 4u);
    for (std::size_t j = 0; j < 4; ++j) EXPECT_EQ(row1[j], 4 + static_cast<int>(j));

    auto col2 = std::submdspan(m, std::full_extent, 2);
    static_assert(std::is_same_v<decltype(col2)::layout_type, std::layout_stride>);
    ASSERT_EQ(col2.extent(0), 3u);
    EXPECT_EQ(col2[0], 2);
    EXPECT_EQ(col2[1], 6);
    EXPECT_EQ(col2[2], 10);
    EXPECT_EQ(col2.stride(0), 4u);

    auto blk = std::submdspan(m, std::pair{1, 3}, std::pair{1, 3});
    static_assert(decltype(blk)::rank() == 2);
    EXPECT_EQ(blk.extent(0), 2u);
    EXPECT_EQ(blk.extent(1), 2u);
    EXPECT_EQ((blk[0, 0]), 5);
    EXPECT_EQ((blk[0, 1]), 6);
    EXPECT_EQ((blk[1, 0]), 9);
    EXPECT_EQ((blk[1, 1]), 10);

    auto evens = std::submdspan(m, 0, std::extent_slice{.offset = 0, .extent = 2, .stride = 2});
    ASSERT_EQ(evens.extent(0), 2u);  // extent = number of elements selected
    EXPECT_EQ(evens[0], 0);
    EXPECT_EQ(evens[1], 2);

    auto odds = std::submdspan(m, 0, std::range_slice{.first = 1, .last = 4, .stride = 2});
    ASSERT_EQ(odds.extent(0), 2u);
    EXPECT_EQ(odds[0], 1);
    EXPECT_EQ(odds[1], 3);

    blk[0, 0] = 99;
    EXPECT_EQ(buf[5], 99);  // aliases the buffer
#endif
}

// "Promise alignment: std::aligned_accessor (C++26)": access through an aligned
// mdspan, offset_policy is default_accessor, is_sufficiently_aligned checks the
// pointer, and the aligned -> default conversion is implicit but not the reverse.
TEST(Mdspan, AlignedAccessor) {
    CPPDOCS_REQUIRE(__cpp_lib_aligned_accessor);
#ifdef __cpp_lib_aligned_accessor
    using Aligned = std::aligned_accessor<float, 32>;
    static_assert(Aligned::byte_alignment == 32);
    static_assert(std::is_same_v<Aligned::offset_policy, std::default_accessor<float>>);

    alignas(32) float buf[16]{};
    std::mdspan<float, std::dims<1>> v{buf, 16};
    EXPECT_TRUE(std::is_sufficiently_aligned<32>(v.data_handle()));
    EXPECT_FALSE(std::is_sufficiently_aligned<32>(buf + 1));

    std::mdspan<float, std::dims<1>, std::layout_right, Aligned> a{
        v.data_handle(), v.mapping(), Aligned{}};
    buf[3] = 1.5f;
    for (std::size_t i = 0; i < a.extent(0); ++i) a[i] *= 2;
    EXPECT_EQ(buf[3], 3.0f);

    using Plain = std::mdspan<float, std::dims<1>>;
    using AlignedM = decltype(a);
    static_assert(std::is_convertible_v<AlignedM, Plain>);
    static_assert(!std::is_convertible_v<Plain, AlignedM>);
    Plain back = a;
    EXPECT_EQ(back[3], 3.0f);
#endif
}

// "Gotchas" ("strided_slice is gone"): the 202603L submdspan has extent_slice
// and range_slice instead.
TEST(Mdspan, StridedSliceRenamed) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_submdspan, 202603L);
#if __cpp_lib_submdspan >= 202603L
    std::extent_slice<int, int, int> e{.offset = 1, .extent = 4, .stride = 3};
    std::range_slice<int, int, int> r{.first = 1, .last = 11, .stride = 3};
    int buf[11];
    std::iota(buf, buf + 11, 0);
    std::mdspan v{buf, 11};
    auto se = std::submdspan(v, e);
    auto sr = std::submdspan(v, r);
    ASSERT_EQ(se.extent(0), 4u);  // indices 1, 4, 7, 10
    ASSERT_EQ(sr.extent(0), 4u);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(se[i], 1 + 3 * static_cast<int>(i));
        EXPECT_EQ(sr[i], se[i]);
    }
#endif
}
