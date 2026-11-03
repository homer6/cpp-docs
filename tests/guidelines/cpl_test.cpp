// Checks claims made by docs/guidelines/core-guidelines/examples/cpl.md.
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

// A stand-in for a C library: an opaque handle with acquire and release functions.
extern "C" {
struct cpl_handle { int id; };
static int cpl_open_count = 0;
static cpl_handle* cpl_acquire(int id) { ++cpl_open_count; return new cpl_handle{id}; }
static void cpl_release(cpl_handle* h) { --cpl_open_count; delete h; }
}

namespace {
struct HandleCloser { void operator()(cpl_handle* h) const { cpl_release(h); } };
using Handle = std::unique_ptr<cpl_handle, HandleCloser>;

int use_handle(int id, bool fail) {
    Handle h{cpl_acquire(id)};
    if (fail) throw std::runtime_error("failed");
    return h->id;
}
}  // namespace

// CPL.1: C++ doesn't convert void* to another object pointer implicitly, as C does.
TEST(CoreGuidelinesCPL, CPL1_NoImplicitConversionFromVoidPointer) {
    static_assert(!std::is_convertible_v<void*, int*>);
    static_assert(std::is_convertible_v<int*, void*>);
    SUCCEED();
}

// CPL.2: compiled as C++, code can't silently drop const from a pointer, which C only warns about.
TEST(CoreGuidelinesCPL, CPL2_CppRejectsDroppingConst) {
    static_assert(!std::is_convertible_v<const int*, int*>);
    SUCCEED();
}

// CPL.3: a C handle wrapped in a C++ owner is released on every path, including exceptions.
TEST(CoreGuidelinesCPL, CPL3_WrapCHandlesInCppOwners) {
    cpl_open_count = 0;
    EXPECT_EQ(use_handle(7, false), 7);
    EXPECT_EQ(cpl_open_count, 0);
    EXPECT_THROW(use_handle(8, true), std::runtime_error);
    EXPECT_EQ(cpl_open_count, 0);
}
