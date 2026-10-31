// Checks claims made by docs/examples/language/classes/inheritance-polymorphism.md.
#include <gtest/gtest.h>

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
