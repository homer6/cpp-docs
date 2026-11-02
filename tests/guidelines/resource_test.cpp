// Checks claims made by docs/guidelines/core-guidelines/examples/resource.md.
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

// Counts live objects, so a test can see that every one was destroyed.
struct Tracked {
    static inline int live = 0;
    static inline int constructed = 0;
    int value = 0;
    Tracked() { ++live; ++constructed; }
    explicit Tracked(int v) : value{v} { ++live; ++constructed; }
    Tracked(const Tracked& o) : value{o.value} { ++live; ++constructed; }
    Tracked& operator=(const Tracked&) = default;
    ~Tracked() { --live; }
    static void reset() { live = 0; constructed = 0; }
};

// A fake C-style resource: acquire/release pairs, counted.
int open_handles = 0;
int acquire() { return ++open_handles; }
void release(int) noexcept { --open_handles; }

// R.1: a handle that acquires in its constructor and releases in its destructor.
class Handle {
    int h_;
public:
    Handle() : h_{acquire()} {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle() { release(h_); }
};

int use_handle(int mode) {
    Handle h;
    if (mode == 1) return 1;                              // early return
    if (mode == 2) throw std::runtime_error("failed");    // exception
    return 0;
}

// R.2
void zero(std::span<int> values) {
    for (int& v : values) v = 0;
}

// R.3 / R.4
void inspect(const Tracked* t) { (void)t; }
void touch(Tracked& t) { ++t.value; }
std::unique_ptr<Tracked> make_owned(int v) { return std::make_unique<Tracked>(v); }

// R.10: a type that records whether its constructor ran.
struct Record {
    static inline int constructed = 0;
    static inline int destroyed = 0;
    std::string name;
    Record() { ++constructed; }
    ~Record() { ++destroyed; }
};

// R.13: Engine's own operator new remembers the allocation, so the test can
// see the leak and then free the bytes itself (no leak left for ASan).
struct Engine {
    static inline int allocations = 0;
    static inline void* last = nullptr;
    static inline bool constructed = false;
    Engine() { constructed = true; }
    static void* operator new(std::size_t n) {
        ++allocations;
        last = ::operator new(n);
        return last;
    }
    static void operator delete(void* p) noexcept {
        --allocations;
        ::operator delete(p);
    }
};
struct Wheel {
    Wheel() { throw std::runtime_error("no wheel"); }
};
using Parts = std::pair<std::unique_ptr<Engine>, std::unique_ptr<Wheel>>;

// R.14: an array parameter is a pointer parameter.
void takes_array(int[10]) {}
void takes_pointer(int*) {}

// R.15: a class with matched allocation functions; both are used.
struct Message {
    static inline int news = 0;
    static inline int deletes = 0;
    static void* operator new(std::size_t n) { ++news; return ::operator new(n); }
    static void operator delete(void* p) noexcept { ++deletes; ::operator delete(p); }
    int payload = 0;
};

// R.22: make_shared constructs with ::new, so a class-specific operator new isn't used.
struct Session {
    static inline int class_news = 0;
    int id;
    explicit Session(int i) : id{i} {}
    static void* operator new(std::size_t n) { ++class_news; return ::operator new(n); }
    static void operator delete(void* p) noexcept { ::operator delete(p); }
};

// R.24
struct Child;
struct Parent : Tracked { std::shared_ptr<Child> child; };
struct Child : Tracked { std::weak_ptr<Parent> parent; };
struct CycleA;
struct CycleB : Tracked { std::shared_ptr<CycleA> a; };
struct CycleA : Tracked { std::shared_ptr<CycleB> b; };

// R.30
struct Shape { double area() const { return 2.0; } };
double area_of(const Shape& s) { return s.area(); }
double area_of_owned(const std::unique_ptr<Shape>& s) { return s->area(); }

// R.31
template <class T>
class SmartHandle {
    T* p_ = nullptr;
public:
    SmartHandle() = default;
    explicit SmartHandle(T* p) noexcept : p_{p} {}
    SmartHandle(const SmartHandle&) = delete;
    SmartHandle& operator=(const SmartHandle&) = delete;
    SmartHandle(SmartHandle&& o) noexcept : p_{std::exchange(o.p_, nullptr)} {}
    SmartHandle& operator=(SmartHandle&& o) noexcept {
        if (this != &o) { delete p_; p_ = std::exchange(o.p_, nullptr); }
        return *this;
    }
    ~SmartHandle() { delete p_; }
    T& operator*() const { return *p_; }
    T* operator->() const { return p_; }
    explicit operator bool() const noexcept { return p_ != nullptr; }
};

// R.32
std::vector<std::unique_ptr<Tracked>> queue;
void enqueue(std::unique_ptr<Tracked> job) { queue.push_back(std::move(job)); }
void consume(std::unique_ptr<Tracked> job) { (void)job; }   // destroys it on return

// R.33
void reload(std::unique_ptr<Tracked>& c) { c = std::make_unique<Tracked>(2); }

// R.34
struct Sprite {
    std::shared_ptr<Tracked> tex;
    explicit Sprite(std::shared_ptr<Tracked> t) : tex{std::move(t)} {}
};

// R.35
void reseat_copy(std::shared_ptr<Tracked> t) { t = std::make_shared<Tracked>(2); }
void reseat_ref(std::shared_ptr<Tracked>& t) { t = std::make_shared<Tracked>(2); }

// R.36
std::vector<std::shared_ptr<Tracked>> kept;
long count_seen_by_value = 0;
long count_seen_by_cref = 0;
void maybe_keep_by_value(std::shared_ptr<Tracked> e) {
    count_seen_by_value = e.use_count();
    if (e->value > 0) kept.push_back(e);
}
void maybe_keep(const std::shared_ptr<Tracked>& e) {
    count_seen_by_cref = e.use_count();
    if (e->value > 0) kept.push_back(e);
}

// R.37
std::shared_ptr<Tracked> current;
int pages_seen = -1;
void print(const Tracked& d) {
    current.reset();
    pages_seen = Tracked::live;   // still 1 only if the caller pinned the object
    (void)d;
}

}  // namespace

