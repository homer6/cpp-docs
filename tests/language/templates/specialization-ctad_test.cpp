// Checks claims made by docs/examples/language/templates/specialization-ctad.md.

#include <array>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {
template <class T> struct Serializer { static std::string to_text(const T&) { return "primary"; } };
template <> struct Serializer<bool> {
    static std::string to_text(bool b) { return b ? "true" : "false"; }
};

template <class T> struct IsPointer { static constexpr bool value = false; };
template <class T> struct IsPointer<T*> { static constexpr bool value = true; };
template <class T> struct Traits;
template <class T> struct Traits<std::vector<T>> { using element = T; };

template <class T> struct GuidedBox { GuidedBox(T) {} };
GuidedBox(const char*) -> GuidedBox<std::string>;

template <class T> constexpr const char* kind(T) { return "value"; }    // overload instead of
template <class T> constexpr const char* kind(T*) { return "pointer"; } // partial specialization
}  // namespace

// "Full specialization": Serializer<bool> replaces the primary for bool.
TEST(SpecializationCtad, FullSpecialization) {
    EXPECT_EQ(Serializer<bool>::to_text(true), "true");
    EXPECT_EQ(Serializer<bool>::to_text(false), "false");
    EXPECT_EQ(Serializer<int>::to_text(1), "primary");
}

// "Partial specialization": patterns such as T* and std::vector<T>; functions overload instead.
TEST(SpecializationCtad, PartialSpecializationAndOverloading) {
    static_assert(!IsPointer<int>::value);
    static_assert(IsPointer<int*>::value);
    static_assert(std::is_same_v<Traits<std::vector<double>>::element, double>);
    int x = 0;
    EXPECT_STREQ(kind(x), "value");
    EXPECT_STREQ(kind(&x), "pointer");
}

// "CTAD": the deduced types listed on the page.
TEST(SpecializationCtad, CtadDeducesFromConstructorArguments) {
    std::mutex mtx;
    std::vector v{1, 2, 3};
    std::pair p{1, 2.5};
    std::lock_guard lk{mtx};
    std::array a{1, 2, 3};
    static_assert(std::is_same_v<decltype(v), std::vector<int>>);
    static_assert(std::is_same_v<decltype(p), std::pair<int, double>>);
    static_assert(std::is_same_v<decltype(lk), std::lock_guard<std::mutex>>);
    static_assert(std::is_same_v<decltype(a), std::array<int, 3>>);
}

// "Deduction guides": the guide maps const char* to std::string; the library's iterator guide.
TEST(SpecializationCtad, DeductionGuides) {
    GuidedBox b{"hi"};
    static_assert(std::is_same_v<decltype(b), GuidedBox<std::string>>);
    std::vector<double> src{1.0, 2.0};
    std::vector w(src.begin(), src.end());
    static_assert(std::is_same_v<decltype(w), std::vector<double>>);
    EXPECT_EQ(w, src);
}

// "Gotchas": std::pair p{1, "x"} deduces pair<int, const char*>, not std::string.
TEST(SpecializationCtad, CtadCanSurprise) {
    std::pair p{1, "x"};
    static_assert(std::is_same_v<decltype(p), std::pair<int, const char*>>);
}

// Tests added with the page sections from #56 (#69); kept in their own
// namespace so their helpers and test names can't clash with the ones above.
namespace from_pr69 {
namespace {
template <class T> std::string kind(T) { return "T"; }
template <class T> std::string kind(T*) { return "T*"; }
template <> std::string kind<int*>(int*) { return "int* specialization"; }
std::string kind(double) { return "double overload"; }
}  // namespace

// "Full specialization": overload resolution picks the T* primary, never seeing #1's specialization.
TEST(SpecializationCtad, FunctionSpecializationIgnoredByOverloading) {
    int i = 0;
    EXPECT_EQ(kind(&i), "T*");
    EXPECT_EQ(kind<int*>(&i), "int* specialization");  // only when #1 is named explicitly
    EXPECT_EQ(kind(2.0), "double overload");
    EXPECT_EQ(kind('c'), "T");
}
}  // namespace from_pr69
