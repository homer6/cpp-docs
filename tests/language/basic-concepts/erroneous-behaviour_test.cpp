// Checks claims made by docs/examples/language/basic-concepts/erroneous-behaviour.md.
#include <gtest/gtest.h>

namespace {
int static_int;  // static storage: zero-initialized

// The helpers use GNU inline asm, which MSVC (x64/ARM64) doesn't accept.
#if __has_cpp_attribute(indeterminate) && !(defined(_MSC_VER) && !defined(__clang__))
[[gnu::noinline]] void dirty_stack() {
    volatile unsigned char buf[256];
    for (auto& c : buf) c = 0xAB;
}
[[gnu::noinline]] void escape(void* p) { asm volatile("" : : "r"(p) : "memory"); }

[[gnu::noinline]] int read_uninitialized() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    int x;  // erroneous value
    escape(&x);
    return x;  // erroneous behaviour, deliberately
#pragma GCC diagnostic pop
}
#endif
}  // namespace

// "What gets an erroneous value": static storage is zero.
TEST(ErroneousBehaviour, StaticStorageIsZero) { EXPECT_EQ(static_int, 0); }

// "Compiler support": GCC 16 reads an uninitialized local as 0.
TEST(ErroneousBehaviour, Gcc16ReadsZero) {
#if __has_cpp_attribute(indeterminate) && defined(__GNUC__) && !defined(__clang__)
    dirty_stack();
    EXPECT_EQ(read_uninitialized(), 0);
#else
    GTEST_SKIP() << "needs a compiler that implements erroneous behaviour (GCC 16)";
#endif
}
