// Checks claims made by docs/examples/metaprogramming/reflection.md.
// GCC 16 builds this file with -freflection (see tests/CMakeLists.txt).
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
#include <meta>
namespace meta = std::meta;

namespace refl {
struct S { int x; void f(); };
struct Base {};
struct T : Base { int a; double b; };
enum class SomeEnum { x, y };
enum class Color { red, green };
struct Derived : Base {};
struct Incomplete;
consteval {
    meta::define_aggregate(^^Incomplete, {meta::data_member_spec(^^int, {.name = "x"}),
                                          meta::data_member_spec(^^int, {.name = "y"})});
}

template <class E>
constexpr std::string_view to_string(E value) {
    template for (constexpr auto e : std::define_static_array(meta::enumerators_of(^^E)))
        if (value == [:e:]) return meta::identifier_of(e);
    return "<unknown>";
}
}  // namespace refl
#endif

// "Mental model": std::meta::info is the type of a reflection.
TEST(Reflection, InfoIsTypeOfReflection) {
    CPPDOCS_REQUIRE(__cpp_impl_reflection);
#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
    static_assert(std::is_same_v<decltype(^^::), meta::info>);
#endif
}

// "Querying entities": name, type, parent and the category/access predicates.
TEST(Reflection, QueryingEntities) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
    constexpr meta::info t = ^^std::string;
    static_assert(meta::identifier_of(t) == "string");
    static_assert(meta::type_of(^^refl::S::x) == ^^int);
    static_assert(meta::parent_of(t) == ^^std);
    static_assert(meta::is_type(t));
    static_assert(meta::is_function(^^refl::S::f));
    static_assert(meta::is_public(^^refl::S::f));
#endif
}

// "Listing members, bases, enumerators": the counts and the layout queries.
TEST(Reflection, ListingAndLayout) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
    constexpr auto ctx = meta::access_context::current();
    static_assert(meta::members_of(^^refl::T, ctx).size() >= 2);
    static_assert(meta::nonstatic_data_members_of(^^refl::T, ctx).size() == 2);
    static_assert(meta::bases_of(^^refl::T, ctx).size() == 1);
    static_assert(meta::enumerators_of(^^refl::SomeEnum).size() == 2);
    constexpr auto member = meta::nonstatic_data_members_of(^^refl::T, ctx)[1];
    static_assert(meta::identifier_of(member) == "b");
    static_assert(meta::offset_of(member).bytes == offsetof(refl::T, b));
    static_assert(meta::size_of(^^refl::T) == sizeof(refl::T) && meta::alignment_of(^^refl::T) == alignof(refl::T));
#endif
}

// "Enum ↔ string, generated": to_string(Color::green) is "green".
TEST(Reflection, EnumToString) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
    static_assert(refl::to_string(refl::Color::green) == "green");
    EXPECT_EQ(refl::to_string(refl::Color::red), "red");
    EXPECT_EQ(refl::to_string(static_cast<refl::Color>(7)), "<unknown>");
#endif
}

// "Type transformations": the reflection-flavoured traits.
TEST(Reflection, TypeTransformations) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
    static_assert(meta::add_const(^^int) == ^^const int);
    static_assert(meta::remove_reference(^^int&) == ^^int);
    static_assert(meta::decay(^^int[3]) == ^^int*);
    static_assert(meta::is_same_type(^^int, ^^int));
    static_assert(meta::is_base_of_type(^^refl::Base, ^^refl::Derived));
    static_assert(meta::is_convertible_type(^^int, ^^double));
#endif
}

// "Generating and substituting": substitute, extract and define_aggregate.
TEST(Reflection, GeneratingAndSubstituting) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
    constexpr meta::info v = meta::substitute(^^std::vector, {^^int});
    using V = [:v:];
    static_assert(std::is_same_v<V, std::vector<int>>);
    constexpr int n = meta::extract<int>(meta::reflect_constant(42));
    static_assert(n == 42);
    refl::Incomplete pt{1, 2};
    EXPECT_EQ(pt.x, 1);
    EXPECT_EQ(pt.y, 2);
#endif
}
