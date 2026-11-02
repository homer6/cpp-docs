// Checks claims made by docs/examples/ranges/views.md.
#include <array>
#include <concepts>
#include <forward_list>
#include <functional>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace rv = std::views;

namespace {
// Copies any range of values into a vector of its value type.
template <class R>
auto vec(R&& r) {
    std::vector<std::ranges::range_value_t<R>> out;
    for (auto&& x : r) out.push_back(x);
    return out;
}

template <class R>
concept Reversible = requires(R& r) { r | rv::reverse; };
}  // namespace

// "The pipe operator": the page's pipeline yields 20,40,60,80 (only four evens); adaptor(range, args) is equivalent.
TEST(Views, PipeOperator) {
    std::vector<int> data{1, 2, 3, 4, 5, 6, 7, 8};
    auto pred = [](int x) { return x % 2 == 0; };
    auto fn = [](int x) { return x * 10; };
    auto pipeline = data | rv::filter(pred) | rv::transform(fn) | rv::take(5);
    EXPECT_EQ(vec(pipeline), (std::vector<int>{20, 40, 60, 80}));
    auto nested = rv::take(rv::transform(rv::filter(data, pred), fn), 5);
    EXPECT_EQ(vec(nested), vec(pipeline));
}

// "Core adaptors (C++20)": results of r1..r13 on the page's inputs.
TEST(Views, CoreAdaptors) {
    std::vector<int> v{1, 2, 3, 4, 5, 6};
    std::vector<std::vector<int>> nested{{1, 2}, {3}};
    std::map<std::string, int> m{{"a", 1}, {"b", 2}};
    std::string csv = "a,b,c";
    auto pred = [](int x) { return x < 4; };
    auto fn = [](int x) { return x * 10; };
    using V = std::vector<int>;
    EXPECT_EQ(vec(v | rv::filter(pred)), (V{1, 2, 3}));
    EXPECT_EQ(vec(v | rv::transform(fn)), (V{10, 20, 30, 40, 50, 60}));
    EXPECT_EQ(vec(v | rv::take(2)), (V{1, 2}));
    EXPECT_EQ(vec(v | rv::take_while(pred)), (V{1, 2, 3}));
    EXPECT_EQ(vec(v | rv::drop(2)), (V{3, 4, 5, 6}));
    EXPECT_EQ(vec(v | rv::drop_while(pred)), (V{4, 5, 6}));
    EXPECT_EQ(vec(v | rv::reverse), (V{6, 5, 4, 3, 2, 1}));
    EXPECT_EQ(vec(nested | rv::join), (V{1, 2, 3}));
    std::vector<std::string> fields;
    for (auto f : csv | rv::split(',')) fields.emplace_back(f.begin(), f.end());
    EXPECT_EQ(fields, (std::vector<std::string>{"a", "b", "c"}));
    EXPECT_EQ(vec(m | rv::elements<1>), (V{1, 2}));
    EXPECT_EQ(vec(m | rv::keys), (std::vector<std::string>{"a", "b"}));
    EXPECT_EQ(vec(m | rv::values), (V{1, 2}));
    auto r4 = v | rv::take_while(pred);
    static_assert(!std::ranges::common_range<decltype(r4)>);
    static_assert(std::ranges::common_range<decltype(r4 | rv::common)>);
}

// "Core adaptors (C++20)": reverse needs a bidirectional range.
TEST(Views, ReverseNeedsBidirectional) {
    static_assert(Reversible<std::vector<int>>);
    static_assert(!Reversible<std::forward_list<int>>);
    SUCCEED();
}

// "New in C++23": zip stops at the shortest; zip_transform; adjacent<2> sliding tuples.
TEST(Views, ZipFamily) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_zip);
#ifdef __cpp_lib_ranges_zip
    std::vector<int> a{1, 2, 3, 4, 5, 6}, b{10, 20, 30};
    EXPECT_EQ(rv::zip(a, b).size(), 3u);
    std::vector<std::pair<int, int>> zipped;
    for (auto [x, y] : rv::zip(a, b)) zipped.emplace_back(x, y);
    EXPECT_EQ(zipped, (std::vector<std::pair<int, int>>{{1, 10}, {2, 20}, {3, 30}}));
    EXPECT_EQ(vec(rv::zip_transform(std::plus<>{}, a, b)), (std::vector<int>{11, 22, 33}));
    auto adj = a | rv::adjacent<2>;
    EXPECT_EQ(adj.size(), 5u);
    // Compared element by element: libstdc++ 13 yields std::pair<int&, int&> for N == 2 and
    // can't compare it with a std::tuple (P2165 made both tuples in C++23; GCC 14 has it).
    EXPECT_EQ(std::get<0>(*adj.begin()), 1);
    EXPECT_EQ(std::get<1>(*adj.begin()), 2);
