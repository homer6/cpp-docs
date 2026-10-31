// Checks claims made by docs/examples/metaprogramming/type-traits.md.
#include <cstdint>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

namespace {
template <class T> requires std::is_integral_v<T>
T half(T x) { return x / 2; }

template <class T>
auto describe(T) {
    if constexpr (std::is_floating_point_v<T>) return "float";
    else if constexpr (std::is_integral_v<T>) return "int";
    else return "other";
}

template <class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
T legacy_half(T x) { return x / 2; }

template <class T> concept CanHalf = requires(T t) { half(t); };
template <class T> concept CanLegacyHalf = requires(T t) { legacy_half(t); };
}  // namespace

// "Mental model": value traits have ::value/_v, type traits ::type/_t.
TEST(TypeTraits, ValueAndTypeHelpers) {
    static_assert(std::is_integral<int>::value);
    static_assert(std::is_integral_v<int>);
    static_assert(std::is_same_v<std::remove_const<const int>::type, int>);
    static_assert(std::is_same_v<std::remove_const_t<const int>, int>);
}

// "The main families": a sample from each family.
TEST(TypeTraits, Families) {
    enum E : std::uint8_t { a };
    struct Base { virtual ~Base() = default; };
    struct Derived : Base {};
    struct Empty {};
    static_assert(std::is_enum_v<E> && std::is_class_v<Base> && std::is_function_v<void(int)>);
    static_assert(std::is_arithmetic_v<double> && std::is_fundamental_v<void>);
    static_assert(std::is_empty_v<Empty> && std::is_polymorphic_v<Base>);
    static_assert(std::has_virtual_destructor_v<Derived>);
    static_assert(std::is_base_of_v<Base, Derived> && std::is_convertible_v<Derived*, Base*>);
    static_assert(std::is_invocable_r_v<int, int (*)(char), char>);
    static_assert(std::is_same_v<std::conditional_t<true, int, long>, int>);
    static_assert(std::is_same_v<std::common_type_t<int, long, short>, long>);
    static_assert(std::is_same_v<std::underlying_type_t<E>, std::uint8_t>);
    static_assert(std::is_same_v<std::add_pointer_t<int&>, int*>);
}

// "Constraining": half/legacy_half accept integers only; describe branches at compile time.
TEST(TypeTraits, ConstrainingAndIfConstexpr) {
    EXPECT_EQ(half(7), 3);
    EXPECT_EQ(legacy_half(9L), 4L);
    static_assert(!CanHalf<double> && !CanLegacyHalf<double>);
    EXPECT_STREQ(describe(1.0), "float");
    EXPECT_STREQ(describe(1), "int");
    EXPECT_STREQ(describe(std::string{}), "other");
}

// "Normalizing a deduced type" and "Gotchas: decay_t vs remove_cvref_t".
TEST(TypeTraits, DecayVersusRemoveCvref) {
    static_assert(std::is_same_v<std::remove_cvref_t<const int&>, int>);
    static_assert(std::is_same_v<std::remove_cvref_t<int (&)[3]>, int[3]>);
    static_assert(std::is_same_v<std::decay_t<int (&)[3]>, int*>);
    static_assert(std::is_same_v<std::decay_t<void(int)>, void (*)(int)>);
    static_assert(std::is_same_v<std::decay_t<const volatile int&&>, int>);
}

// "static_assert for clear contracts": the trait store() checks.
TEST(TypeTraits, TriviallyCopyable) {
    struct Pod { int x; };
    static_assert(std::is_trivially_copyable_v<Pod>);
    static_assert(!std::is_trivially_copyable_v<std::string>);
}
