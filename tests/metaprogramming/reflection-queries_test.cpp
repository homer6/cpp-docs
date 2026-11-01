// Checks claims made by docs/examples/metaprogramming/reflection-queries.md.
// GCC 16 builds this file with -freflection (see tests/CMakeLists.txt); on
// other toolchains every test skips.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
#include <meta>
#define CPPDOCS_HAVE_REFLECTION 1
namespace meta = std::meta;

void reflq_connect(const char* host, int port = 80);
void reflq_g(int a);
void reflq_g(int b);

namespace reflq {
struct V {
    V operator+(V) const;
    bool operator==(const V&) const = default;
};
struct Config { int port; }; constexpr unsigned config_line = __LINE__;
struct B {
    virtual void f();
    static int s;
    const int k = 0;
};
struct Flags { std::uint8_t a : 3; std::uint8_t b : 5; int c; };

consteval bool throws_size_of_bitfield() {
    try { (void)meta::size_of(^^Flags::a); } catch (const meta::exception&) { return true; }
    return false;
}
using VI = std::vector<int>;
consteval bool throws_template_of_alias() {
    try { (void)meta::template_of(^^VI); } catch (const meta::exception&) { return true; }
    return false;
}
}  // namespace reflq
#endif

// "Function parameters (P3096)": names, types, default arguments, return type.
TEST(ReflectionQueries, Parameters) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#ifdef CPPDOCS_HAVE_REFLECTION
    constexpr auto params = std::define_static_array(meta::parameters_of(^^reflq_connect));
    static_assert(params.size() == 2);
    static_assert(meta::identifier_of(params[0]) == "host");
    static_assert(meta::identifier_of(params[1]) == "port");
    static_assert(meta::type_of(params[0]) == ^^const char*);
    static_assert(!meta::has_default_argument(params[0]));
    static_assert(meta::has_default_argument(params[1]));
    static_assert(meta::return_type_of(^^reflq_connect) == ^^void);
    static_assert(meta::parameters_of(^^void(int, double)).size() == 2);
    static_assert(!meta::has_identifier(meta::parameters_of(^^void(int))[0]));  // no names
    // declarations that disagree on the name leave the parameter unnamed
    static_assert(!meta::has_identifier(meta::parameters_of(^^reflq_g)[0]));
#endif
}

// "Templates and their arguments": template_of, template_arguments_of, dealias.
TEST(ReflectionQueries, Templates) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#ifdef CPPDOCS_HAVE_REFLECTION
    constexpr auto a = ^^std::array<int, 3>;
    static_assert(meta::has_template_arguments(a));
    static_assert(meta::template_of(a) == ^^std::array);
    static_assert(meta::template_arguments_of(a)[0] == ^^int);
    static_assert(meta::extract<std::size_t>(meta::template_arguments_of(a)[1]) == 3);
    static_assert(meta::template_arguments_of(^^std::vector<int>).size() == 2);
    static_assert(meta::template_arguments_of(^^std::vector<int>)[1] == ^^std::allocator<int>);
    static_assert(!meta::has_template_arguments(^^reflq::VI));
    static_assert(meta::has_template_arguments(meta::dealias(^^reflq::VI)));
    static_assert(!meta::has_template_arguments(^^int));
    // "Gotchas": template_of on the alias throws
    static_assert(reflq::throws_template_of_alias());
#endif
}

// "Operators": operator_of returns a std::meta::operators enumerator; symbol_of spells it.
TEST(ReflectionQueries, Operators) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#ifdef CPPDOCS_HAVE_REFLECTION
    static_assert(meta::is_operator_function(^^reflq::V::operator+));
    static_assert(meta::operator_of(^^reflq::V::operator+) == meta::operators::op_plus);
    static_assert(meta::symbol_of(meta::operator_of(^^reflq::V::operator==)) == "==");
    EXPECT_EQ(std::string(meta::symbol_of(meta::op_plus)), "+");
#endif
}

// "Names and source locations": source_location_of points at the declaration;
// display_string_of is implementation-defined text.
TEST(ReflectionQueries, SourceLocationAndDisplayString) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#ifdef CPPDOCS_HAVE_REFLECTION
    constexpr auto loc = meta::source_location_of(^^reflq::Config);
    EXPECT_NE(std::string(loc.file_name()).find("reflection-queries_test.cpp"), std::string::npos);
    EXPECT_EQ(loc.line(), reflq::config_line);
    static_assert(!meta::has_identifier(^^int));
    const std::string shown(meta::display_string_of(^^reflq::VI));
#if defined(__GNUC__) && !defined(__clang__)
    EXPECT_EQ(shown, "reflq::VI {aka std::vector<int>}");  // GCC 16's spelling
#else
    EXPECT_FALSE(shown.empty());
#endif
#endif
}

// "Predicates": is_virtual, is_static_member, is_const / is_const_type.
TEST(ReflectionQueries, Predicates) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#ifdef CPPDOCS_HAVE_REFLECTION
    static_assert(meta::is_virtual(^^reflq::B::f));
    static_assert(meta::is_static_member(^^reflq::B::s));
    static_assert(!meta::is_static_member(^^reflq::B::k));
    static_assert(meta::is_const(^^reflq::B::k));
    static_assert(meta::is_const_type(meta::type_of(^^reflq::B::k)));
    static_assert(meta::is_public(^^reflq::B::f));
#endif
}

// "Layout": member_offset {bytes, bits}, bit_size_of, size_of, alignment_of.
TEST(ReflectionQueries, Layout) {
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#ifdef CPPDOCS_HAVE_REFLECTION
    static_assert(meta::bit_size_of(^^reflq::Flags::b) == 5);
    static_assert(meta::offset_of(^^reflq::Flags::b).bytes == 0);
    static_assert(meta::offset_of(^^reflq::Flags::b).bits == 3);
    static_assert(meta::offset_of(^^reflq::Flags::c).bytes == offsetof(reflq::Flags, c));
    static_assert(meta::offset_of(^^reflq::Flags::c).total_bits() == 8 * offsetof(reflq::Flags, c));
    static_assert(meta::size_of(^^reflq::Flags) == sizeof(reflq::Flags));
    static_assert(meta::alignment_of(^^reflq::Flags) == alignof(reflq::Flags));
    // "Gotchas": size_of a bit-field throws
    static_assert(reflq::throws_size_of_bitfield());
#endif
}
