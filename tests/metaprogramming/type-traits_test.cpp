// Checks claims made by docs/examples/metaprogramming/type-traits.md.
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <version>

#include "support/feature.hpp"

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

struct NoDefault { NoDefault(int); int size() const; };

template <class T, class = void> struct has_size : std::false_type {};
template <class T>
struct has_size<T, std::void_t<decltype(std::declval<const T&>().size())>> : std::true_type {};
template <class T> concept HasSize = requires(const T& t) { t.size(); };

template <class T>
T clamp_to(T v, std::type_identity_t<T> lo, std::type_identity_t<T> hi) {
    return v < lo ? lo : hi < v ? hi : v;
}
template <class T> T clamp_bad(T v, T lo, T hi) { return v < lo ? lo : hi < v ? hi : v; }
template <class... A> concept CanClampBad = requires(A... a) { clamp_bad(a...); };

template <class T> struct is_big : std::bool_constant<(sizeof(T) > 4)> {};
template <class... Ts> constexpr bool all_ints = std::conjunction_v<std::is_integral<Ts>...>;

struct LA { int x; char c; };
struct LB { int y; char d; };
struct LC { int z; double e; };
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

// "std::declval": names a T&& in unevaluated contexts, even without a default constructor.
TEST(TypeTraits, Declval) {
    static_assert(!std::is_default_constructible_v<NoDefault>);
    static_assert(std::is_same_v<decltype(std::declval<NoDefault>().size()), int>);
    static_assert(std::is_same_v<decltype(std::declval<int>()), int&&>);
    static_assert(std::is_same_v<decltype(std::declval<int&>()), int&>);
}

// "std::void_t and the detection idiom": has_size and the HasSize concept agree.
TEST(TypeTraits, VoidTDetectionVersusRequires) {
    static_assert(std::is_same_v<std::void_t<int, double>, void>);
    static_assert(has_size<std::vector<int>>::value && !has_size<int>::value);
    static_assert(HasSize<std::string> && !HasSize<int>);
    static_assert(has_size<std::string>::value == HasSize<std::string>);
}

// "std::type_identity": the type_identity_t parameters are not deduced, so they convert.
TEST(TypeTraits, TypeIdentityBlocksDeduction) {
    static_assert(std::is_same_v<std::type_identity_t<int>, int>);
    auto r = clamp_to(1.5, 0, 1);
    static_assert(std::is_same_v<decltype(r), double>);
    EXPECT_EQ(r, 1.0);
    static_assert(!CanClampBad<double, int, int>);  // T deduced as both double and int
    static_assert(CanClampBad<double, double, double>);
}

// "conjunction, disjunction, negation": short-circuit; later traits are not instantiated.
TEST(TypeTraits, LogicalTraitsShortCircuit) {
    static_assert(!std::conjunction_v<std::is_class<int>, is_big<void>>);
    static_assert(std::disjunction_v<std::is_integral<int>, is_big<void>>);
    static_assert(std::negation_v<std::is_pointer<int>>);
    static_assert(all_ints<int, long> && !all_ints<int, double>);
    // conjunction derives from the first false trait.
    static_assert(std::is_base_of_v<std::is_class<int>,
                                    std::conjunction<std::true_type, std::is_class<int>, is_big<void>>>);
}

// "is_layout_compatible and is_corresponding_member (C++20)": GCC has them, libc++ doesn't yet.
TEST(TypeTraits, LayoutCompatibleAndCorrespondingMember) {
    CPPDOCS_REQUIRE(__cpp_lib_is_layout_compatible);
#ifdef __cpp_lib_is_layout_compatible
    static_assert(std::is_layout_compatible_v<LA, LB>);
    static_assert(!std::is_layout_compatible_v<LA, LC>);
    static_assert(std::is_corresponding_member(&LA::c, &LB::d));
    static_assert(std::is_corresponding_member(&LA::x, &LC::z));
    static_assert(!std::is_corresponding_member(&LA::c, &LC::e));
#endif
}
