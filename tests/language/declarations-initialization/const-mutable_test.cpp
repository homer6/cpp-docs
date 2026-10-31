// Checks claims made by docs/examples/language/declarations-initialization/const-mutable.md.
#include <mutex>
#include <optional>
#include <type_traits>

#include <gtest/gtest.h>

namespace {
struct Result { int value; };
int compute_calls = 0;
Result compute() { ++compute_calls; return Result{42}; }

class Cache {
    mutable std::mutex m_;
    mutable std::optional<Result> cached_;
public:
    Result get() const {
        std::lock_guard lk{m_};
        if (!cached_) cached_ = compute();
        return *cached_;
    }
};

struct Counter {
    int n = 3;
    int value() const { return n; }
    void bump() { ++n; }
};

template <class T> concept CanBump = requires(T& t) { t.bump(); };
template <class T> concept CanValue = requires(T& t) { t.value(); };

int read() { return 7; }
}  // namespace

// "const": which part of each pointer declaration is const (read right-to-left).
TEST(ConstMutable, PointerConstness) {
    int x = 0;
    const int* p1 = &x;
    int* const p2 = &x;
    const int* const p3 = &x;
    static_assert(std::is_const_v<std::remove_pointer_t<decltype(p1)>> && !std::is_const_v<decltype(p1)>);
    static_assert(!std::is_const_v<std::remove_pointer_t<decltype(p2)>> && std::is_const_v<decltype(p2)>);
    static_assert(std::is_const_v<std::remove_pointer_t<decltype(p3)>> && std::is_const_v<decltype(p3)>);
    *p2 = 5;  // const pointer, mutable pointee
    EXPECT_EQ(*p1 + *p3, 10);
}

// "const-correctness": a const member function is callable on const objects; non-const ones aren't.
TEST(ConstMutable, ConstMemberFunctions) {
    const Counter c;
    EXPECT_EQ(c.value(), 3);
    static_assert(CanValue<const Counter>);
    static_assert(!CanBump<const Counter>);
    static_assert(CanBump<Counter>);
}

// "mutable": a const get() fills the cache once and reuses it.
TEST(ConstMutable, MutableCache) {
    compute_calls = 0;
    const Cache cache;
    EXPECT_EQ(cache.get().value, 42);
    EXPECT_EQ(cache.get().value, 42);
    EXPECT_EQ(compute_calls, 1);
}

// "constexpr / constinit" + Gotchas: constexpr implies const; runtime-initialized const isn't a constant.
TEST(ConstMutable, ConstexprVersusConst) {
    const int a = read();
    constexpr int b = 5 * 5;
    static_assert(b == 25);
    static_assert(std::is_same_v<decltype(b), const int>);  // constexpr implies const
    EXPECT_EQ(a, 7);
    // auto drops top-level const.
    auto copy = b;
    static_assert(std::is_same_v<decltype(copy), int>);
    (void)copy;
}

// "volatile": volatile qualifies the pointee, and it is not atomic.
TEST(ConstMutable, VolatileQualifier) {
    volatile int reg = 1;
    volatile int* hardware_reg = &reg;
    static_assert(std::is_volatile_v<std::remove_pointer_t<decltype(hardware_reg)>>);
    *hardware_reg = 2;
    EXPECT_EQ(reg, 2);
}