// R.1: an RAII handle releases its resource on normal exit, early return and exception.
TEST(CoreGuidelinesR, R1_RaiiReleasesOnEveryPath) {
    open_handles = 0;
    EXPECT_EQ(use_handle(0), 0);
    EXPECT_EQ(open_handles, 0);
    EXPECT_EQ(use_handle(1), 1);
    EXPECT_EQ(open_handles, 0);
    EXPECT_THROW(use_handle(2), std::runtime_error);
    EXPECT_EQ(open_handles, 0);
    static_assert(!std::is_copy_constructible_v<Handle>);

    std::mutex m;
    auto locked_elsewhere = [&m] {
        bool got = false;
        std::thread t{[&] { got = m.try_lock(); if (got) m.unlock(); }};
        t.join();
        return got;
    };
    {
        std::lock_guard lock{m};
        EXPECT_FALSE(locked_elsewhere());   // held while the guard lives
    }
    EXPECT_TRUE(m.try_lock());   // the guard unlocked it
    m.unlock();
}

// R.2: a span carries its size, so a function sees exactly the caller's elements.
TEST(CoreGuidelinesR, R2_SpanCarriesTheSize) {
    int buffer[4] = {1, 2, 3, 4};
    std::span<int> s = buffer;
    EXPECT_EQ(s.size(), 4u);
    zero(buffer);
    for (int v : buffer) EXPECT_EQ(v, 0);
}

// R.3: passing a raw pointer doesn't transfer ownership; a unique_ptr owner frees the object.
TEST(CoreGuidelinesR, R3_RawPointerDoesNotOwn) {
    Tracked::reset();
    {
        auto owned = make_owned(1);
        inspect(owned.get());
        EXPECT_EQ(Tracked::live, 1);   // inspect didn't destroy it
    }
    EXPECT_EQ(Tracked::live, 0);       // the owner did
}

// R.4: a reference borrows: binding and passing it neither copies nor destroys the object.
TEST(CoreGuidelinesR, R4_ReferenceDoesNotOwn) {
    Tracked::reset();
    Tracked t{1};
    {
        Tracked& r = t;
        touch(r);
    }
    EXPECT_EQ(t.value, 2);
    EXPECT_EQ(Tracked::constructed, 1);
    EXPECT_EQ(Tracked::live, 1);
}

