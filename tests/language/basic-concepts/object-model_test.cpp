// Checks claims made by docs/examples/language/basic-concepts/object-model.md.
#include <cstdlib>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
bool holder_destroyed = false;
struct Holder {
    std::vector<int> v;
    ~Holder() { holder_destroyed = true; }
    const std::vector<int>& items() const { return v; }
};
}  // namespace

// "Temporary objects & lifetime extension": binding through member access extends the whole temporary.
TEST(ObjectModel, MemberAccessExtendsTemporary) {
    const int& x = std::pair{1, 2}.first;
    EXPECT_EQ(x, 1);
}

// "Temporary objects & lifetime extension": C++23 range-for keeps every temporary in the range initializer alive.
TEST(ObjectModel, RangeForExtendsTemporaries) {
    CPPDOCS_REQUIRE_VERSION(__cpp_range_based_for, 202211L);
    holder_destroyed = false;
    int sum = 0;
    for (int x : Holder{{1, 2, 3}}.items()) {
        EXPECT_FALSE(holder_destroyed);
        sum += x;
    }
    EXPECT_TRUE(holder_destroyed);
    EXPECT_EQ(sum, 6);
}

// "Implicit-lifetime types": malloc implicitly creates an implicit-lifetime object.
TEST(ObjectModel, MallocCreatesImplicitLifetimeObject) {
    struct Point { int x, y; };
    auto* p = static_cast<Point*>(std::malloc(sizeof(Point)));
    ASSERT_NE(p, nullptr);
    p->x = 1;
    p->y = 2;
    EXPECT_EQ(p->x + p->y, 3);
    std::free(p);
}
