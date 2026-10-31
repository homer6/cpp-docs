// Checks claims made by docs/examples/language/classes/unions-aggregates.md.
#include <bit>
#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>

#include <gtest/gtest.h>

namespace {

struct Point { int x; int y; };
struct Config { int w = 80; bool wrap = false; };

union Value {
    int    i;
    double d;
    char   bytes[8];
};

union WithString {  // a non-trivial member: the union can't destroy it for you
    std::string s;
    int i;
};

struct UserCtor { UserCtor() = default; int x; };  // C++20: any user-declared ctor
class PrivateData {
    int x = 0;
public:
    int get() const { return x; }
};

}  // namespace

// "Aggregates": brace, designated and array initialization, and structured bindings.
TEST(UnionsAggregates, AggregateInitialization) {
    static_assert(std::is_aggregate_v<Point>);
    static_assert(std::is_aggregate_v<Config>);
    static_assert(std::is_aggregate_v<int[3]>);

    Point p{1, 2};
    Config c{.w = 100, .wrap = true};
    Config d{};
    int arr[3]{1, 2, 3};
    auto [x, y] = p;
    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, 2);
    EXPECT_EQ(c.w, 100);
    EXPECT_TRUE(c.wrap);
    EXPECT_EQ(d.w, 80);  // default member initializers fill omitted members
    EXPECT_FALSE(d.wrap);
    EXPECT_EQ(arr[2], 3);
}

// "Gotchas": a user-declared constructor (even = default) or private data ends aggregate-ness.
TEST(UnionsAggregates, NonAggregates) {
    static_assert(!std::is_aggregate_v<UserCtor>);
    static_assert(!std::is_aggregate_v<PrivateData>);
}

// "Unions": all members share the same storage; writing one switches the active member.
TEST(UnionsAggregates, UnionMembersShareStorage) {
    Value v;
    static_assert(sizeof(Value) == 8);
    EXPECT_EQ(static_cast<void*>(&v.i), static_cast<void*>(&v.d));
    EXPECT_EQ(static_cast<void*>(&v.i), static_cast<void*>(v.bytes));
    v.i = 42;
    EXPECT_EQ(v.i, 42);
    v.d = 3.14;
    EXPECT_DOUBLE_EQ(v.d, 3.14);
}

// "Gotchas": a union with a std::string has a deleted destructor; bit_cast replaces punning.
TEST(UnionsAggregates, NonTrivialMembersAndBitCast) {
    static_assert(!std::is_destructible_v<WithString>);
    static_assert(!std::is_default_constructible_v<WithString>);
    EXPECT_EQ(std::bit_cast<std::uint32_t>(1.0f), 0x3F800000u);
}

// "Prefer std::variant": it tracks the active type and std::get throws on a mismatch.
TEST(UnionsAggregates, VariantIsTypeSafe) {
    std::variant<int, double, std::string> v = 3.14;
    EXPECT_TRUE(std::holds_alternative<double>(v));
    EXPECT_EQ(v.index(), 1u);
    EXPECT_DOUBLE_EQ(std::get<double>(v), 3.14);
    EXPECT_THROW((void)std::get<int>(v), std::bad_variant_access);
}
