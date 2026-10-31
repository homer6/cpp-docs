// Checks claims made by docs/examples/algorithms/execution-and-ranges.md.
#include <algorithm>
#include <concepts>
#include <functional>
#include <list>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <version>
#if __has_include(<execution>)
#include <execution>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Execution policies (C++17)": seq, par, par_unseq and (C++20) unseq all sort / for_each correctly.
TEST(ExecutionAndRanges, PoliciesGiveTheSameResult) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_execution, 201902L);
#if defined(__cpp_lib_execution) && __cpp_lib_execution >= 201902L
    static_assert(std::is_execution_policy_v<std::execution::sequenced_policy>);
    static_assert(std::is_execution_policy_v<std::execution::parallel_policy>);
    static_assert(std::is_execution_policy_v<std::execution::parallel_unsequenced_policy>);
    static_assert(std::is_execution_policy_v<std::execution::unsequenced_policy>);

    const std::vector<int> sorted{1, 2, 3, 4, 5};
    for (int i = 0; i < 3; ++i) {
        std::vector<int> v{5, 3, 1, 4, 2};
        if (i == 0) std::sort(std::execution::seq, v.begin(), v.end());
        if (i == 1) std::sort(std::execution::par, v.begin(), v.end());
        if (i == 2) std::sort(std::execution::par_unseq, v.begin(), v.end());
        EXPECT_EQ(v, sorted);
    }
    std::vector<int> v{1, 2, 3};
    std::for_each(std::execution::unseq, v.begin(), v.end(), [](int& x) { x *= 2; });
    EXPECT_EQ(v, (std::vector<int>{2, 4, 6}));
#endif
}

// "Rules": an exception escaping an element op calls std::terminate under any policy, even seq.
TEST(ExecutionAndRangesDeathTest, ThrowFromElementOpTerminates) {
    CPPDOCS_REQUIRE(__cpp_lib_execution);
#ifdef __cpp_lib_execution
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto run = [](const auto& policy) {
        std::vector<int> v(3);
        std::for_each(policy, v.begin(), v.end(), [](int) { throw std::runtime_error("boom"); });
    };
    EXPECT_DEATH(run(std::execution::par), "");
    EXPECT_DEATH(run(std::execution::seq), "");
#endif
}

namespace {
struct Person {
    std::string name;
    int age;
    int score;
};
}  // namespace

// "std::ranges algorithms (C++20)": whole-range calls, projections, results on sample data.
TEST(ExecutionAndRanges, RangesAlgorithmsAndProjections) {
    std::vector<int> v{3, -1, 42, 2};
    std::vector<Person> people{{"Cy", 30, 1}, {"Al", 12, 2}, {"Bo", 18, 3}};

    std::ranges::sort(v);
    EXPECT_EQ(v, (std::vector<int>{-1, 2, 3, 42}));
    std::ranges::sort(people, {}, &Person::age);  // default comparator, projected field
    EXPECT_EQ(people[0].name, "Al");
    EXPECT_EQ(people[1].name, "Bo");
    EXPECT_EQ(people[2].name, "Cy");

    EXPECT_TRUE(std::ranges::any_of(v, [](int x) { return x > 0; }));
    EXPECT_EQ(std::ranges::find(v, 42) - v.begin(), 3);
    std::ranges::for_each(v, [](int& x) { x *= 2; });
    EXPECT_EQ(v, (std::vector<int>{-2, 4, 6, 84}));
    EXPECT_EQ(std::ranges::count_if(people, [](int age) { return age >= 18; }, &Person::age), 2);
}

// "std::ranges algorithms": richer result types, and concept-checked (constrained) signatures.
TEST(ExecutionAndRanges, RicherResultsAndConstraints) {
    std::vector<int> in{1, 2, 3}, out(3);
    auto r = std::ranges::copy(in, out.begin());
    static_assert(std::same_as<decltype(r), std::ranges::in_out_result<std::vector<int>::iterator,
                                                                       std::vector<int>::iterator>>);
    EXPECT_EQ(r.in, in.end());   // classic std::copy returns only the output iterator
    EXPECT_EQ(r.out, out.end());

    // A list isn't random-access, so ranges::sort is simply not callable (no deep template error).
    static_assert(!std::invocable<decltype(std::ranges::sort), std::list<int>&>);
    static_assert(std::invocable<decltype(std::ranges::sort), std::vector<int>&>);
}

// "Projections": max_element with &Person::score finds Ann (90).
TEST(ExecutionAndRanges, ProjectionMaxElement) {
    struct P { std::string name; int score; };
    std::vector<P> people{{"Ann", 90}, {"Bob", 75}};
    auto best = std::ranges::max_element(people, std::less<>{}, &P::score);
    EXPECT_EQ(best->name, "Ann");
}

// "Gotchas": find with a projection still returns an iterator to the element, not the projected value.
TEST(ExecutionAndRanges, ProjectionAppliesToPredicateNotResult) {
    std::vector<Person> people{{"Ann", 40, 90}, {"Bob", 17, 75}};
    auto it = std::ranges::find(people, 17, &Person::age);
    static_assert(std::same_as<decltype(it), std::vector<Person>::iterator>);
    EXPECT_EQ(it->name, "Bob");
}

namespace {
template <class T>
concept Dereferenceable = requires(T t) { *t; };
}  // namespace

// "Borrowed iterators & dangling": a temporary vector gives std::ranges::dangling; an lvalue an iterator.
TEST(ExecutionAndRanges, DanglingForTemporaries) {
    auto make_vector = [] { return std::vector<int>{1, 2, 3}; };
    auto it = std::ranges::find(make_vector(), 1);
    static_assert(std::same_as<decltype(it), std::ranges::dangling>);
    static_assert(!Dereferenceable<decltype(it)>);  // can't dereference

    auto v = make_vector();
    static_assert(std::same_as<decltype(std::ranges::find(v, 1)), std::vector<int>::iterator>);
}

// "Gotchas": ranges algorithms take no execution policy (until C++26 parallel range algorithms).
TEST(ExecutionAndRanges, RangesAlgorithmsTakeNoPolicy) {
    CPPDOCS_REQUIRE(__cpp_lib_execution);
#ifdef __cpp_lib_execution
#ifdef __cpp_lib_parallel_range_algorithms
    GTEST_SKIP() << "this toolchain has C++26 parallel range algorithms (__cpp_lib_parallel_range_algorithms)";
#else
    static_assert(!std::invocable<decltype(std::ranges::sort), const std::execution::parallel_policy&,
                                  std::vector<int>&>);
#endif
#endif
}
