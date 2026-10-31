// Checks claims made by docs/examples/memory/unique_ptr.md.
#include <memory>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace {
int live = 0;  // counts objects alive, to observe deletion
struct Tracked {
    Tracked() { ++live; }
    ~Tracked() { --live; }
};
}  // namespace

// "Ownership transfer (move-only)": not copyable; moving leaves the source null.
TEST(UniquePtr, MoveOnlyAndMovedFromIsNull) {
    static_assert(!std::is_copy_constructible_v<std::unique_ptr<int>>);
    static_assert(std::is_nothrow_move_constructible_v<std::unique_ptr<int>>);
    auto a = std::make_unique<int>(7);
    std::unique_ptr<int> b = std::move(a);
    EXPECT_EQ(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(*b, 7);
}

// "Gotchas": release() gives up ownership without deleting; reset() deletes.
TEST(UniquePtr, ReleaseVersusReset) {
    live = 0;
    auto p = std::make_unique<Tracked>();
    Tracked* raw = p.release();
    EXPECT_EQ(p, nullptr);
    EXPECT_EQ(live, 1);  // still alive: we own it now
    delete raw;
    EXPECT_EQ(live, 0);

    auto q = std::make_unique<Tracked>();
    q.reset();
    EXPECT_EQ(q, nullptr);
    EXPECT_EQ(live, 0);  // reset() deleted it
}

// "As a class member = automatic cleanup": destroying the owner frees the object.
TEST(UniquePtr, ScopeEndDeletes) {
    live = 0;
    {
        auto p = std::make_unique<Tracked>();
        EXPECT_EQ(live, 1);
    }
    EXPECT_EQ(live, 0);
}

// "Gotchas": a function-pointer deleter makes unique_ptr bigger than a raw
// pointer; a stateless functor deleter keeps it pointer-sized.
TEST(UniquePtr, DeleterChangesSize) {
    struct FreeInt {
        void operator()(int* p) const { delete p; }
    };
    static_assert(sizeof(std::unique_ptr<int, FreeInt>) == sizeof(int*));
    static_assert(sizeof(std::unique_ptr<int, void (*)(int*)>) > sizeof(int*));
    static_assert(sizeof(std::unique_ptr<int>) == sizeof(int*));
}

// "Arrays and custom deleters": the custom deleter isn't called for a null pointer.
TEST(UniquePtr, CustomDeleterNotCalledOnNull) {
    static int calls = 0;
    calls = 0;
    {
        std::unique_ptr<int, void (*)(int*)> p{nullptr, [](int* q) { ++calls; delete q; }};
    }
    EXPECT_EQ(calls, 0);
    {
        std::unique_ptr<int, void (*)(int*)> p{new int{1}, [](int* q) { ++calls; delete q; }};
    }
    EXPECT_EQ(calls, 1);
}
