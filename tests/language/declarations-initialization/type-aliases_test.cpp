// Checks claims made by docs/examples/language/declarations-initialization/type-aliases.md.
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

namespace {
using Bytes = std::vector<std::uint8_t>;
typedef std::vector<std::uint8_t> BytesT;

using Callback = int (*)(int, int);
typedef int (*CallbackT)(int, int);
int add(int a, int b) { return a + b; }

template <class T> using Vec = std::vector<T>;
template <class K, class V> using Dict = std::unordered_map<K, V>;

struct MyContainer {
    using value_type = int;
    using iterator = int*;
    int data[2] = {7, 8};
};

template <class C>
typename C::value_type first(const C& c) { return c.data[0]; }

using Meters = double;
double twice(Meters m) { return m * 2; }
}  // namespace

// "using vs typedef": both spellings name the same type.
TEST(TypeAliases, UsingEqualsTypedef) {
    static_assert(std::is_same_v<Bytes, BytesT>);
    static_assert(std::is_same_v<Callback, CallbackT>);
    Callback cb = add;
    EXPECT_EQ(cb(2, 3), 5);
}

// "Alias templates": Vec<int> is std::vector<int>; the _t helpers are alias templates.
TEST(TypeAliases, AliasTemplates) {
    static_assert(std::is_same_v<Vec<int>, std::vector<int>>);
    static_assert(std::is_same_v<Dict<int, char>, std::unordered_map<int, char>>);
    static_assert(std::is_same_v<std::remove_cvref_t<const int&>,
                                 typename std::remove_cvref<const int&>::type>);
}

// "Member type aliases": generic code reads C::value_type.
TEST(TypeAliases, MemberAliases) {
    MyContainer c;
    static_assert(std::is_same_v<decltype(first(c)), int>);
    static_assert(std::is_same_v<MyContainer::iterator, int*>);
    EXPECT_EQ(first(c), 7);
}

// Gotchas: an alias is not a new type — a plain double passes for Meters.
TEST(TypeAliases, AliasIsNotANewType) {
    static_assert(std::is_same_v<Meters, double>);
    double plain = 1.5;
    EXPECT_EQ(twice(plain), 3.0);
}
