// Checks claims made by docs/examples/language/declarations-initialization/attributes.md.
#include <gtest/gtest.h>

namespace {
int count_calls() {
    int count = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wassume"
    [[assume(++count > 0)]];
#pragma GCC diagnostic pop
    return count;
}
}  // namespace

// "[[assume(expr)]]": the expression is never evaluated.
TEST(Attributes, AssumeIsNotEvaluated) { EXPECT_EQ(count_calls(), 0); }
