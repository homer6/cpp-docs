// Checks claims made by docs/examples/memory/allocators.md.
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <new>
#include <scoped_allocator>
#include <string>
#include <type_traits>
#include <vector>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

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

namespace {
template <class T>
struct Arena {
    using value_type = T;
    int id;
    explicit Arena(int i) : id(i) {}
    template <class U> Arena(const Arena<U>& o) noexcept : id(o.id) {}
    T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
    void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
    template <class U> bool operator==(const Arena<U>& o) const noexcept { return id == o.id; }
};
using ArenaStr = std::basic_string<char, std::char_traits<char>, Arena<char>>;

template <class T>
struct Minimal {
    using value_type = T;
    T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
    void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
};
}  // namespace

// "scoped_allocator_adaptor": the vector passes its allocator down to each string.
TEST(Allocators, ScopedAdaptorPropagatesToElements) {
    using Outer = std::scoped_allocator_adaptor<Arena<ArenaStr>>;
    std::vector<ArenaStr, Outer> v{Outer{Arena<ArenaStr>{7}}};
    v.emplace_back("a string long enough to allocate");
    EXPECT_EQ(v[0].get_allocator().id, 7);
}

// "scoped_allocator_adaptor": PMR containers pass their resource down by default.
TEST(Allocators, PmrPropagatesWithoutAdaptor) {
    std::pmr::monotonic_buffer_resource res;
    std::pmr::vector<std::pmr::string> v{&res};
    v.emplace_back("a string long enough to allocate");
    EXPECT_EQ(v[0].get_allocator().resource(), &res);
}

// "allocate_at_least (C++23)": count >= n; allocator_traits falls back to
// {allocate(n), n} for allocators without the member; no free function.
TEST(Allocators, AllocateAtLeast) {
    CPPDOCS_REQUIRE_VERSION(__cpp_lib_allocate_at_least, 202302L);
#ifdef __cpp_lib_allocate_at_least
    std::allocator<int> a;
    using Tr = std::allocator_traits<std::allocator<int>>;
    auto [p, count] = Tr::allocate_at_least(a, 5);
    EXPECT_GE(count, 5u);
    Tr::deallocate(a, p, count);

    Minimal<int> m;
    std::allocation_result<int*> r = std::allocator_traits<Minimal<int>>::allocate_at_least(m, 5);
    EXPECT_EQ(r.count, 5u);
    std::allocator_traits<Minimal<int>>::deallocate(m, r.ptr, r.count);
#endif
}
