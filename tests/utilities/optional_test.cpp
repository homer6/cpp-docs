// Checks claims made by docs/examples/utilities/optional.md.
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
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

// "Mental model": optional<int> a = 42 is engaged; b = std::nullopt (or `return {};`) is empty.
TEST(Optional, MentalModelEngagedOrEmpty) {
    std::optional<int> a = 42;
    std::optional<int> b = std::nullopt;
    auto braces = []() -> std::optional<int> { return {}; };
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(*a, 42);
    EXPECT_FALSE(b.has_value());
    EXPECT_FALSE(braces().has_value());
    EXPECT_EQ(b, std::nullopt);
}

namespace {
struct Counted {
    static inline int copies = 0, moves = 0;
    Counted(int, char) {}
    Counted(const Counted&) { ++copies; }
    Counted(Counted&&) noexcept { ++moves; }
};
}  // namespace

// "In-place construction": emplace builds the value in place, no temporary is copied or moved.
TEST(Optional, EmplaceMakesNoTemporary) {
    Counted::copies = Counted::moves = 0;
    std::optional<Counted> c;
    c.emplace(5, 'x');
    std::optional<Counted> d{std::in_place, 5, 'x'};
    EXPECT_EQ(Counted::copies, 0);
    EXPECT_EQ(Counted::moves, 0);
}

// "Monadic operations (C++23)": transform maps to optional<V>; or_else runs only when empty.
TEST(Optional, MonadicResultTypesAndLaziness) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_optional, 202110L);
#if defined(__cpp_lib_optional) && __cpp_lib_optional >= 202110L
    std::optional<int> n = 3;
    auto s = n.transform([](int x) { return std::to_string(x); });
    static_assert(std::is_same_v<decltype(s), std::optional<std::string>>);
    EXPECT_EQ(s, "3");
    auto half = [](int x) { return x % 2 == 0 ? std::optional<double>{x / 2.0} : std::nullopt; };
    static_assert(std::is_same_v<decltype(n.and_then(half)), std::optional<double>>);

    int calls = 0;
    auto fallback = [&] { ++calls; return std::optional<int>{0}; };
    EXPECT_EQ(n.or_else(fallback), 3);
    EXPECT_EQ(calls, 0);  // engaged: or_else not invoked
    EXPECT_EQ(std::optional<int>{}.or_else(fallback), 0);
    EXPECT_EQ(calls, 1);
#endif
}

// "Gotchas": an engaged optional<T*> holding nullptr is truthy too.
TEST(Optional, OptionalPointerTestsEngagement) {
    std::optional<int*> p{nullptr};
    EXPECT_TRUE(p.has_value());
    EXPECT_EQ(*p, nullptr);
}
