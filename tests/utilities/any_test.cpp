// Checks claims made by docs/examples/utilities/any.md.
#include <any>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>

#include <gtest/gtest.h>

// "Examples": reassigning changes the held type; any_cast by exact type; pointer form gives nullptr; reset empties.
TEST(Any, Basics) {
    std::any a = 42;
    EXPECT_EQ(a.type(), typeid(int));
    a = std::string{"hello"};
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(a.type(), typeid(std::string));
    EXPECT_EQ(std::any_cast<std::string>(a), "hello");
    EXPECT_THROW((void)std::any_cast<int>(a), std::bad_any_cast);
    EXPECT_EQ(std::any_cast<int>(&a), nullptr);
    a.reset();
    EXPECT_FALSE(a.has_value());
    EXPECT_EQ(a.type(), typeid(void));  // an empty any reports typeid(void)
}

// "A heterogeneous bag": values of different types in one map, retrieved by their stored type.
TEST(Any, HeterogeneousBag) {
    std::map<std::string, std::any> properties;
    properties["count"] = 10;
    properties["name"] = std::string{"widget"};
    properties["scale"] = 1.5;
    EXPECT_EQ(std::any_cast<int>(properties["count"]), 10);
    EXPECT_EQ(std::any_cast<std::string>(properties["name"]), "widget");
    EXPECT_EQ(std::any_cast<double>(properties["scale"]), 1.5);
}

// "Gotchas": no implicit conversions — any_cast<long> on an int throws; the pointer form returns nullptr.
TEST(Any, ExactTypeRequired) {
    std::any a = 42;
    EXPECT_THROW((void)std::any_cast<long>(a), std::bad_any_cast);
    EXPECT_EQ(std::any_cast<long>(&a), nullptr);
    ASSERT_NE(std::any_cast<int>(&a), nullptr);
    EXPECT_EQ(*std::any_cast<int>(&a), 42);
}

// "Gotchas": the stored type must be copy-constructible.
TEST(Any, CopyableTypesOnly) {
    static_assert(std::is_constructible_v<std::any, std::string>);
    static_assert(!std::is_constructible_v<std::any, std::unique_ptr<int>>);
}