// R.5: a scoped object is destroyed when its scope ends, even when an exception leaves it.
TEST(CoreGuidelinesR, R5_ScopedObjectsAreDestroyed) {
    Tracked::reset();
    try {
        Tracked g{1};
        EXPECT_EQ(Tracked::live, 1);
        throw std::runtime_error("x");
    } catch (const std::runtime_error&) {
    }
    EXPECT_EQ(Tracked::live, 0);
}

// R.10: malloc/free run no constructor or destructor; make_unique runs both.
TEST(CoreGuidelinesR, R10_MallocDoesNotConstruct) {
    Record::constructed = Record::destroyed = 0;
    void* raw = std::malloc(sizeof(Record));
    ASSERT_NE(raw, nullptr);
    std::free(raw);
    EXPECT_EQ(Record::constructed, 0);
    EXPECT_EQ(Record::destroyed, 0);
    {
        auto r = std::make_unique<Record>();
        r->name = "x";
        EXPECT_EQ(Record::constructed, 1);
    }
    EXPECT_EQ(Record::destroyed, 1);
}

// R.11: an object owned by make_unique is freed even when the code using it throws.
TEST(CoreGuidelinesR, R11_NoNakedNewOrDelete) {
    Tracked::reset();
    auto draw = [] {
        auto s = std::make_unique<Tracked>();
        throw std::runtime_error("draw failed");
    };
    EXPECT_THROW(draw(), std::runtime_error);
    EXPECT_EQ(Tracked::constructed, 1);
    EXPECT_EQ(Tracked::live, 0);
}

// R.12: a resource handed to its owner at once is released when the next step throws.
TEST(CoreGuidelinesR, R12_OwnTheResourceImmediately) {
    open_handles = 0;
    auto step = [] {
        Handle h;                                     // owned at once
        throw std::length_error("buffer too big");    // the next step fails
    };
    EXPECT_THROW(step(), std::length_error);
    EXPECT_EQ(open_handles, 0);
}

// R.13: two raw news in one expression leak the first when the second throws; make_unique doesn't.
TEST(CoreGuidelinesR, R13_OneAllocationPerStatement) {
    Engine::allocations = 0;
    Engine::constructed = false;
    EXPECT_THROW((Parts{new Engine, new Wheel}), std::runtime_error);
    // Braced-list elements are evaluated left to right, so the Engine was made first and leaked.
    EXPECT_TRUE(Engine::constructed);
    EXPECT_EQ(Engine::allocations, 1);
    ::operator delete(Engine::last);   // free the leaked bytes ourselves
    Engine::allocations = 0;

    Engine::constructed = false;
    EXPECT_THROW((Parts{std::make_unique<Engine>(), std::make_unique<Wheel>()}), std::runtime_error);
    EXPECT_TRUE(Engine::constructed);
    EXPECT_EQ(Engine::allocations, 0);   // the Engine's unique_ptr freed it
}

// R.14: an array parameter is adjusted to a pointer (its size is lost); a span keeps it.
TEST(CoreGuidelinesR, R14_ArrayParameterIsAPointer) {
    static_assert(std::is_same_v<decltype(takes_array), decltype(takes_pointer)>);
    static_assert(std::is_same_v<decltype(takes_array), void(int*)>);
    const int a[10] = {};
    std::span<const int> s = a;
    EXPECT_EQ(s.size(), 10u);
    takes_array(nullptr);
    takes_pointer(nullptr);
}

// R.15: new and delete on a class use its matched pair of allocation functions.
TEST(CoreGuidelinesR, R15_MatchedAllocationPairs) {
    Message::news = Message::deletes = 0;
    Message* m = new Message;
    delete m;
    EXPECT_EQ(Message::news, 1);
    EXPECT_EQ(Message::deletes, 1);
}

// R.20: unique_ptr and shared_ptr destroy their object when the last owner goes away.
TEST(CoreGuidelinesR, R20_SmartPointersOwn) {
    Tracked::reset();
    {
        auto u = std::make_unique<Tracked>();
        auto s = std::make_shared<Tracked>();
        auto s2 = s;
        EXPECT_EQ(Tracked::live, 2);
        EXPECT_EQ(s.use_count(), 2);
    }
    EXPECT_EQ(Tracked::live, 0);
}

