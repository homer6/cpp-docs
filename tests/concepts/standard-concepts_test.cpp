// Checks claims made by docs/examples/concepts/standard-concepts.md.
#include <concepts>
#include <functional>
#include <string>

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
}  // namespace

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
