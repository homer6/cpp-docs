// Checks claims made by docs/examples/memory/allocators.md.
#include <cstddef>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {
int allocations = 0;

// The page's minimal allocator, instrumented to count allocate() calls.
template <class T>
struct MyAlloc {
    using value_type = T;
    MyAlloc() = default;
    template <class U> MyAlloc(const MyAlloc<U>&) noexcept {}
    T* allocate(std::size_t n) {
        ++allocations;
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }
    void deallocate(T* p, std::size_t) { ::operator delete(p); }
    template <class U> bool operator==(const MyAlloc<U>&) const noexcept { return true; }
};
}  // namespace

// "Mental model": containers default their trailing Allocator parameter to std::allocator<T>.
TEST(Allocators, DefaultIsStdAllocator) {
    static_assert(std::is_same_v<std::vector<int>::allocator_type, std::allocator<int>>);
    static_assert(std::is_same_v<std::string::allocator_type, std::allocator<char>>);
}

// "allocator_traits": a minimal allocator gets construct, max_size, rebind and
// the propagation flags from the traits.
TEST(Allocators, TraitsSupplyDefaults) {
    using Tr = std::allocator_traits<MyAlloc<int>>;
    static_assert(std::is_same_v<Tr::pointer, int*>);
    static_assert(std::is_same_v<Tr::rebind_alloc<double>, MyAlloc<double>>);
    static_assert(!Tr::propagate_on_container_copy_assignment::value);
    static_assert(Tr::is_always_equal::value);  // empty class
    MyAlloc<int> a;
    EXPECT_GT(Tr::max_size(a), 0u);

    // make_one from the page: allocate, construct, destroy, deallocate.
    auto p = Tr::allocate(a, 1);
    Tr::construct(a, p, 42);
    EXPECT_EQ(*p, 42);
    Tr::destroy(a, p);
    Tr::deallocate(a, p, 1);
}

// "A minimal C++11+ allocator": a vector really allocates through it.
TEST(Allocators, VectorUsesCustomAllocator) {
    allocations = 0;
    std::vector<int, MyAlloc<int>> v;
    v.push_back(1);
    EXPECT_GE(allocations, 1);
}

// "Gotchas": the allocator type is part of the container type.
TEST(Allocators, AllocatorIsPartOfType) {
    static_assert(!std::is_same_v<std::vector<int>, std::vector<int, MyAlloc<int>>>);
    static_assert(!std::is_convertible_v<std::vector<int, MyAlloc<int>>, std::vector<int>>);
}