// R.21: unique_ptr can't be copied, only moved, and needs no reference count.
TEST(CoreGuidelinesR, R21_UniquePtrIsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<std::unique_ptr<Tracked>>);
    static_assert(!std::is_copy_assignable_v<std::unique_ptr<Tracked>>);
    static_assert(std::is_nothrow_move_constructible_v<std::unique_ptr<Tracked>>);
    static_assert(std::is_constructible_v<std::shared_ptr<Tracked>, std::unique_ptr<Tracked>&&>);
    // Not guaranteed by the standard, but true of libstdc++, libc++ and the MS STL.
    EXPECT_EQ(sizeof(std::unique_ptr<Tracked>), sizeof(Tracked*));
    EXPECT_GT(sizeof(std::shared_ptr<Tracked>), sizeof(Tracked*));

    Tracked::reset();
    auto p = std::make_unique<Tracked>();
    auto q = std::move(p);
    EXPECT_EQ(p, nullptr);
    q.reset();
    EXPECT_EQ(Tracked::live, 0);   // destroyed exactly where we said
}

// R.22: make_shared makes one owner and constructs the object with ::new, not the class's operator new.
TEST(CoreGuidelinesR, R22_MakeShared) {
    Session::class_news = 0;
    auto s = std::make_shared<Session>(42);
    EXPECT_EQ(s->id, 42);
    EXPECT_EQ(s.use_count(), 1);
    EXPECT_EQ(Session::class_news, 0);   // a combined allocation from the library
    std::shared_ptr<Session> t{new Session{7}};
    EXPECT_EQ(Session::class_news, 1);   // the object's own allocation; the counts need another
}

// R.23: make_unique forwards its arguments to the constructor and returns the owner.
TEST(CoreGuidelinesR, R23_MakeUnique) {
    Tracked::reset();
    auto b = std::make_unique<Tracked>(64);
    static_assert(std::is_same_v<decltype(b), std::unique_ptr<Tracked>>);
    EXPECT_EQ(b->value, 64);
    b.reset();
    EXPECT_EQ(Tracked::live, 0);
}

// R.24: a shared_ptr cycle is never destroyed; a weak_ptr back link lets both objects go.
TEST(CoreGuidelinesR, R24_WeakPtrBreaksCycles) {
    Tracked::reset();
    std::weak_ptr<CycleA> watch;
    {
        auto a = std::make_shared<CycleA>();
        a->b = std::make_shared<CycleB>();
        a->b->a = a;
        watch = a;
    }
    EXPECT_EQ(Tracked::live, 2);      // the cycle kept both alive
    if (auto a = watch.lock()) a->b->a.reset();   // break it by hand so nothing leaks
    EXPECT_EQ(Tracked::live, 0);

    std::weak_ptr<Parent> parent_watch;
    {
        auto p = std::make_shared<Parent>();
        p->child = std::make_shared<Child>();
        p->child->parent = p;
        parent_watch = p;
        EXPECT_EQ(p.use_count(), 1);  // the weak link doesn't count
        EXPECT_NE(p->child->parent.lock(), nullptr);
    }
    EXPECT_EQ(Tracked::live, 0);      // both destroyed
    EXPECT_TRUE(parent_watch.expired());
    EXPECT_EQ(parent_watch.lock(), nullptr);
}

// R.30: a T& parameter accepts objects owned any way; a smart-pointer parameter rejects a local.
TEST(CoreGuidelinesR, R30_TakeReferencesNotSmartPointers) {
    Shape local;
    auto owned = std::make_unique<Shape>();
    auto shared = std::make_shared<Shape>();
    EXPECT_DOUBLE_EQ(area_of(local) + area_of(*owned) + area_of(*shared), 6.0);
    EXPECT_DOUBLE_EQ(area_of_owned(owned), 2.0);
    static_assert(!std::is_invocable_v<decltype(&area_of_owned), Shape&>);
    static_assert(!std::is_invocable_v<decltype(&area_of_owned), std::shared_ptr<Shape>&>);
}

