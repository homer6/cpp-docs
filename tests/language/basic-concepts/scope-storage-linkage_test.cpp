// Checks claims made by docs/examples/language/basic-concepts/scope-storage-linkage.md.
#include <memory>
#include <thread>
#include <type_traits>

#include <gtest/gtest.h>

namespace {
int compute() { return 7; }

int g = 1;                       // namespace scope

int counter() {
    static int calls = 0;        // initialized once, persists across calls
    return ++calls;
}

thread_local int per_thread = 0; // one instance per thread

int hidden = 3;                  // anonymous namespace → internal linkage
}  // namespace

namespace scope_storage_linkage_test {  // named, so it keeps external linkage without clashing
inline int shared_inline = 42;   // inline variable (C++17): one definition across TUs
}  // namespace scope_storage_linkage_test

// "Scope": block, for-loop and if-init names end with their statement; inner names shadow outer ones.
TEST(ScopeStorageLinkage, ScopeAndShadowing) {
    int i = 100;
    int sum = 0;
    for (int i = 0; i < 10; ++i) sum += i;   // this i is scoped to the loop
    EXPECT_EQ(sum, 45);
    EXPECT_EQ(i, 100);
    int seen = 0;
    if (int n = compute(); n > 0) seen = n;  // n scoped to the if (C++17 init-statement)
    EXPECT_EQ(seen, 7);
    EXPECT_EQ(g, 1);
    {
        int g = 2;                           // shadows the namespace-scope g
        EXPECT_EQ(g, 2);
    }
    EXPECT_EQ(g, 1);
}

// "Storage duration": static locals persist across calls; thread_local is per thread; dynamic until freed.
TEST(ScopeStorageLinkage, StorageDurations) {
    const int first = counter();
    EXPECT_GE(first, 1);
    EXPECT_EQ(counter(), first + 1);
    EXPECT_EQ(counter(), first + 2);

    per_thread = 10;
    int other_initial = -1;
    std::thread t([&] {
        other_initial = per_thread;   // a fresh instance in the new thread
        per_thread = 99;
    });
    t.join();
    EXPECT_EQ(other_initial, 0);
    EXPECT_EQ(per_thread, 10);        // this thread's copy is untouched

    std::weak_ptr<int> w;
    {
        auto p = std::make_shared<int>(5);
        w = p;
        EXPECT_FALSE(w.expired());
    }
    EXPECT_TRUE(w.expired());         // dynamic object freed when its owner released it
}

// "Linkage": internal and inline names are usable here; "one per TU" vs "one shared" needs several TUs.
TEST(ScopeStorageLinkage, Linkage) {
    EXPECT_EQ(hidden, 3);
    EXPECT_EQ(scope_storage_linkage_test::shared_inline, 42);
    GTEST_SKIP() << "internal vs external linkage and the ODR are only observable across translation units";
}
