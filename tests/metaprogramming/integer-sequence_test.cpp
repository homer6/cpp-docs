// Checks claims made by docs/examples/metaprogramming/integer-sequence.md.
#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace {
template <class F, class Tuple, std::size_t... Is>
auto apply_impl(F&& f, Tuple&& t, std::index_sequence<Is...>) {
    return std::forward<F>(f)(std::get<Is>(std::forward<Tuple>(t))...);
}

template <class F, class Tuple>
auto my_apply(F&& f, Tuple&& t) {
    constexpr auto n = std::tuple_size_v<std::remove_cvref_t<Tuple>>;
    return apply_impl(std::forward<F>(f), std::forward<Tuple>(t), std::make_index_sequence<n>{});
}

template <std::size_t... Is>
constexpr auto squares_impl(std::index_sequence<Is...>) {
    return std::array{(Is * Is)...};
}
}  // namespace

// "Mental model": make_index_sequence<3> is index_sequence<0, 1, 2>.
TEST(IntegerSequence, MakeIndexSequenceGeneratesZeroToNMinusOne) {
    static_assert(std::is_same_v<std::make_index_sequence<3>, std::index_sequence<0, 1, 2>>);
    static_assert(std::make_index_sequence<5>::size() == 5);
}

// "Gotchas": index_sequence is integer_sequence specialized for std::size_t.
TEST(IntegerSequence, IndexSequenceIsSizeTIntegerSequence) {
    static_assert(std::is_same_v<std::index_sequence<0, 1>, std::integer_sequence<std::size_t, 0, 1>>);
    static_assert(std::is_same_v<std::make_integer_sequence<int, 3>, std::integer_sequence<int, 0, 1, 2>>);
}

// "The canonical pattern": my_apply sums the tuple to 6, the same as std::apply.
TEST(IntegerSequence, MyApplyMatchesStdApply) {
    auto sum = [](int a, int b, int c) { return a + b + c; };
    EXPECT_EQ(my_apply(sum, std::tuple{1, 2, 3}), 6);
    EXPECT_EQ(std::apply(sum, std::tuple{1, 2, 3}), 6);
}

// "Building an array from a generator": squares is {0, 1, 4, 9, 16}.
TEST(IntegerSequence, SquaresArray) {
    constexpr auto squares = squares_impl(std::make_index_sequence<5>{});
    static_assert(squares == std::array<std::size_t, 5>{0, 1, 4, 9, 16});
    EXPECT_EQ(squares.back(), 16u);
}
