// Checks claims made by docs/examples/language/exceptions/noexcept-and-safety.md.
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

// Declared only (external linkage, so no unused warnings); used in unevaluated operands.
namespace noexcept_safety_decls {
void cleanup() noexcept;
int compute() noexcept(false);
int compute_default();
}  // namespace noexcept_safety_decls

namespace {
using namespace noexcept_safety_decls;

template <class T>
void cond_swap(T& a, T& b) noexcept(std::is_nothrow_move_constructible_v<T>) {
    T t = std::move(a);
    a = std::move(b);
    b = std::move(t);
}

struct ThrowingMove {
    ThrowingMove() = default;
    ThrowingMove(ThrowingMove&&) {}
    ThrowingMove& operator=(ThrowingMove&&) { return *this; }
};

struct Gadget {  // the page's example, with bodies, counting copies
    static inline int copies = 0;
    Gadget() = default;
    Gadget(const Gadget&) { ++copies; }
    Gadget(Gadget&&) noexcept {}
    Gadget& operator=(Gadget&&) noexcept { return *this; }
};

struct Dtor { ~Dtor() {} };

void thrower() { throw std::runtime_error{"boom"}; }
void throw_through_noexcept() noexcept { thrower(); }  // the exception escapes a noexcept function

class Buffer {  // the page's copy-and-swap example
    std::vector<int> data_;
public:
    Buffer() = default;
    explicit Buffer(std::vector<int> d) : data_(std::move(d)) {}
    Buffer(const Buffer&) = default;
    Buffer(Buffer&&) noexcept = default;
    friend void swap(Buffer& a, Buffer& b) noexcept { a.data_.swap(b.data_); }
    Buffer& operator=(Buffer other) noexcept {
        swap(*this, other);
        return *this;
    }
    const std::vector<int>& data() const { return data_; }
};

// Copying throws, so `a = b` fails while making the by-value parameter.
struct Bomb {
    Bomb() = default;
    Bomb(const Bomb&) { throw std::runtime_error{"copy"}; }
};
}  // namespace

// "noexcept — this won't throw": plain, noexcept(false) (the default), and conditional.
TEST(NoexceptAndSafety, NoexceptSpecifiers) {
    static_assert(noexcept(cleanup()));
    static_assert(!noexcept(compute()));
    static_assert(!noexcept(compute_default()));
    int a = 1, b = 2;
    ThrowingMove x, y;
    static_assert(noexcept(cond_swap(a, b)));
    static_assert(!noexcept(cond_swap(x, y)));
    cond_swap(a, b);
    EXPECT_EQ(a, 2);
}

// "noexcept": a noexcept function that throws calls std::terminate.
TEST(NoexceptAndSafetyDeathTest, ThrowFromNoexceptTerminates) {
    EXPECT_DEATH(throw_through_noexcept(), "");
}

// "Why noexcept matters for moves": vector moves on reallocation only with a noexcept move.
TEST(NoexceptAndSafety, VectorMovesOnlyNoexceptMoves) {
    Gadget::copies = 0;
    std::vector<Gadget> v(3);
    v.reserve(v.capacity() + 1);
    EXPECT_EQ(Gadget::copies, 0);
    static_assert(std::is_nothrow_destructible_v<Dtor>);  // destructors are implicitly noexcept
}

// "Techniques": RAII releases the lock and the buffer when the body throws.
TEST(NoexceptAndSafety, RaiiReleasesOnUnwind) {
    std::mutex m;
    auto fill = [&m](std::size_t n) {
        std::lock_guard lk{m};
        auto buf = std::make_unique<char[]>(n);
        throw std::runtime_error{"after acquiring"};
    };
    EXPECT_THROW(fill(16), std::runtime_error);
    EXPECT_TRUE(m.try_lock());  // released during unwinding
    m.unlock();
}

// "Techniques": copy-and-swap assignment is noexcept and gives the strong guarantee.
TEST(NoexceptAndSafety, CopyAndSwapStrongGuarantee) {
    static_assert(std::is_nothrow_move_assignable_v<Buffer>);
    Buffer a{{1, 2}}, b{{3}};
    a = b;
    EXPECT_EQ(a.data(), (std::vector<int>{3}));

    // The copy happens before *this is touched, so a throwing copy leaves the target unchanged.
    std::vector<Bomb> target(2), source(5);
    auto assign = [&] { target = std::vector<Bomb>(source); };
    EXPECT_THROW(assign(), std::runtime_error);
    EXPECT_EQ(target.size(), 2u);
}

// "Gotchas": noexcept is part of a function's type (since C++17).
TEST(NoexceptAndSafety, NoexceptIsPartOfTheType) {
    static_assert(!std::is_same_v<void (*)() noexcept, void (*)()>);
    static_assert(std::is_convertible_v<void (*)() noexcept, void (*)()>);
    static_assert(!std::is_convertible_v<void (*)(), void (*)() noexcept>);
    static_assert(std::is_same_v<decltype(&cleanup), void (*)() noexcept>);
}
