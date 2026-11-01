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

// "Create with make_unique": get() borrows without transferring ownership.
TEST(UniquePtr, GetBorrowsWithoutTransfer) {
    auto p = std::make_unique<int>(42);
    int* raw = p.get();
    EXPECT_EQ(raw, &*p);
    EXPECT_NE(p, nullptr);  // still the owner
    EXPECT_EQ(*raw, 42);
}

// "Ownership transfer": returning a local by value moves it out, no std::move needed.
TEST(UniquePtr, ReturnByValueMovesOut) {
    auto make = [] {
        auto p = std::make_unique<int>(3);
        return p;
    };
    auto q = make();
    ASSERT_NE(q, nullptr);
    EXPECT_EQ(*q, 3);
}

// "As a class member": the owner frees the member without a user-written destructor.
TEST(UniquePtr, MemberFreedWithOwner) {
    struct Owner {
        std::unique_ptr<Tracked> impl_ = std::make_unique<Tracked>();
    };
    live = 0;
    {
        Owner o;
        EXPECT_EQ(live, 1);
    }
    EXPECT_EQ(live, 0);
}

// "Arrays and custom deleters": unique_ptr<int[]> indexes and uses delete[].
TEST(UniquePtr, ArrayFormUsesDeleteArray) {
    auto arr = std::make_unique<int[]>(100);
    arr[0] = 1;
    EXPECT_EQ(arr[0], 1);
    static_assert(std::is_same_v<decltype(arr)::deleter_type, std::default_delete<int[]>>);
}

namespace {
struct CHandle {
    int v;
};
int handle_create(CHandle** out, int v) {
    *out = new CHandle{v};
    return 0;
}
int handle_grow(CHandle** io) {
    auto* n = new CHandle{(*io)->v * 10};
    delete *io;
    *io = n;
    return 0;
}
int handle_deletes = 0;
struct CHandleFree {
    void operator()(CHandle* h) const {
        ++handle_deletes;
        delete h;
    }
};
std::unique_ptr<CHandle, CHandleFree>* watched = nullptr;
bool watched_was_null = false;
int handle_peek(CHandle** out) {
    watched_was_null = (*watched == nullptr);
    *out = new CHandle{2};
    return 0;
}
}  // namespace

// "C APIs that return through T**": out_ptr adopts the result; inout_ptr passes
// the old pointer in and adopts the replacement.
TEST(UniquePtr, OutPtrAndInoutPtr) {
    std::unique_ptr<CHandle, CHandleFree> h;
    handle_create(std::out_ptr(h), 7);
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(h->v, 7);
    handle_grow(std::inout_ptr(h));
    EXPECT_EQ(h->v, 70);

    std::shared_ptr<CHandle> s;
    handle_create(std::out_ptr(s, CHandleFree{}), 3);  // shared_ptr needs the deleter
    EXPECT_EQ(s->v, 3);
    handle_deletes = 0;
    s.reset();
    EXPECT_EQ(handle_deletes, 1);  // the passed deleter is used
}

// "Gotchas": out_ptr resets the smart pointer before the call (old object freed first).
TEST(UniquePtr, OutPtrResetsBeforeCall) {
    std::unique_ptr<CHandle, CHandleFree> h{new CHandle{1}};
    watched = &h;
    handle_deletes = 0;
    handle_peek(std::out_ptr(h));
    EXPECT_TRUE(watched_was_null);
    EXPECT_EQ(handle_deletes, 1);
    EXPECT_EQ(h->v, 2);
    watched = nullptr;
}
