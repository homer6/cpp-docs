// Checks claims made by docs/examples/language-support/type-info.md.
#include <compare>
#include <cstdlib>
#if __has_include(<cxxabi.h>)
#include <cxxabi.h>  // Itanium C++ ABI only: not on MSVC or clang-cl
#endif
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include <gtest/gtest.h>

namespace {
struct Base { virtual ~Base() = default; };
struct Derived : Base {};
struct Plain {};
struct PlainDerived : Plain {};

#if __has_include(<cxxabi.h>)
std::string demangle(const char* mangled) {
    int status = 0;
    std::unique_ptr<char, decltype(&std::free)> p{
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), &std::free};
    return status == 0 ? p.get() : mangled;
}
#endif

int calls = 0;
// Only named in an unevaluated operand, hence [[maybe_unused]].
[[maybe_unused]] Plain make_plain() { ++calls; return {}; }
Base& base_ref() { static Derived d; ++calls; return d; }
}  // namespace

// "Static vs dynamic type": the three trues.
TEST(TypeInfo, StaticVersusDynamic) {
    Derived d;
    Base& b = d;
    PlainDerived pd;
    Plain& p = pd;
    EXPECT_TRUE(typeid(b) == typeid(Derived));
    EXPECT_TRUE(typeid(p) == typeid(Plain));
    EXPECT_TRUE(typeid(const int&) == typeid(int));
}

// "Mental model": type_info isn't copyable or assignable; type_index is copyable,
// ordered (<=> strong_ordering) and hashable.
TEST(TypeInfo, TypeInfoVersusTypeIndex) {
    static_assert(!std::is_copy_constructible_v<std::type_info>);
    static_assert(!std::is_copy_assignable_v<std::type_info>);
    static_assert(std::is_copy_constructible_v<std::type_index>);
    static_assert(std::is_same_v<decltype(std::type_index(typeid(int)) <=> std::type_index(typeid(int))),
                                 std::strong_ordering>);
    EXPECT_EQ(std::hash<std::type_index>{}(typeid(int)), std::type_index(typeid(int)).hash_code());
}

// "A map keyed by type".
TEST(TypeInfo, MapKeyedByType) {
    std::unordered_map<std::type_index, std::string> names{
        {typeid(int), "int"}, {typeid(double), "double"}, {typeid(std::string), "string"}};
    EXPECT_EQ(names.at(typeid(int)), "int");
}

// "name() is mangled on GCC and Clang" + "Gotchas": the exact spellings.
TEST(TypeInfo, NameIsImplementationDefined) {
#if defined(_MSC_VER)
    // MSVC ABI (MSVC, clang-cl): name() is already the readable, undecorated name.
    EXPECT_STREQ(typeid(int).name(), "int");
    EXPECT_NE(std::string(typeid(std::string).name()).find("std::basic_string<char"), std::string::npos)
        << typeid(std::string).name();
#else
    EXPECT_STREQ(typeid(int).name(), "i");
#if defined(_LIBCPP_VERSION)
    EXPECT_NE(demangle(typeid(std::string).name()).find("std::__1::basic_string"), std::string::npos);
#elif defined(__GLIBCXX__)
    EXPECT_STREQ(typeid(std::string).name(), "NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE");
    EXPECT_EQ(demangle(typeid(std::string).name()),
              "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >");
#else
    GTEST_SKIP() << "spelling recorded only for libstdc++ and libc++";
#endif
#endif
}

// "Gotchas": before() order differs — false with libstdc++, true with libc++.
TEST(TypeInfo, BeforeIsImplementationDefined) {
    bool b = typeid(int).before(typeid(double));
#if defined(_LIBCPP_VERSION)
    EXPECT_TRUE(b);
#elif defined(__GLIBCXX__)
    EXPECT_FALSE(b);
#else
    GTEST_SKIP() << "order recorded only for libstdc++ and libc++";
#endif
    EXPECT_NE(typeid(int).before(typeid(double)), typeid(double).before(typeid(int)));
}

// "typeid on a null pointer throws" bad_typeid.
TEST(TypeInfo, NullPolymorphicThrows) {
    Base* p = nullptr;
    EXPECT_THROW((void)typeid(*p), std::bad_typeid);
}

// "Gotchas": the operand is evaluated only for polymorphic glvalues.
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wpotentially-evaluated-expression"
#endif
TEST(TypeInfo, OperandEvaluatedOnlyIfPolymorphic) {
    calls = 0;
    (void)typeid(make_plain());
    EXPECT_EQ(calls, 0);
    EXPECT_TRUE(typeid(base_ref()) == typeid(Derived));
    EXPECT_EQ(calls, 1);
}
