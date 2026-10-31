// Checks claims made by docs/examples/language/basic-concepts/object-model.md.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
std::vector<std::string>& events() {
    static std::vector<std::string> log;  // constructed on first use, before the globals below
    return log;
}

struct Tracer {
    std::string name;
    explicit Tracer(std::string n) : name(std::move(n)) { events().push_back("ctor " + name); }
    ~Tracer() { events().push_back("dtor " + name); }
};

Tracer make(const char* n) { return Tracer(n); }

// Namespace-scope objects with static storage: constructed before main, in declaration order.
int construction_seq = 0;  // constant-initialized, so it's 0 before any dynamic initialization
struct Ordered { int seq = ++construction_seq; };
Ordered first_global;
Ordered second_global;

struct Noisy {
    const char* name;
    ~Noisy() { std::fprintf(stderr, "dtor %s\n", name); }
};

void on_exit_handler() { std::fprintf(stderr, "atexit ran\n"); }

struct Plain { int a, b; };
}  // namespace

// "Objects and lifetime": lifetime begins at construction and ends at scope exit (RAII).
TEST(ObjectModel, LifetimeEndsAtScopeExit) {
    events().clear();
    {
        Tracer s("s");
        EXPECT_EQ(events(), std::vector<std::string>{"ctor s"});
    }
    EXPECT_EQ(events(), (std::vector<std::string>{"ctor s", "dtor s"}));
}

// "Temporary objects & lifetime extension": a temporary bound to const& lives as long as the reference.
TEST(ObjectModel, TemporaryLifetimeExtension) {
    events().clear();
    {
        const Tracer& r = make("tmp");
        EXPECT_EQ(events(), std::vector<std::string>{"ctor tmp"});  // still alive here
        EXPECT_EQ(r.name, "tmp");
        Tracer&& rr = make("rref");                                 // "(or &&)"
        EXPECT_EQ(rr.name, "rref");
    }
    EXPECT_EQ(events(), (std::vector<std::string>{"ctor tmp", "ctor rref", "dtor rref", "dtor tmp"}));
    events().clear();
    (void)make("unbound").name.size();  // not bound to a reference: destroyed at the end of the full-expression
    EXPECT_EQ(events(), (std::vector<std::string>{"ctor unbound", "dtor unbound"}));
}

// "main": implicit return 0, argv[argc] == nullptr, no recursion/address.
TEST(ObjectModel, MainFunction) {
    GTEST_SKIP() << "gtest_main owns main(), so its implicit return and argv can't be observed here";
}

// "Startup & shutdown order": statics in one TU construct in declaration order before main and are
// destroyed in reverse after std::exit; abort skips destructors; atexit handlers run.
TEST(ObjectModel, StartupAndShutdownOrder) {
    EXPECT_EQ(first_global.seq, 1);
    EXPECT_EQ(second_global.seq, 2);
    EXPECT_EXIT(
        {
            static Noisy a{"a"};
            static Noisy b{"b"};
            std::atexit(on_exit_handler);  // registered after a and b: runs before their destructors
            std::exit(0);
        },
        ::testing::ExitedWithCode(0), "atexit ran\ndtor b\ndtor a");
    EXPECT_DEATH(
        {
            static Noisy q{"q"};
            std::fprintf(stderr, "before\n");
            std::abort();
        },
        "^before\n$");
}

// "Startup & shutdown order": std::quick_exit() skips normal destruction.
TEST(ObjectModel, QuickExitSkipsDestructors) {
#if defined(__APPLE__)
    GTEST_SKIP() << "macOS libc has no quick_exit (libc++ declares std::quick_exit only if ::quick_exit exists)";
#else
    EXPECT_EXIT(
        {
            static Noisy q{"q"};
            std::fprintf(stderr, "before\n");
            std::quick_exit(3);
        },
        ::testing::ExitedWithCode(3), "^before\n$");
#endif
}

// "Trivial vs non-trivial types": a struct of ints is trivially copyable (memcpy-able); std::string isn't.
TEST(ObjectModel, TriviallyCopyable) {
    static_assert(std::is_trivially_copyable_v<int> && std::is_trivially_copyable_v<Plain>);
    static_assert(!std::is_trivially_copyable_v<std::string>);
    Plain src{1, 2}, dst{};
    std::memcpy(&dst, &src, sizeof src);
    EXPECT_EQ(dst.a, 1);
    EXPECT_EQ(dst.b, 2);
}

// Tests added with the page sections from #56 (#69); kept in their own
// namespace so their helpers and test names can't clash with the ones above.
namespace from_pr69 {
namespace {
bool holder_destroyed = false;
struct Holder {
    std::vector<int> v;
    ~Holder() { holder_destroyed = true; }
    const std::vector<int>& items() const { return v; }
};
}  // namespace

// "Temporary objects & lifetime extension": binding through member access extends the whole temporary.
TEST(ObjectModel, MemberAccessExtendsTemporary) {
    const int& x = std::pair{1, 2}.first;
    EXPECT_EQ(x, 1);
}

// "Temporary objects & lifetime extension": C++23 range-for keeps every temporary in the range initializer alive.
TEST(ObjectModel, RangeForExtendsTemporaries) {
    CPPDOCS_REQUIRE_VERSION(__cpp_range_based_for, 202211L);
    holder_destroyed = false;
    int sum = 0;
    for (int x : Holder{{1, 2, 3}}.items()) {
        EXPECT_FALSE(holder_destroyed);
        sum += x;
    }
    EXPECT_TRUE(holder_destroyed);
    EXPECT_EQ(sum, 6);
}

// "Implicit-lifetime types": malloc implicitly creates an implicit-lifetime object.
TEST(ObjectModel, MallocCreatesImplicitLifetimeObject) {
    struct Point { int x, y; };
    auto* p = static_cast<Point*>(std::malloc(sizeof(Point)));
    ASSERT_NE(p, nullptr);
    p->x = 1;
    p->y = 2;
    EXPECT_EQ(p->x + p->y, 3);
    std::free(p);
}
}  // namespace from_pr69
