// Checks claims made by docs/examples/metaprogramming/ratio.md.
#include <chrono>
#include <cstdint>
#include <ratio>
#include <type_traits>

#include <gtest/gtest.h>

// "Mental model": num/den are static constexpr members, reduced to lowest terms.
TEST(Ratio, MembersAndReduction) {
    using half = std::ratio<1, 2>;
    static_assert(half::num == 1 && half::den == 2);
    using r = std::ratio<4, 8>;
    static_assert(r::num == 1 && r::den == 2);
    static_assert(std::ratio<3, -6>::num == -1 && std::ratio<3, -6>::den == 2);  // sign on num
}

// "Arithmetic on types": the values in the comments.
TEST(Ratio, ArithmeticAndComparison) {
    using sum = std::ratio_add<std::ratio<1, 2>, std::ratio<1, 3>>;
    using diff = std::ratio_subtract<std::ratio<1, 2>, std::ratio<1, 3>>;
    using prod = std::ratio_multiply<std::ratio<2, 3>, std::ratio<3, 4>>;
    using quot = std::ratio_divide<std::ratio<1, 2>, std::ratio<1, 4>>;
    static_assert(sum::num == 5 && sum::den == 6);
    static_assert(diff::num == 1 && diff::den == 6);
    static_assert(prod::num == 1 && prod::den == 2);
    static_assert(quot::num == 2 && quot::den == 1);
    static_assert(std::ratio_equal_v<std::ratio<1, 2>, std::ratio<2, 4>>);
    static_assert(std::ratio_less_v<std::ratio<1, 3>, std::ratio<1, 2>>);
}

// "SI prefixes": the predefined prefixes and their use in chrono.
TEST(Ratio, SiPrefixes) {
    static_assert(std::is_same_v<std::milli, std::ratio<1, 1000>>);
    static_assert(std::is_same_v<std::micro, std::ratio<1, 1'000'000>>);
    static_assert(std::is_same_v<std::kilo, std::ratio<1000, 1>>);
    static_assert(std::mega::num == 1'000'000 && std::giga::num == 1'000'000'000);
    static_assert(std::nano::den == 1'000'000'000 && std::pico::den == 1'000'000'000'000);
    static_assert(std::is_same_v<std::chrono::milliseconds::period, std::milli>);
    using ms = std::chrono::duration<long long, std::milli>;
    EXPECT_EQ(std::chrono::duration_cast<ms>(std::chrono::seconds{2}).count(), 2000);
}

// "Gotchas": it's type-level — num/den are compile-time intmax_t constants.
TEST(Ratio, TypeLevelIntmax) {
    static_assert(std::is_same_v<decltype(std::ratio<1, 2>::num), const std::intmax_t>);
    static_assert(std::is_same_v<std::ratio<2, 4>::type, std::ratio<1, 2>>);
}
