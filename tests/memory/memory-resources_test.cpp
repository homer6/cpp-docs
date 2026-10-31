// Checks claims made by docs/examples/memory/memory-resources.md.
#include <cstddef>
#include <memory_resource>
#include <new>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {
// Counts calls that reach the upstream resource.
struct Counting : std::pmr::memory_resource {
    int allocs = 0;
    void* do_allocate(std::size_t n, std::size_t a) override {
        ++allocs;
        return std::pmr::new_delete_resource()->allocate(n, a);
    }
    void do_deallocate(void* p, std::size_t n, std::size_t a) override {
        std::pmr::new_delete_resource()->deallocate(p, n, a);
    }
    bool do_is_equal(const memory_resource& o) const noexcept override { return this == &o; }
};
}  // namespace

// "Mental model": every pmr::vector<int> is one type, whatever resource backs it.
TEST(MemoryResources, OneTypeForAllResources) {
    std::pmr::monotonic_buffer_resource arena;
    std::pmr::vector<int> a{&arena};
    std::pmr::vector<int> b;
    static_assert(std::is_same_v<decltype(a), decltype(b)>);
    EXPECT_EQ(a.get_allocator().resource(), &arena);
}

// "Example": the vector and string allocate out of the stack buffer, not the heap.
TEST(MemoryResources, MonotonicArenaUsesBuffer) {
    std::byte buffer[4096];
    Counting upstream;
    std::pmr::monotonic_buffer_resource arena{buffer, sizeof buffer, &upstream};
    std::pmr::vector<int> v{&arena};
    for (int i = 0; i < 100; ++i) v.push_back(i);
    std::pmr::string s{"hello, a string too long for SSO", &arena};
    auto* data = reinterpret_cast<std::byte*>(v.data());
    EXPECT_TRUE(data >= buffer && data < buffer + sizeof buffer);
    EXPECT_EQ(upstream.allocs, 0);  // everything fit in the buffer
}

// "Gotchas": monotonic never frees individually, and grows from upstream past the buffer.
TEST(MemoryResources, MonotonicGrowsPastBuffer) {
    std::byte buffer[64];
    Counting upstream;
    std::pmr::monotonic_buffer_resource arena{buffer, sizeof buffer, &upstream};
    void* p = arena.allocate(32);
    arena.deallocate(p, 32);         // no-op
    void* q = arena.allocate(32);
    EXPECT_NE(p, q);                 // the freed block isn't reused
    (void)arena.allocate(1024);
    EXPECT_GE(upstream.allocs, 1);
}

// "The standard resources": null_memory_resource always throws — caps a buffer.
TEST(MemoryResources, NullResourceThrows) {
    EXPECT_THROW((void)std::pmr::null_memory_resource()->allocate(1), std::bad_alloc);
    std::byte buffer[64];
    std::pmr::monotonic_buffer_resource arena{buffer, sizeof buffer,
                                              std::pmr::null_memory_resource()};
    std::pmr::vector<int> v{&arena};
    EXPECT_THROW(v.resize(1000), std::bad_alloc);
}

// "Gotchas": without an explicit resource, pmr containers use the default,
// which starts out as new_delete_resource().
TEST(MemoryResources, DefaultIsNewDelete) {
    EXPECT_EQ(std::pmr::get_default_resource(), std::pmr::new_delete_resource());
    std::pmr::vector<int> v;
    EXPECT_EQ(v.get_allocator().resource(), std::pmr::new_delete_resource());
}

// "Gotchas": pmr::vector<T> and std::vector<T> are different, non-convertible types.
TEST(MemoryResources, PmrVectorIsNotStdVector) {
    static_assert(!std::is_same_v<std::pmr::vector<int>, std::vector<int>>);
    static_assert(!std::is_convertible_v<std::pmr::vector<int>, std::vector<int>>);
}

// "The standard resources": the pool resources hand out usable storage.
TEST(MemoryResources, PoolResourcesAllocate) {
    std::pmr::synchronized_pool_resource sync;
    std::pmr::unsynchronized_pool_resource unsync;
    std::pmr::vector<int> a{{1, 2, 3}, &sync};
    std::pmr::vector<int> b{{4, 5}, &unsync};
    EXPECT_EQ(a.size() + b.size(), 5u);
}
