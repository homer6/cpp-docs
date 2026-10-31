// Checks claims made by docs/examples/language/templates/constraints.md.
#include <concepts>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <list>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "support/capture.hpp"

namespace {
template <class C>
concept Container = requires(C c, const C cc) {
    c.begin();
    typename C::value_type;
    { cc.size() } noexcept -> std::convertible_to<std::size_t>;
    requires sizeof(typename C::value_type) <= 64;
};
struct Big { char bytes[65]; };

template <class T> void describe(const T& x) {
    if constexpr (requires { x.size(); })
        std::cout << "sized: " << x.size() << '\n';
    else
        std::cout << "not sized\n";
}

auto add(std::integral auto a, std::integral auto b) { return a + b; }
template <class T> concept AddAccepts = requires(T t) { add(t, 1); };

template <class T> struct Holder {
    ~Holder() requires std::is_trivially_destructible_v<T> = default;
    ~Holder() {}
};

template <class T> concept Growable = requires(T t) { t.push_back(t.front()); };
template <class T> struct Store {
    void grow() requires Growable<T> {}
};
template <class S> concept HasGrow = requires(S s) { s.grow(); };

template <class T> concept Animal = requires(T t) { t.speak(); };
template <class T> concept Pet = Animal<T> && requires(T t) { t.name(); };
struct Cat { void speak() {} const char* name() { return "cat"; } };
struct Lion { void speak() {} };
const char* who(Animal auto) { return "some animal"; }
const char* who(Pet auto) { return "a pet"; }

template <std::input_iterator I> const char* algo(I) { return "input"; }
template <std::random_access_iterator I> const char* algo(I) { return "random-access"; }

// Wrapping traits in concepts makes the refined overload subsume the base one.
template <class T> concept Integral = std::is_integral_v<T>;
template <class T> concept SignedIntegral = Integral<T> && std::is_signed_v<T>;
template <Integral T> const char* kind(T) { return "integral"; }
template <SignedIntegral T> const char* kind(T) { return "signed integral"; }

template <int N> concept NestedPositive = requires { requires N > 0; };
template <int N> concept SimpleOnlyValid = requires { N > 0; };
}  // namespace

// "requires-expressions": all four requirement kinds; Container<vector<int>> holds, Container<int> doesn't.
TEST(Constraints, RequiresExpressionKinds) {
    static_assert(Container<std::vector<int>>);
    static_assert(!Container<int>);
    static_assert(!Container<std::vector<Big>>);  // the nested requirement is evaluated
}

// "requires-expressions": used directly as a bool in if constexpr.
TEST(Constraints, RequiresExpressionInIfConstexpr) {
    EXPECT_EQ(cppdocs::capture_stdout([] {
                  describe(std::string{"abc"});
                  describe(42);
              }),
              "sized: 3\nnot sized\n");
}

// "Abbreviated function templates": each constrained auto is an independent parameter.
TEST(Constraints, AbbreviatedTemplatesAndConstrainedAuto) {
    static_assert(std::same_as<decltype(add(1, 2L)), long>);
    EXPECT_EQ(add(1, 2L), 3L);
    static_assert(!AddAccepts<double>);
    std::integral auto n = 40 + 2;
    std::convertible_to<long> auto m = 'x';
    auto twice = [](std::floating_point auto x) { return 2 * x; };
    static_assert(std::is_same_v<decltype(n), int>);
    static_assert(std::is_same_v<decltype(m), char>);
    EXPECT_EQ(n, 42);
    EXPECT_EQ(twice(1.5), 3.0);
}

// "Constraining members": a conditionally trivial destructor; a member switched off by requires.
TEST(Constraints, ConstrainedMembers) {
    static_assert(std::is_trivially_destructible_v<Holder<int>>);
    static_assert(!std::is_trivially_destructible_v<Holder<std::string>>);
    static_assert(HasGrow<Store<std::vector<int>>>);
    static_assert(!HasGrow<Store<int>>);
}

// "Partial ordering by constraints": Pet subsumes Animal; random_access_iterator subsumes input_iterator.
TEST(Constraints, SubsumptionPicksMoreConstrained) {
    EXPECT_STREQ(who(Cat{}), "a pet");
    EXPECT_STREQ(who(Lion{}), "some animal");
    std::vector<int> v;
    std::list<int> l;
    EXPECT_STREQ(algo(v.begin()), "random-access");
    EXPECT_STREQ(algo(l.begin()), "input");
}

// "Subsumption only sees through named concepts": with the trait wrapped in a concept, no ambiguity.
TEST(Constraints, NamedConceptsSubsume) {
    EXPECT_STREQ(kind(1), "signed integral");
    EXPECT_STREQ(kind(1u), "integral");
}

// "Gotchas": `requires N > 0;` tests the value; `N > 0;` only tests validity.
TEST(Constraints, NestedVersusSimpleRequirement) {
    static_assert(NestedPositive<1>);
    static_assert(!NestedPositive<0>);
    static_assert(SimpleOnlyValid<0>);  // valid expression, so satisfied even though false
}
