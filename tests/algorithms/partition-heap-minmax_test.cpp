// Checks claims made by docs/examples/algorithms/partition-heap-minmax.md.
#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {
auto is_even = [](int x) { return x % 2 == 0; };
}  // namespace

// "Partitioning": [begin, mid) satisfy pred; is_partitioned and partition_point agree.
TEST(PartitionHeapMinmax, PartitionSplitsByPredicate) {
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7};
    auto mid = std::partition(v.begin(), v.end(), is_even);
    EXPECT_EQ(mid - v.begin(), 3);
    EXPECT_TRUE(std::all_of(v.begin(), mid, is_even));
    EXPECT_TRUE(std::none_of(mid, v.end(), is_even));
    EXPECT_TRUE(std::is_partitioned(v.begin(), v.end(), is_even));
    EXPECT_EQ(std::partition_point(v.begin(), v.end(), is_even), mid);
}

// "Partitioning": stable_partition keeps relative order within each group.
TEST(PartitionHeapMinmax, StablePartitionKeepsOrder) {
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7};
    std::stable_partition(v.begin(), v.end(), is_even);
    EXPECT_EQ(v, (std::vector<int>{2, 4, 6, 1, 3, 5, 7}));
}

// "Heap operations": make_heap puts the max at the front in O(n) comparisons.
TEST(PartitionHeapMinmax, MakeHeapMaxAtFrontLinear) {
    std::vector<int> v{3, 1, 4, 1, 5};
    std::make_heap(v.begin(), v.end());
    EXPECT_EQ(v.front(), 5);
    EXPECT_TRUE(std::is_heap(v.begin(), v.end()));

    // The standard bounds make_heap at 3n comparisons.
    std::vector<int> big(10000);
    for (int i = 0; i < 10000; ++i) big[i] = (i * 7919) % 10007;
    long calls = 0;
    std::make_heap(big.begin(), big.end(), [&](int a, int b) { ++calls; return a < b; });
    EXPECT_LE(calls, 3L * 10000);
}

// "Heap operations": push_back + push_heap sifts up; pop_heap moves the max to the back.
TEST(PartitionHeapMinmax, PushPopSortHeap) {
    std::vector<int> v{3, 1, 4, 1, 5};
    std::make_heap(v.begin(), v.end());
    v.push_back(9);
    std::push_heap(v.begin(), v.end());
    EXPECT_EQ(v.front(), 9);
    std::pop_heap(v.begin(), v.end());
    EXPECT_EQ(v.back(), 9);
    v.pop_back();
    EXPECT_TRUE(std::is_heap(v.begin(), v.end()));
    std::sort_heap(v.begin(), v.end());
    EXPECT_EQ(v, (std::vector<int>{1, 1, 3, 4, 5}));
}

// "Heap operations": make_heap then repeated pop_heap + pop_back extracts in descending order.
TEST(PartitionHeapMinmax, ExtractDescending) {
    std::vector<int> v{3, 1, 4, 1, 5};
    std::make_heap(v.begin(), v.end());
    std::vector<int> out;
    while (!v.empty()) {
        std::pop_heap(v.begin(), v.end());
        out.push_back(v.back());
        v.pop_back();
    }
    EXPECT_EQ(out, (std::vector<int>{5, 4, 3, 1, 1}));
}

// "Min / max / clamp": values on the page's inputs.
TEST(PartitionHeapMinmax, MinMaxClamp) {
    int a = 3, b = 7, x = 12;
    std::vector<int> v{3, 1, 4, 1, 5};
    EXPECT_EQ(std::min(a, b), 3);
    EXPECT_EQ(std::max(a, b), 7);
    EXPECT_EQ(std::min({3, 1, 4, 1, 5}), 1);
    auto [lo, hi] = std::minmax(a, b);
    EXPECT_EQ(lo, 3);
    EXPECT_EQ(hi, 7);
    EXPECT_EQ(*std::min_element(v.begin(), v.end()), 1);
    EXPECT_EQ(std::min_element(v.begin(), v.end()) - v.begin(), 1);  // first of the equal minima
    EXPECT_EQ(*std::max_element(v.begin(), v.end()), 5);
    auto [mn, mx] = std::minmax_element(v.begin(), v.end());
    EXPECT_EQ(*mn, 1);
    EXPECT_EQ(*mx, 5);
    EXPECT_EQ(std::clamp(x, lo, hi), 7);
    EXPECT_EQ(std::clamp(5, lo, hi), 5);
    EXPECT_EQ(std::clamp(0, lo, hi), 3);
}

// "Gotchas": min/max return const&, minmax returns references to its arguments.
TEST(PartitionHeapMinmax, ReturnReferences) {
    int a = 3, b = 7;
    static_assert(std::is_same_v<decltype(std::max(a, b)), const int&>);
    static_assert(std::is_same_v<decltype(std::minmax(a, b)), std::pair<const int&, const int&>>);
    static_assert(std::is_same_v<decltype(std::min({1, 2})), int>);  // initializer_list form is by value
    EXPECT_EQ(&std::max(a, b), &b);
    auto mm = std::minmax(a, b);
    EXPECT_EQ(&mm.first, &a);
    EXPECT_EQ(&mm.second, &b);
}
