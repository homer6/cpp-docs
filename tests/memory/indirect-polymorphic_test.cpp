// Checks claims made by docs/examples/memory/indirect-polymorphic.md.
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

#ifdef __cpp_lib_indirect
namespace {
// "indirect for pImpl": T may be incomplete where the member is declared.
class Gadget {
    struct Impl;
    std::indirect<Impl> impl_;
public:
    Gadget();
    Gadget(const Gadget&);
    Gadget& operator=(const Gadget&);
    ~Gadget();
    std::string& name();
};
struct Gadget::Impl { std::string name = "w"; };
Gadget::Gadget() = default;
Gadget::Gadget(const Gadget&) = default;
Gadget& Gadget::operator=(const Gadget&) = default;
Gadget::~Gadget() = default;
std::string& Gadget::name() { return impl_->name; }
}  // namespace
#endif

#ifdef __cpp_lib_polymorphic
namespace {
struct Shape {
    virtual ~Shape() = default;
    virtual double area() const = 0;
};
struct Square : Shape {
    double s;
    explicit Square(double s) : s(s) {}
    double area() const override { return s * s; }
};
template <class T>
concept EqualityComparable = requires(const T& a, const T& b) { a == b; };
}  // namespace
#endif

// Macro values stated in the intro.
TEST(IndirectPolymorphic, FeatureMacros) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_indirect, 202502L);
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_polymorphic, 202502L);
}

// "indirect: deep copy and value comparison"; "Declaration": const-correct access.
TEST(IndirectPolymorphic, IndirectDeepCopyAndCompare) {
    CPPDOCS_REQUIRE(__cpp_lib_indirect);
#ifdef __cpp_lib_indirect
    std::indirect<std::string> a{"hello"};
    auto b = a;
    *b += "!";
    EXPECT_EQ(*a, "hello");
    EXPECT_EQ(*b, "hello!");
    EXPECT_NE(&*a, &*b);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a == std::indirect<std::string>{"hello"});

    std::indirect<int> n;
    EXPECT_EQ(*n, 0);  // holds 0, not null

    static_assert(std::is_same_v<decltype(*std::declval<const std::indirect<int>&>()), const int&>);
    static_assert(!std::is_constructible_v<bool, std::indirect<int>>);  // no operator bool
#endif
}

// "indirect for pImpl": the defaulted copy constructor deep-copies Impl.
TEST(IndirectPolymorphic, IndirectPimplCopies) {
    CPPDOCS_REQUIRE(__cpp_lib_indirect);
#ifdef __cpp_lib_indirect
    Gadget a;
    Gadget b = a;
    b.name() = "copy";
    EXPECT_EQ(a.name(), "w");
    EXPECT_EQ(b.name(), "copy");
#endif
}

// "polymorphic: copying without clone()": copies keep the derived type; no ==.
TEST(IndirectPolymorphic, PolymorphicCopiesDerived) {
    CPPDOCS_REQUIRE(__cpp_lib_polymorphic);
#ifdef __cpp_lib_polymorphic
    std::vector<std::polymorphic<Shape>> shapes;
    shapes.emplace_back(std::in_place_type<Square>, 3.0);
    auto copy = shapes;
    EXPECT_DOUBLE_EQ(copy[0]->area(), 9.0);
    EXPECT_NE(&*copy[0], &*shapes[0]);
    EXPECT_NE(dynamic_cast<const Square*>(&*copy[0]), nullptr);
    static_assert(!EqualityComparable<std::polymorphic<Shape>>);
#endif
}

// "Gotchas": moved-from means valueless.
TEST(IndirectPolymorphic, MovedFromIsValueless) {
    CPPDOCS_REQUIRE(__cpp_lib_indirect);
    CPPDOCS_REQUIRE(__cpp_lib_polymorphic);
#if defined(__cpp_lib_indirect) && defined(__cpp_lib_polymorphic)
    std::indirect<int> a{5};
    auto b = std::move(a);
    EXPECT_TRUE(a.valueless_after_move());
    EXPECT_FALSE(b.valueless_after_move());

    std::polymorphic<Shape> p{std::in_place_type<Square>, 2.0};
    auto q = std::move(p);
    EXPECT_TRUE(p.valueless_after_move());
    EXPECT_DOUBLE_EQ(q->area(), 4.0);
#endif
}
