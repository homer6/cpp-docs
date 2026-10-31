// Checks claims made by docs/examples/language/templates/specialization-ctad.md.
#include <string>

#include <gtest/gtest.h>

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