#endif
}

// "New in C++23": enumerate yields (index, element).
TEST(Views, Enumerate) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_enumerate);
#ifdef __cpp_lib_ranges_enumerate
    std::vector<std::string> names{"ann", "bob", "cy"};
    std::vector<std::string> seen;
    for (auto [i, name] : names | rv::enumerate) seen.push_back(std::to_string(i) + name);
    EXPECT_EQ(seen, (std::vector<std::string>{"0ann", "1bob", "2cy"}));
#endif
}

// "New in C++23": chunk(2) groups, slide(2) windows.
TEST(Views, ChunkAndSlide) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_chunk);
    CPPDOCS_REQUIRE(__cpp_lib_ranges_slide);
#if defined(__cpp_lib_ranges_chunk) && defined(__cpp_lib_ranges_slide)
    std::vector<int> a{1, 2, 3, 4, 5, 6};
    std::vector<std::vector<int>> chunks, windows;
    for (auto c : a | rv::chunk(2)) chunks.push_back(vec(c));
    for (auto w : a | rv::slide(2)) windows.push_back(vec(w));
    EXPECT_EQ(chunks, (std::vector<std::vector<int>>{{1, 2}, {3, 4}, {5, 6}}));
    EXPECT_EQ(windows.size(), 5u);
    EXPECT_EQ(windows.back(), (std::vector<int>{5, 6}));
#endif
}

// "New in C++23": chunk_by splits where pred(a, b) is false.
TEST(Views, ChunkBy) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_chunk_by);
#ifdef __cpp_lib_ranges_chunk_by
    std::vector<int> a{1, 2, 3, 2, 5, 1};
    std::vector<std::vector<int>> runs;
    for (auto r : a | rv::chunk_by(std::ranges::less{})) runs.push_back(vec(r));
    EXPECT_EQ(runs, (std::vector<std::vector<int>>{{1, 2, 3}, {2, 5}, {1}}));
#endif
}

// "New in C++23": stride(2) takes every 2nd element.
TEST(Views, Stride) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_stride);
#ifdef __cpp_lib_ranges_stride
    std::vector<int> a{1, 2, 3, 4, 5, 6};
    EXPECT_EQ(vec(a | rv::stride(2)), (std::vector<int>{1, 3, 5}));
#endif
}

// "New in C++23": as_const yields const references, as_rvalue yields rvalues.
TEST(Views, AsConstAsRvalue) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_as_const);
    CPPDOCS_REQUIRE(__cpp_lib_ranges_as_rvalue);
#if defined(__cpp_lib_ranges_as_const) && defined(__cpp_lib_ranges_as_rvalue)
    std::vector<int> a{1, 2, 3};
    static_assert(std::same_as<std::ranges::range_reference_t<decltype(a | rv::as_const)>, const int&>);
    static_assert(std::same_as<std::ranges::range_reference_t<decltype(a | rv::as_rvalue)>, int&&>);
    SUCCEED();
#endif
}

// "Examples": sum of squares of the first 3 even numbers in 1..8 is 4 + 16 + 36.
TEST(Views, FilterTransformTake) {
    namespace rv = std::views;
    std::vector<int> data{1, 2, 3, 4, 5, 6, 7, 8};
    int s = 0;
    for (int x : data | rv::filter([](int n) { return n % 2 == 0; })
                      | rv::transform([](int n) { return n * n; })
                      | rv::take(3))
        s += x;
    EXPECT_EQ(s, 56);
}

// "Examples": split a CSV line into its tokens.
TEST(Views, SplitCsvLine) {
    std::string line = "a,b,c";
    std::vector<std::string> fields;
    for (auto field : line | std::views::split(','))
        fields.emplace_back(std::string_view{field.begin(), field.end()});
    EXPECT_EQ(fields, (std::vector<std::string>{"a", "b", "c"}));
}

