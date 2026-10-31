// Checks claims made by docs/examples/memory/weak_ptr.md.
#include <memory>

#include <gtest/gtest.h>

// "Observe without owning": a weak_ptr doesn't bump the strong count; lock()
// returns an owner while alive and an empty pointer afterwards; expired() turns true.
TEST(WeakPtr, LockAndExpired) {
    auto sp = std::make_shared<int>(5);
    std::weak_ptr<int> wp = sp;
    EXPECT_EQ(sp.use_count(), 1);
    if (auto locked = wp.lock()) {
        EXPECT_EQ(*locked, 5);
        EXPECT_EQ(sp.use_count(), 2);  // lock() promoted to owner
    } else {
        FAIL() << "object should be alive";
    }
    EXPECT_FALSE(wp.expired());
    sp.reset();
    EXPECT_TRUE(wp.expired());
    EXPECT_EQ(wp.lock(), nullptr);
}

// "Breaking a reference cycle": a weak back-edge lets both nodes be destroyed.
TEST(WeakPtr, WeakBackEdgeBreaksCycle) {
    struct Node {
        std::shared_ptr<Node> next;
        std::weak_ptr<Node> prev;
    };
    std::weak_ptr<Node> wa, wb;
    {
        auto a = std::make_shared<Node>();
        auto b = std::make_shared<Node>();
        a->next = b;
        b->prev = a;
        wa = a;
        wb = b;
    }
    EXPECT_TRUE(wa.expired());
    EXPECT_TRUE(wb.expired());
}

// "Gotchas": no operator* / operator->; constructing a shared_ptr from an
// expired weak_ptr throws std::bad_weak_ptr.
template <class P>
concept Derefable = requires(P p) { *p; };
TEST(WeakPtr, NoDerefAndBadWeakPtr) {
    static_assert(!Derefable<std::weak_ptr<int>>);
    std::weak_ptr<int> wp;
    { auto sp = std::make_shared<int>(1); wp = sp; }
    EXPECT_THROW(std::shared_ptr<int>{wp}, std::bad_weak_ptr);
}

// "Gotchas": it pins the control block, not the object — the object is
// destroyed when the last owner goes even while a weak_ptr remains.
TEST(WeakPtr, ObjectDestroyedWhileWeakRemains) {
    static int dtor = 0;
    struct D { ~D() { ++dtor; } };
    dtor = 0;
    auto sp = std::make_shared<D>();
    std::weak_ptr<D> wp = sp;
    sp.reset();
    EXPECT_EQ(dtor, 1);
    EXPECT_EQ(wp.use_count(), 0);
}
