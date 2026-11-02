// Checks claims made by docs/examples/language/expressions/conversions.md.
#include <type_traits>
#include <typeinfo>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
struct Base {};
struct Derived : Base {};

struct Animal { virtual ~Animal() = default; };
struct Dog : Animal {};
struct Cat : Animal {};

struct Meters {
    double v;
    explicit Meters(double d) : v(d) {}
    explicit operator double() const { return v; }
};

template <class T> concept BraceFromDouble = requires(double d) { T{d}; };
template <class To, class From> concept DynamicCastable = requires(From* f) { dynamic_cast<To*>(f); };
}  // namespace

// "Implicit conversions": double -> int truncates; widening is value-preserving; upcasts are implicit.
TEST(Conversions, ImplicitConversions) {
    double src = 3.9;
    int i = static_cast<int>(src);  // the page's `int i = 3.9;` (written explicitly to avoid the warning)
    EXPECT_EQ(i, 3);
    double d = i;
    long L = i;
    EXPECT_EQ(d, 3.0);
    EXPECT_EQ(L, 3L);
    int* ptr = nullptr;
    EXPECT_FALSE(static_cast<bool>(ptr));
    static_assert(std::is_convertible_v<Derived*, Base*>);
    static_assert(!std::is_convertible_v<Base*, Derived*>);
    static_assert(!BraceFromDouble<int>);  // int x{3.9} is ill-formed
    int narrow(src);                        // narrowing in () compiles silently
    EXPECT_EQ(narrow, 3);
}

// "The four named casts": static_cast forces floating division; dynamic_cast checks at run time.
TEST(Conversions, NamedCasts) {
    int a = 1, b = 2;
    EXPECT_EQ(static_cast<double>(a) / b, 0.5);
    EXPECT_EQ(a / b, 0);

    Dog dog;
    Cat cat;
    Animal* pa = &dog;
    Animal* pc = &cat;
    EXPECT_EQ(dynamic_cast<Dog*>(pa), &dog);
    EXPECT_EQ(dynamic_cast<Dog*>(pc), nullptr);       // pointer: nullptr on failure
    EXPECT_THROW((void)dynamic_cast<Dog&>(*pc), std::bad_cast);  // reference: throws

    int x = 5;
    const int* cp = &x;
    *const_cast<int*>(cp) = 6;  // fine: x itself isn't const
    EXPECT_EQ(x, 6);

    enum class E { a = 3 };
    EXPECT_EQ(static_cast<int>(E::a), 3);
    EXPECT_EQ(static_cast<E>(3), E::a);
    void* vp = &x;
    EXPECT_EQ(static_cast<int*>(vp), &x);
}

// "Gotchas": dynamic_cast down a hierarchy needs a polymorphic type.
TEST(Conversions, DynamicCastNeedsPolymorphicType) {
    static_assert(DynamicCastable<Dog, Animal>);
    static_assert(!DynamicCastable<Derived, Base>);
    static_assert(std::is_polymorphic_v<Animal> && !std::is_polymorphic_v<Base>);
}

// "Explicit constructors & conversion operators": no silent conversions either way.
TEST(Conversions, ExplicitBlocksImplicit) {
    static_assert(!std::is_convertible_v<double, Meters>);
    static_assert(!std::is_convertible_v<Meters, double>);
    static_assert(std::is_constructible_v<Meters, double>);
    Meters m{5.0};
    EXPECT_EQ(static_cast<double>(m), 5.0);
}

namespace auto_cast {
struct NoMove {
    NoMove() = default;
    NoMove(const NoMove&) = delete;
    NoMove(NoMove&&) = delete;
};
NoMove make() { return {}; }
}  // namespace auto_cast

// "auto(x) — make a copy (C++23)": a decayed prvalue copy.
TEST(Conversions, AutoDecayCopy) {
    CPPDOCS_REQUIRE_VERSION(__cpp_auto_cast, 202110L);
#if __cpp_auto_cast >= 202110L  // MSVC: Build Tools 14.50 and later
    std::vector<int> v{1, 2, 1, 3, 1};
    std::erase(v, auto(v.front()));
    EXPECT_EQ(v, (std::vector<int>{2, 3}));
    const int& r = v[0];
    static_assert(std::is_same_v<decltype(auto(r)), int>);
    static_assert(std::is_same_v<decltype(auto{r}), int>);
    int arr[3]{};
    static_assert(std::is_same_v<decltype(auto(arr)), int*>);  // arrays decay
    [[maybe_unused]] auto x = auto(auto_cast::make());          // prvalue: no copy or move needed
#endif
}
