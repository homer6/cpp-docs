// Checks claims made by docs/examples/language/expressions/reflection.md.
// Reflection needs GCC 16 with -freflection (added by tests/CMakeLists.txt);
// on other toolchains every test skips.
#include <string>
#include <type_traits>
#include <vector>
#include <version>

#if defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
#include <format>
#include <meta>
#define CPPDOCS_HAVE_REFLECTION 1
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

#ifdef CPPDOCS_HAVE_REFLECTION
int reflection_twice(int v) { return 2 * v; }  // external linkage: reflected below

namespace {
struct Point { int x; int y; int z; };

template <class T> std::string dump(const T& obj) {  // the page's example, returning the text
    std::string out;
    template for (constexpr auto member :
                  std::define_static_array(
                      std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()))) {
        out += std::format("{} = {}\n", std::meta::identifier_of(member), obj.[:member:]);
    }
    return out;
}

enum class Color { red, green };

struct Range { int lo, hi; };
struct Config {
    [[=Range{1, 65535}]] int port;
    [[=1, =2]]           int level;
                         int plain;
};
}  // namespace

[[=42]] int reflection_annotated();
[[=24]] int reflection_annotated();  // a redeclaration adds its annotations
[[=42, =42]] int reflection_twice_annotated;
#endif

// "Reflecting and splicing": ^^ gives a std::meta::info; [: r :] splices it back.
TEST(Reflection, ReflectAndSplice) {
    CPPDOCS_REQUIRE(__cpp_impl_reflection);
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#ifdef CPPDOCS_HAVE_REFLECTION
    constexpr std::meta::info r = ^^int;
    using T = [:r:];
    static_assert(std::is_same_v<T, int>);
    static_assert(std::is_same_v<decltype(r), const std::meta::info>);

    constexpr auto rv = ^^std::vector;
    static_assert(std::meta::is_template(rv));
    using VI = typename[:std::meta::substitute(rv, {^^int}):];  // typename[: :] in type position
    static_assert(std::is_same_v<VI, std::vector<int>>);

    constexpr auto fn = ^^reflection_twice;
    static_assert(std::meta::is_function(fn));
    EXPECT_EQ([:fn:](21), 42);  // a bare splice in an expression

    constexpr auto ns = ^^::;
    static_assert(std::meta::is_namespace(ns));
    static_assert(ns == std::meta::parent_of(fn));
#endif
}

// "The classic example": dump(Point{1, 2, 3}) prints x = 1 / y = 2 / z = 3.
TEST(Reflection, IterateStructMembers) {
    CPPDOCS_REQUIRE(__cpp_impl_reflection);
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#ifdef CPPDOCS_HAVE_REFLECTION
    EXPECT_EQ(dump(Point{1, 2, 3}), "x = 1\ny = 2\nz = 3\n");
#endif
}

// "What it unlocks": enum-to-string with std::meta::enumerators_of.
TEST(Reflection, EnumeratorsOf) {
    CPPDOCS_REQUIRE(__cpp_impl_reflection);
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#ifdef CPPDOCS_HAVE_REFLECTION
    std::string names;
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^Color)))
        names += std::string(std::meta::identifier_of(e)) + ' ';
    EXPECT_EQ(names, "red green ");
#endif
}

// "Annotations (C++26)": annotations_of, constant_of, annotations_of_with_type, extract.
TEST(Reflection, Annotations) {
    CPPDOCS_REQUIRE(__cpp_impl_reflection);
    CPPDOCS_REQUIRE(__cpp_lib_reflection);
#ifdef CPPDOCS_HAVE_REFLECTION
    namespace meta = std::meta;
    static_assert(meta::annotations_of(^^Config::level).size() == 2);
    static_assert(meta::annotations_of(^^Config::plain).empty());
    static_assert([:meta::constant_of(meta::annotations_of(^^Config::level)[0]):] == 1);
    static_assert([:meta::constant_of(meta::annotations_of(^^Config::level)[1]):] == 2);  // in order
    constexpr auto r = meta::annotations_of_with_type(^^Config::port, ^^Range)[0];
    static_assert(meta::is_annotation(r));
    constexpr Range range = meta::extract<Range>(r);
    EXPECT_EQ(range.lo, 1);
    EXPECT_EQ(range.hi, 65535);
    // "Gotchas": every annotation is distinct, and redeclarations add up.
    static_assert(meta::annotations_of(^^reflection_twice_annotated).size() == 2);
    static_assert(meta::annotations_of(^^reflection_annotated).size() == 2);
#endif
}
