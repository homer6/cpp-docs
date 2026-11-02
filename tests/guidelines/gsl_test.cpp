// Checks claims made by docs/guidelines/core-guidelines/examples/gsl.md.
#include <concepts>
#include <memory>

#include <gtest/gtest.h>

namespace {
template <class P>
concept Pointer = std::default_initializable<P> && std::equality_comparable<P> &&
                  requires(P p) { *p; p.operator->(); };
template <class P>
concept Unique_pointer = Pointer<P> && std::movable<P> && !std::copyable<P>;
template <class P>
concept Shared_pointer = Pointer<P> && std::copyable<P>;

struct Point { int x = 1; };
}  // namespace

// GSL.ptr: unique_ptr is a unique pointer (movable, not copyable); shared_ptr is a shared pointer (copyable).
TEST(CoreGuidelinesGSL, GSLptr_StandardSmartPointersMatchTheConcepts) {
    static_assert(Pointer<std::unique_ptr<Point>> && Pointer<std::shared_ptr<Point>>);
    static_assert(Unique_pointer<std::unique_ptr<Point>>);
    static_assert(!Shared_pointer<std::unique_ptr<Point>>);
    static_assert(Shared_pointer<std::shared_ptr<Point>>);
    static_assert(!Unique_pointer<std::shared_ptr<Point>>);
    const std::unique_ptr<Point> empty;   // default construction gives the null value
    EXPECT_EQ(empty, nullptr);
}
