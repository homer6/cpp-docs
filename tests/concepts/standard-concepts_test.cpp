// Checks claims made by docs/examples/concepts/standard-concepts.md.
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {
template <class T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
};

template <Addable T>
T sum(T a, T b) { return a + b; }

struct NoPlus {};
struct PlusReturnsInt { int operator+(PlusReturnsInt) const { return 0; } };

const char* process(std::integral auto) { return "integral"; }
const char* process(std::floating_point auto) { return "floating_point"; }

struct Base {};
struct Derived : Base {};

inline constexpr auto is_even = [](int x) { return x % 2 == 0; };
using IsEven = decltype(is_even);

// "Four ways to apply a concept": each form accepts int and rejects double.
template <std::integral T> constexpr bool f(T) { return true; }
template <class T> requires std::integral<T> constexpr bool g(T) { return true; }
constexpr bool h(std::integral auto) { return true; }
template <class T> concept CallsF = requires(T t) { f(t); };
template <class T> concept CallsG = requires(T t) { g(t); };
template <class T> concept CallsH = requires(T t) { h(t); };
}  // namespace

// "The standard library concepts": core language / relationships.
TEST(StandardConcepts, CoreRelationships) {
    static_assert(std::same_as<int, int>);
    static_assert(std::derived_from<Derived, Base>);
    static_assert(!std::derived_from<Base, Derived>);
    static_assert(std::convertible_to<int, double>);
    static_assert(std::common_with<int, double>);
    static_assert(std::common_reference_with<int&, const int&>);
    static_assert(std::assignable_from<std::string&, const char*>);
    static_assert(std::swappable<std::string>);
    SUCCEED();
}

// "The standard library concepts": type categories.
TEST(StandardConcepts, TypeCategories) {
    static_assert(std::integral<char>);
    static_assert(std::signed_integral<int>);
    static_assert(std::unsigned_integral<unsigned>);
    static_assert(std::floating_point<double>);
    static_assert(!std::integral<double> && !std::floating_point<int>);
    SUCCEED();
}

// "The standard library concepts": comparison.
TEST(StandardConcepts, Comparison) {
    static_assert(std::equality_comparable<std::string>);
    static_assert(std::equality_comparable_with<std::string, const char*>);
    static_assert(std::totally_ordered<int>);
    static_assert(std::totally_ordered_with<int, double>);
    static_assert(std::three_way_comparable<std::string>);
    SUCCEED();
}

// "The standard library concepts": object / lifetime, and what semiregular / regular bundle.
TEST(StandardConcepts, ObjectLifetime) {
    static_assert(std::destructible<std::string>);
    static_assert(std::constructible_from<std::string, std::size_t, char>);
    static_assert(std::default_initializable<std::vector<int>>);
    static_assert(std::move_constructible<std::unique_ptr<int>>);
    static_assert(std::copy_constructible<std::string>);
    static_assert(std::movable<std::unique_ptr<int>>);
    static_assert(!std::copyable<std::unique_ptr<int>>);  // move-only
    static_assert(std::semiregular<std::string>);
    static_assert(std::regular<int>);
    // semiregular = copyable + default_initializable; regular adds equality_comparable.
    static_assert(std::semiregular<int> == (std::copyable<int> && std::default_initializable<int>));
    static_assert(!std::semiregular<std::unique_ptr<int>>);
    static_assert(std::semiregular<Base> && !std::regular<Base>);  // no operator==
    SUCCEED();
}

// "The standard library concepts": callable.
TEST(StandardConcepts, Callable) {
    static_assert(std::invocable<IsEven, int>);
    static_assert(std::regular_invocable<IsEven, int>);
    static_assert(std::predicate<IsEven, int>);
    static_assert(!std::invocable<IsEven, std::string>);
    EXPECT_TRUE(is_even(4));
    EXPECT_FALSE(is_even(3));
}

// "Four ways to apply a concept": constrained parameter, requires-clause, constrained auto, variable.
TEST(StandardConcepts, FourWaysToApply) {
    static_assert(CallsF<int> && !CallsF<double>);
    static_assert(CallsG<int> && !CallsG<double>);
    static_assert(CallsH<int> && !CallsH<double>);
    std::integral auto value = 42;
    static_assert(std::same_as<decltype(value), int>);
    EXPECT_EQ(value, 42);
}

// "The standard library concepts": the relation/strict_weak_order lines hold.
TEST(StandardConcepts, StandardLibraryConcepts) {
    static_assert(std::relation<std::less<>, int, long>);
    static_assert(std::strict_weak_order<std::less<>, int, double>);
    static_assert(std::regular<int> && std::regular<std::string>);
    SUCCEED();
}

// "Writing your own": Addable requires a + b to be valid AND yield T.
TEST(StandardConcepts, WritingYourOwn) {
    static_assert(Addable<int>);
    static_assert(Addable<std::string>);
    static_assert(!Addable<NoPlus>);
    static_assert(!Addable<PlusReturnsInt>);  // valid, but yields int, not T
    static_assert(!Addable<short>);           // short + short is int
    EXPECT_EQ(sum(2, 3), 5);
}

// "Overloading by constraint": ints pick the integral overload, doubles the floating one.
TEST(StandardConcepts, OverloadingByConstraint) {
    EXPECT_STREQ(process(1), "integral");
    EXPECT_STREQ(process(1.0), "floating_point");
}
