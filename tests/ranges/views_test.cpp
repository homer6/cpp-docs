// Checks claims made by docs/examples/ranges/views.md.
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

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
