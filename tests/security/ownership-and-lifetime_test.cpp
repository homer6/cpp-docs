// Checks claims made by docs/security/ownership-and-lifetime.md.
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {

int closes = 0;
struct FakeHandle { int id; };
struct HandleCloser {
    void operator()(FakeHandle* h) const noexcept { ++closes; delete h; }
};
using Handle = std::unique_ptr<FakeHandle, HandleCloser>;

struct Session {
    std::string user = "alice";
    int replies = 0;
    void on_reply() { ++replies; }
};

}  // namespace

// "Make ownership explicit": an RAII handle releases exactly once, on every path,
// including when an exception unwinds past it.
TEST(SecurityOwnership, RaiiHandleReleasesOnceOnEveryPath) {
    closes = 0;
    { Handle h{new FakeHandle{1}}; }
    EXPECT_EQ(closes, 1);
    try {
        Handle h{new FakeHandle{2}};
        throw 1;
    } catch (int) {}
    EXPECT_EQ(closes, 2);
    Handle a{new FakeHandle{3}};
    Handle b = std::move(a);   // ownership moves, nothing is released yet
    EXPECT_EQ(closes, 2);
    b.reset();
    EXPECT_EQ(closes, 3);
}

// "Make ownership explicit": an exclusive owner can't be copied into a second owner.
TEST(SecurityOwnership, UniquePtrCantBeCopied) {
    static_assert(!std::is_copy_constructible_v<std::unique_ptr<int>>);
    static_assert(!std::is_copy_assignable_v<std::unique_ptr<int>>);
    static_assert(std::is_move_constructible_v<std::unique_ptr<int>>);
    SUCCEED();
}

// "Give every borrow a lifetime argument": a queued callback holding a weak_ptr does
// nothing once the session is gone, and runs normally while it exists.
TEST(SecurityOwnership, WeakPtrCallbackSkipsEndedSession) {
    std::vector<std::function<void()>> queue;
    auto s = std::make_shared<Session>();
    std::weak_ptr<Session> observer = s;
    auto task = [w = std::weak_ptr<Session>{s}] { if (auto p = w.lock()) p->on_reply(); };
    queue.push_back(task);
    queue.push_back(task);
    queue.front()();
    EXPECT_EQ(s->replies, 1);
    s.reset();
    EXPECT_TRUE(observer.expired());
    for (auto& t : queue) t();   // no effect, no access to the freed session
    EXPECT_TRUE(observer.expired());
}

// "Give every borrow a lifetime argument": capturing a shared_ptr keeps the object alive
// until the callback has run (and so extends its lifetime).
TEST(SecurityOwnership, SharedPtrCaptureExtendsLifetime) {
    std::weak_ptr<Session> observer;
    std::function<void()> task;
    {
        auto s = std::make_shared<Session>();
        observer = s;
        task = [s] { s->on_reply(); };
    }
    EXPECT_FALSE(observer.expired());   // the callback still owns it
    task();
    EXPECT_EQ(observer.lock()->replies, 1);
    task = nullptr;
    EXPECT_TRUE(observer.expired());
}

// "Make ownership explicit": enable_shared_from_this gives an object a shared_ptr to
// itself that shares the existing control block (not a second owner).
TEST(SecurityOwnership, SharedFromThisSharesOwnership) {
    struct Node : std::enable_shared_from_this<Node> {};
    auto a = std::make_shared<Node>();
    auto b = a->shared_from_this();
    EXPECT_EQ(a.use_count(), 2);
    EXPECT_EQ(a.get(), b.get());
}
