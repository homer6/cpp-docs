// Checks claims made by docs/examples/concurrency/safe-reclamation.md.
//
// No local toolchain ships <rcu> or <hazard_pointer> (GCC 15.2, GCC 16.2,
// Clang 22 + libc++), so the behaviour tests skip until one does.
#include <atomic>
#include <memory>
#include <version>

#ifdef __cpp_lib_rcu
#include <mutex>
#include <rcu>
#endif
#ifdef __cpp_lib_hazard_pointer
#include <hazard_pointer>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

// "Compiler support": neither macro is defined by the toolchains in the table.
TEST(SafeReclamation, CompilerSupportTable) {
#if (defined(__GNUC__) && !defined(__clang__) && (__GNUC__ == 15 || __GNUC__ == 16)) || \
    (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION / 10000 == 22)
#if defined(__cpp_lib_rcu) || defined(__cpp_lib_hazard_pointer)
    FAIL() << "the page's table says this toolchain has neither; update it";
#endif
#else
    GTEST_SKIP() << "table covers only GCC 15/16 and libc++ 22";
#endif
}

#ifdef __cpp_lib_rcu
namespace {
int rcu_deleted = 0;
struct Config : std::rcu_obj_base<Config> {
    explicit Config(int l) : limit(l) {}
    ~Config() { ++rcu_deleted; }
    int limit;
};
}  // namespace
#endif

// "RCU: a read-mostly configuration": a retired object outlives the reader
// that can still see it; rcu_barrier runs the scheduled deletion.
TEST(SafeReclamation, RcuRetireWaitsForReaders) {
    CPPDOCS_REQUIRE(__cpp_lib_rcu);
#ifdef __cpp_lib_rcu
    rcu_deleted = 0;
    std::atomic<Config*> current{new Config(10)};
    {
        std::scoped_lock guard(std::rcu_default_domain());
        Config* c = current.load(std::memory_order_acquire);
        Config* old = current.exchange(new Config(20), std::memory_order_acq_rel);
        old->retire();
        EXPECT_EQ(c->limit, 10);  // still readable inside the section
    }
    std::rcu_synchronize();
    std::rcu_barrier();
    EXPECT_EQ(rcu_deleted, 1);
    delete current.load();
#endif
}

// "Hazard pointers": a default-constructed hazard_pointer is empty; protect()
// keeps a retired node alive until protection ends.
TEST(SafeReclamation, HazardPointerProtects) {
    CPPDOCS_REQUIRE(__cpp_lib_hazard_pointer);
#ifdef __cpp_lib_hazard_pointer
    static int deleted = 0;
    struct Node : std::hazard_pointer_obj_base<Node> {
        explicit Node(int v) : value(v) {}
        ~Node() { ++deleted; }
        int value;
    };
    EXPECT_TRUE(std::hazard_pointer{}.empty());
    std::atomic<Node*> head{new Node(1)};
    {
        std::hazard_pointer hp = std::make_hazard_pointer();
        EXPECT_FALSE(hp.empty());
        Node* p = hp.protect(head);
        head.exchange(new Node(2))->retire();
        EXPECT_EQ(p->value, 1);  // protected: not reclaimed
    }
    delete head.load();
#endif
}
