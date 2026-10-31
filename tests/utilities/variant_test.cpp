// Checks claims made by docs/examples/utilities/variant.md.
#include <cstddef>
#include <stdexcept>
#include <string>
#include <variant>

#include <gtest/gtest.h>

namespace {
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// Not trivially copyable, and its constructor throws: emplacing it can leave a variant valueless.
struct Throws {
    std::string s;
    explicit Throws(int) { throw std::runtime_error("boom"); }
    Throws(const Throws&) = default;
};
}  // namespace

// "Mental model": variant<int, std::string, double> v = "hi" has index() == 1.
TEST(Variant, MentalModelIndex) {
    std::variant<int, std::string, double> v = "hi";
    EXPECT_EQ(v.index(), 1u);
    EXPECT_EQ(std::get<std::string>(v), "hi");
}

// "Assign, query, access": index 0 then 1; holds_alternative, get by type / index, get_if.
TEST(Variant, AssignQueryAccess) {
    std::variant<int, std::string> v = 42;
    EXPECT_EQ(v.index(), 0u);
    v = "hello";
    EXPECT_EQ(v.index(), 1u);
    EXPECT_TRUE(std::holds_alternative<std::string>(v));
    EXPECT_EQ(std::get<std::string>(v), "hello");
    EXPECT_EQ(std::get<1>(v), "hello");
    EXPECT_EQ(std::get_if<int>(&v), nullptr);
    ASSERT_NE(std::get_if<std::string>(&v), nullptr);
    EXPECT_EQ(*std::get_if<std::string>(&v), "hello");
}

// "Gotchas": get with the wrong alternative throws std::bad_variant_access.
TEST(Variant, WrongGetThrows) {
    std::variant<int, std::string> v = "hello";
    EXPECT_THROW((void)std::get<int>(v), std::bad_variant_access);
    EXPECT_THROW((void)std::get<0>(v), std::bad_variant_access);
}

// "Visiting": a generic lambda and an overloaded set both dispatch on the active type.
// (A visitor missing an alternative is a hard compile error, which a run-time test can't show.)
TEST(Variant, Visit) {
    std::variant<int, double, std::string> v = 3.14;
    auto name = overloaded{
        [](int) { return std::string{"int"}; },
        [](double) { return std::string{"double"}; },
        [](const std::string&) { return std::string{"string"}; },
    };
    EXPECT_EQ(std::visit(name, v), "double");
    v = std::string{"s"};
    EXPECT_EQ(std::visit(name, v), "string");
    v = 1;
    EXPECT_EQ(std::visit(name, v), "int");

    std::size_t seen = 0;
    std::visit([&](const auto& x) { seen = sizeof x; }, v);
    EXPECT_EQ(seen, sizeof(int));
}

// "Gotchas": a throw while changing alternatives can leave the variant valueless_by_exception().
TEST(Variant, ValuelessByException) {
    std::variant<int, Throws> v = 7;
    EXPECT_FALSE(v.valueless_by_exception());
    EXPECT_THROW(v.emplace<Throws>(0), std::runtime_error);
    EXPECT_TRUE(v.valueless_by_exception());
    EXPECT_EQ(v.index(), std::variant_npos);
    EXPECT_THROW((void)std::get<int>(v), std::bad_variant_access);
}

// "Gotchas": duplicate types need index access (get<int> on variant<int, int> is ill-formed).
TEST(Variant, DuplicateTypesByIndex) {
    std::variant<int, int> v{std::in_place_index<1>, 5};
    EXPECT_EQ(v.index(), 1u);
    EXPECT_EQ(std::get<1>(v), 5);
    EXPECT_THROW((void)std::get<0>(v), std::bad_variant_access);
}
