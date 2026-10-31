// Checks claims made by docs/examples/language/classes/inheritance-polymorphism.md.

#include <memory>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

namespace {

class Shape {  // "Inheritance"
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
    virtual std::string name() const { return "shape"; }
};

class Circle : public Shape {
    double r_;
public:
    explicit Circle(double r) : r_{r} {}
    double area() const override { return 3.14159 * r_ * r_; }
    std::string name() const override { return "circle"; }
};

double render(const Shape& sh) { return sh.area(); }

class Sealed final {};
struct HasFinal { virtual ~HasFinal() = default; virtual int g() const { return 0; } };
struct OverridesFinal : HasFinal { int g() const final { return 1; } };

// Hiding the base virtual is the point of Dog::greet.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
struct Animal {
    virtual ~Animal() = default;
    virtual std::string speak() const { return "..."; }
    virtual std::string greet() const { return "base"; }
};
struct Dog : Animal {
    std::string speak() const override { return "woof"; }
    std::string greet() { return "dog"; }  // missing const: a new function, not an override
};
#pragma GCC diagnostic pop

struct Logged {  // records which destructors ran
    static inline std::string log;
};
struct VBase { virtual ~VBase() { Logged::log += "base "; } };
struct VDerived : VBase { ~VDerived() override { Logged::log += "derived "; } };

std::string seen_in_ctor;
struct CBase {
    CBase() { seen_in_ctor = who(); }
    virtual ~CBase() = default;
    virtual std::string who() const { return "base"; }
};
struct CDerived : CBase { std::string who() const override { return "derived"; } };

}  // namespace

// "Inheritance" and "Abstract base classes": a pure virtual makes the class abstract; is-a.
TEST(InheritancePolymorphism, AbstractBaseAndIsA) {
    static_assert(std::is_abstract_v<Shape>);
    static_assert(!std::is_abstract_v<Circle>);
    static_assert(std::is_base_of_v<Shape, Circle>);
    static_assert(std::is_convertible_v<Circle*, Shape*>);
    static_assert(std::has_virtual_destructor_v<Shape>);
}

// "Dynamic dispatch": a base pointer/reference calls the most-derived override.
TEST(InheritancePolymorphism, DynamicDispatch) {
    std::unique_ptr<Shape> s = std::make_unique<Circle>(2.0);
    EXPECT_DOUBLE_EQ(s->area(), 3.14159 * 4.0);
    EXPECT_EQ(s->name(), "circle");
    EXPECT_DOUBLE_EQ(render(*s), s->area());
}

// "override and final": final classes can't be derived from; final ends overriding.
TEST(InheritancePolymorphism, OverrideAndFinal) {
    static_assert(std::is_final_v<Sealed>);
    static_assert(!std::is_final_v<OverridesFinal>);
    std::unique_ptr<HasFinal> p = std::make_unique<OverridesFinal>();
    EXPECT_EQ(p->g(), 1);
}

// "Gotchas": slicing loses the override; a signature mismatch silently calls the base.
TEST(InheritancePolymorphism, SlicingAndMissingOverride) {
    Dog d;
    const Animal& ref = d;
    Animal sliced = d;
    EXPECT_EQ(ref.speak(), "woof");
    EXPECT_EQ(sliced.speak(), "...");
    EXPECT_EQ(ref.greet(), "base");
}

// "Gotchas": deleting through a base pointer runs the derived destructor when it's virtual.
TEST(InheritancePolymorphism, VirtualDestructorRunsDerived) {
    Logged::log.clear();
    std::unique_ptr<VBase> p = std::make_unique<VDerived>();
    p.reset();
    EXPECT_EQ(Logged::log, "derived base ");
}

// "Gotchas": during base construction, a virtual call reaches the base version.
TEST(InheritancePolymorphism, VirtualCallInConstructorIsBase) {
    CDerived d;
    EXPECT_EQ(seen_in_ctor, "base");
    EXPECT_EQ(d.who(), "derived");
}

// Tests added with the page sections from #56 (#69); kept in their own
// namespace so their helpers and test names can't clash with the ones above.
namespace from_pr69 {
namespace {
struct Device { int id; explicit Device(int i = 0) : id{i} {} };
struct Scanner : virtual Device { Scanner() : Device{1} {} };
struct Printer : virtual Device { Printer() : Device{2} {} };
struct Copier : Scanner, Printer { Copier() : Device{3} {} };
}  // namespace

// "Multiple inheritance & virtual bases": one shared base, built by the most-derived class.
TEST(InheritancePolymorphism, VirtualBaseBuiltByMostDerived) {
    Copier c;
    EXPECT_EQ(c.id, 3);
    EXPECT_EQ(&static_cast<Scanner&>(c).id, &static_cast<Printer&>(c).id);
    EXPECT_EQ(Scanner{}.id, 1);
}
}  // namespace from_pr69
