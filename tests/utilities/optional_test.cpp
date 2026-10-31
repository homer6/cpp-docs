// Checks claims made by docs/examples/utilities/optional.md.
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
std::optional<int> find_even(std::span<const int> xs) {
    for (int x : xs)
        if (x % 2 == 0) return x;
    return std::nullopt;
}
}  // namespace

// "Create, check, access": value() throws bad_optional_access when empty; value_or falls back.
TEST(Optional, CreateCheckAccess) {
    const int some[]{1, 3, 4, 5};
    const int none[]{1, 3, 5};
    auto r = find_even(some);
    ASSERT_TRUE(r);
    EXPECT_EQ(*r, 4);
    EXPECT_EQ(r.value(), 4);

    auto e = find_even(none);
    EXPECT_FALSE(e);
    EXPECT_THROW((void)e.value(), std::bad_optional_access);
    EXPECT_EQ(e.value_or(-1), -1);
}

// "In-place construction": emplace(5, 'x') builds "xxxxx"; reset() empties.
TEST(Optional, InPlaceConstruction) {
    std::optional<std::string> s;
    s.emplace(5, 'x');
    EXPECT_EQ(*s, "xxxxx");
    std::optional<std::string> t{std::in_place, "hello"};
    EXPECT_EQ(*t, "hello");
    s.reset();
    EXPECT_FALSE(s.has_value());
}

// "Monadic operations (C++23)": and_then / transform / or_else chain.
TEST(Optional, MonadicOperations) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_optional, 202110L);
#if defined(__cpp_lib_optional) && __cpp_lib_optional >= 202110L
    auto validate = [](int x) { return x >= 0 ? std::optional{x} : std::nullopt; };
    auto square = [](int x) { return x * x; };
    auto default_value = [] { return std::optional<int>{0}; };
    auto run = [&](std::optional<int> n) {
        return n.and_then(validate).transform(square).or_else(default_value);
    };
    EXPECT_EQ(run(3), 9);
    EXPECT_EQ(run(-3), 0);            // validate fails -> or_else
    EXPECT_EQ(run(std::nullopt), 0);  // empty from the start
#endif
}

// "Gotchas": an engaged optional<bool>{false} is truthy as an optional.
TEST(Optional, OptionalBoolTestsEngagement) {
    std::optional<bool> b{false};
    EXPECT_TRUE(static_cast<bool>(b));
    EXPECT_FALSE(*b);
}

// "Gotchas": optional<T&> is standard since C++26.
TEST(Optional, OptionalReferenceCpp26) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_optional, 202506L);
#if defined(__cpp_lib_optional) && __cpp_lib_optional >= 202506L
    int x = 1;
    std::optional<int&> r{x};
    *r = 2;
    EXPECT_EQ(x, 2);
#endif
}
