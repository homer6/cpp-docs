// Checks claims made by docs/examples/memory/shared_ptr.md.
#include <cstdio>
#include <latch>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace {
int alive = 0;
struct Counted {
    Counted() { ++alive; }
    ~Counted() { --alive; }
};
}  // namespace

// "Create with make_shared": copying bumps the strong count; reset() drops one owner.
TEST(SharedPtr, CopyBumpsCountResetDrops) {
    alive = 0;
    auto p = std::make_shared<Counted>();
    std::shared_ptr<Counted> q = p;
    EXPECT_EQ(p.use_count(), 2);
    q.reset();
    EXPECT_EQ(p.use_count(), 1);
    EXPECT_EQ(alive, 1);  // still alive via p
    p.reset();
    EXPECT_EQ(alive, 0);  // last owner gone → destroyed
}

// "Sharing into structures": the node lives until both the local and the vector are gone.
TEST(SharedPtr, VectorCoOwns) {
    alive = 0;
    std::vector<std::shared_ptr<Counted>> graph;
    {
        auto n = std::make_shared<Counted>();
        graph.push_back(n);
        EXPECT_EQ(n.use_count(), 2);
    }
    EXPECT_EQ(alive, 1);
    graph.clear();
    EXPECT_EQ(alive, 0);
}

// "Custom deleter / aliasing": the deleter runs even when the stored pointer is null.
TEST(SharedPtr, DeleterRunsOnNull) {
    static int calls = 0;
    calls = 0;
    { std::shared_ptr<std::FILE> f{static_cast<std::FILE*>(nullptr), [](std::FILE*) { ++calls; }}; }
    EXPECT_EQ(calls, 1);
}

// "Custom deleter / aliasing": the aliasing constructor shares ownership but
// points at the sub-object.
TEST(SharedPtr, AliasingConstructor) {
    struct Field { int v = 7; };
    struct Parent { Field field; };
    auto parent = std::make_shared<Parent>();
    std::shared_ptr<Field> fld{parent, &parent->field};
    EXPECT_EQ(fld.get(), &parent->field);
    EXPECT_EQ(parent.use_count(), 2);  // fld co-owns parent
    std::weak_ptr<Parent> w = parent;
    parent.reset();
    EXPECT_FALSE(w.expired());  // kept alive through fld
    EXPECT_EQ(fld->v, 7);
}

// "Gotchas": larger than a raw pointer / unique_ptr (pointer + control block pointer).
TEST(SharedPtr, LargerThanUniquePtr) {
    static_assert(sizeof(std::shared_ptr<int>) > sizeof(std::unique_ptr<int>));
    static_assert(sizeof(std::unique_ptr<int>) == sizeof(int*));
}

// "Mental model": the count is atomic, so copying/destroying from many threads is safe.
TEST(SharedPtr, ConcurrentCopiesKeepCountExact) {
    auto p = std::make_shared<int>(1);
    constexpr int kThreads = 4, kCopies = 1000;
    std::latch go{1};
    std::vector<std::thread> ts;
    for (int i = 0; i < kThreads; ++i)
        ts.emplace_back([&] {
            go.wait();
            for (int j = 0; j < kCopies; ++j) { auto c = p; (void)c; }
        });
    go.count_down();
    for (auto& t : ts) t.join();
    EXPECT_EQ(p.use_count(), 1);
}

// "Gotchas: cycles leak": two objects owning each other never reach count 0.
TEST(SharedPtr, CycleKeepsBothAlive) {
    struct Node {
        std::shared_ptr<Node> other;
        Counted c;
    };
    alive = 0;
    std::weak_ptr<Node> wa;
    {
        auto a = std::make_shared<Node>();
        auto b = std::make_shared<Node>();
        a->other = b;
        b->other = a;
        wa = a;
    }
    EXPECT_FALSE(wa.expired());  // leaked: nothing outside points at them
    EXPECT_EQ(alive, 2);
    wa.lock()->other->other.reset();  // break the cycle so the test doesn't leak
    EXPECT_EQ(alive, 0);
}

// "Gotchas": enable_shared_from_this hands out a pointer sharing the existing control block.
TEST(SharedPtr, SharedFromThisSharesControlBlock) {
    struct W : std::enable_shared_from_this<W> {};
    auto a = std::make_shared<W>();
    auto b = a->shared_from_this();
    EXPECT_EQ(a.use_count(), 2);
    EXPECT_FALSE(a.owner_before(b) || b.owner_before(a));  // same control block
}