// R.31: a custom owning handle shaped like unique_ptr is move-only and frees its object once.
TEST(CoreGuidelinesR, R31_CustomSmartPointerFollowsStd) {
    static_assert(!std::is_copy_constructible_v<SmartHandle<Tracked>>);
    static_assert(std::is_nothrow_move_constructible_v<SmartHandle<Tracked>>);
    Tracked::reset();
    {
        SmartHandle<Tracked> a{new Tracked{5}};
        EXPECT_EQ((*a).value, 5);
        SmartHandle<Tracked> b = std::move(a);
        EXPECT_FALSE(a);
        EXPECT_TRUE(b);
        EXPECT_EQ(b->value, 5);
        a = std::move(b);
        EXPECT_TRUE(a);
    }
    EXPECT_EQ(Tracked::live, 0);
}

// R.32: a by-value unique_ptr parameter takes ownership; it can't be called with an lvalue owner.
TEST(CoreGuidelinesR, R32_UniquePtrSink) {
    static_assert(!std::is_invocable_v<decltype(&enqueue), std::unique_ptr<Tracked>&>);
    static_assert(std::is_invocable_v<decltype(&enqueue), std::unique_ptr<Tracked>&&>);
    Tracked::reset();
    auto j = std::make_unique<Tracked>();
    enqueue(std::move(j));
    EXPECT_EQ(j, nullptr);
    EXPECT_EQ(queue.size(), 1u);
    EXPECT_EQ(Tracked::live, 1);
    queue.clear();
    EXPECT_EQ(Tracked::live, 0);

    consume(std::make_unique<Tracked>());
    EXPECT_EQ(Tracked::live, 0);   // the sink destroyed it
}

// R.33: a unique_ptr& parameter can reseat the caller's pointer; the old object is destroyed.
TEST(CoreGuidelinesR, R33_UniquePtrRefReseats) {
    Tracked::reset();
    auto c = std::make_unique<Tracked>(1);
    reload(c);
    EXPECT_EQ(c->value, 2);
    EXPECT_EQ(Tracked::constructed, 2);
    EXPECT_EQ(Tracked::live, 1);
}

// R.34: a by-value shared_ptr parameter makes the receiver one more owner; the object outlives the caller's pointer.
TEST(CoreGuidelinesR, R34_SharedPtrByValueShares) {
    Tracked::reset();
    auto t = std::make_shared<Tracked>();
    Sprite s{t};
    EXPECT_EQ(t.use_count(), 2);
    t.reset();
    EXPECT_EQ(Tracked::live, 1);
    EXPECT_EQ(s.tex.use_count(), 1);
}

// R.35: reseating a shared_ptr parameter taken by value changes only the copy; by reference, the caller's.
TEST(CoreGuidelinesR, R35_SharedPtrRefReseats) {
    Tracked::reset();
    auto t = std::make_shared<Tracked>(1);
    reseat_copy(t);
    EXPECT_EQ(t->value, 1);
    reseat_ref(t);
    EXPECT_EQ(t->value, 2);
    EXPECT_EQ(Tracked::live, 1);
}

// R.36: a const shared_ptr& parameter adds no count unless the function keeps a copy.
TEST(CoreGuidelinesR, R36_ConstSharedPtrRefMightRetain) {
    kept.clear();
    auto ignored = std::make_shared<Tracked>(0);
    maybe_keep(ignored);
    EXPECT_EQ(count_seen_by_cref, 1);   // no copy made for the call
    maybe_keep_by_value(ignored);
    EXPECT_EQ(count_seen_by_value, 2);  // by value always copies
    EXPECT_EQ(ignored.use_count(), 1);

    auto important = std::make_shared<Tracked>(1);
    maybe_keep(important);
    EXPECT_EQ(important.use_count(), 2);   // kept
    kept.clear();
}

// R.37: a local copy of an aliased shared_ptr keeps the object alive while the callee resets the original.
TEST(CoreGuidelinesR, R37_PinAliasedSmartPointers) {
    Tracked::reset();
    current = std::make_shared<Tracked>();
    {
        auto pin = current;
        print(*pin);
        EXPECT_EQ(pages_seen, 1);       // alive during the call
        EXPECT_EQ(current, nullptr);
    }
    EXPECT_EQ(Tracked::live, 0);        // freed once the pin goes
}