// "Gotchas": lazy means re-evaluated — iterating twice runs the transform twice.
TEST(Views, LazyIsReevaluated) {
    std::vector<int> data{1, 2, 3};
    int calls = 0;
    auto v = data | std::views::transform([&](int n) { ++calls; return n * 2; });
    EXPECT_EQ(calls, 0);  // nothing runs until iteration
    for (int x : v) (void)x;
    for (int x : v) (void)x;
    EXPECT_EQ(calls, 6);
}

// "Gotchas": filter_view caches its first element's position, so begin() runs the predicate only once.
TEST(Views, FilterCachesBegin) {
    std::vector<int> data{1, 3, 4, 5};
    int calls = 0;
    auto f = data | rv::filter([&](int n) { ++calls; return n % 2 == 0; });
    EXPECT_EQ(*f.begin(), 4);
    EXPECT_EQ(calls, 3);  // 1, 3, 4
    EXPECT_EQ(*f.begin(), 4);
    EXPECT_EQ(calls, 3);  // cached: no re-scan
}

// "join_with — flatten with a separator": a char or a string_view goes between the pieces.
TEST(Views, JoinWith) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_join_with);
#ifdef __cpp_lib_ranges_join_with
    std::vector<std::string> words{"a", "bc", "d"};
    EXPECT_EQ(words | rv::join_with(',') | std::ranges::to<std::string>(), "a,bc,d");
    EXPECT_EQ(words | rv::join_with(std::string_view{", "}) | std::ranges::to<std::string>(), "a, bc, d");
#endif
}

// "cartesian_product — every combination": last range varies fastest; size is the product.
TEST(Views, CartesianProduct) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_cartesian_product);
#ifdef __cpp_lib_ranges_cartesian_product
    std::vector<int> xs{1, 2};
    std::string ys = "ab";
    std::vector<std::pair<int, char>> seen;
    for (auto [x, y] : rv::cartesian_product(xs, ys)) seen.emplace_back(x, y);
    EXPECT_EQ(seen, (std::vector<std::pair<int, char>>{{1, 'a'}, {1, 'b'}, {2, 'a'}, {2, 'b'}}));
    EXPECT_EQ(rv::cartesian_product(xs, ys).size(), 4u);
#endif
}

// "concat — one range after another": mixed range types; random access and sized; common reference.
TEST(Views, Concat) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_concat);
#ifdef __cpp_lib_ranges_concat
    std::vector<int> a{1, 2};
    std::array<int, 2> b{3, 4};
    EXPECT_EQ(vec(rv::concat(a, b, rv::single(5))), (std::vector<int>{1, 2, 3, 4, 5}));
    auto ab = rv::concat(a, b);
    static_assert(std::ranges::random_access_range<decltype(ab)>);
    EXPECT_EQ(ab.size(), 4u);
    // "Gotchas": int + long ranges yield long values, not references.
    std::vector<long> l{7};
    static_assert(std::same_as<std::ranges::range_reference_t<decltype(rv::concat(a, l))>, long>);
    static_assert(std::same_as<std::ranges::range_reference_t<decltype(rv::concat(a, a))>, int&>);
#endif
}

// "cache_latest — compute each element once": 6 transform calls without it, 4 with; input range only.
TEST(Views, CacheLatest) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_cache_latest);
#ifdef __cpp_lib_ranges_cache_latest
    std::vector<int> v{1, 2, 3, 4};
    int calls = 0;
    auto square = [&](int x) { ++calls; return x * x; };
    auto even = [](int x) { return x % 2 == 0; };
    for (int x : v | rv::transform(square) | rv::filter(even)) (void)x;
    EXPECT_EQ(calls, 6);
    calls = 0;
    for (int x : v | rv::transform(square) | rv::cache_latest | rv::filter(even)) (void)x;
    EXPECT_EQ(calls, 4);
    using C = decltype(v | rv::cache_latest);
    static_assert(std::ranges::input_range<C> && !std::ranges::forward_range<C>);
#endif
}

// "as_input — downgrade to an input range": input-only (the P3828 name; GCC 15 has only to_input).
TEST(Views, AsInput) {
    CPPDOCS_REQUIRE(__cpp_lib_ranges_as_input);
#ifdef __cpp_lib_ranges_as_input
    std::vector<int> v{1, 2, 3};
    auto in = v | rv::as_input;
    static_assert(std::ranges::input_range<decltype(in)>);
    static_assert(!std::ranges::forward_range<decltype(in)>);
    static_assert(!std::ranges::common_range<decltype(in)>);
    EXPECT_EQ(vec(in), (std::vector<int>{1, 2, 3}));
#endif
}
