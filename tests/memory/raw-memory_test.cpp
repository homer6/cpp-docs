// Checks claims made by docs/examples/memory/raw-memory.md.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <version>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {
int live = 0;
struct Probe {
    std::string s;
    explicit Probe(std::string v = "d") : s(std::move(v)) { ++live; }
    Probe(const Probe& o) : s(o.s) { ++live; }
    ~Probe() { --live; }
};
}  // namespace

// "std::addressof": returns the true address even when operator& is overloaded.
TEST(RawMemory, AddressofBypassesOverloadedAmpersand) {
    struct Hijack {
        Hijack* operator&() const { return nullptr; }
    };
    Hijack h;
    EXPECT_EQ(&h, nullptr);
    EXPECT_NE(std::addressof(h), nullptr);
    EXPECT_EQ(static_cast<void*>(std::addressof(h)), static_cast<void*>(&reinterpret_cast<char&>(h)));
}

// "Constructing into raw storage": construct_at builds in place, destroy_at
// runs the destructor without freeing; construct_at works in constexpr.
TEST(RawMemory, ConstructAtDestroyAt) {
    live = 0;
    alignas(Probe) std::byte buf[sizeof(Probe)];
    Probe* p = std::construct_at(reinterpret_cast<Probe*>(buf), "x");
    EXPECT_EQ(static_cast<void*>(p), static_cast<void*>(buf));
    EXPECT_EQ(p->s, "x");
    EXPECT_EQ(live, 1);
    std::destroy_at(p);
    EXPECT_EQ(live, 0);

    constexpr int v = [] {
        std::allocator<int> a;
        int* q = a.allocate(1);
        std::construct_at(q, 9);
        int r = *q;
        std::destroy_at(q);
        a.deallocate(q, 1);
        return r;
    }();
    static_assert(v == 9);
}

// "Bulk uninitialized algorithms": copy/fill construct into raw memory; destroy() ends them.
TEST(RawMemory, UninitializedAlgorithms) {
    live = 0;
    Probe src[3]{Probe{"a"}, Probe{"b"}, Probe{"c"}};
    alignas(Probe) std::byte raw[3 * sizeof(Probe)];
    auto* out = reinterpret_cast<Probe*>(raw);
    std::uninitialized_copy(src, src + 3, out);
    EXPECT_EQ(live, 6);
    EXPECT_EQ(out[2].s, "c");
    std::destroy(out, out + 3);
    EXPECT_EQ(live, 3);

    std::uninitialized_fill(out, out + 3, Probe{"f"});
    EXPECT_EQ(out[1].s, "f");
    std::destroy(out, out + 3);
    EXPECT_EQ(live, 3);
}

// "Bulk uninitialized algorithms": if an element's constructor throws, the
// already-constructed ones are destroyed.
TEST(RawMemory, UninitializedCopyCleansUpOnThrow) {
    static int built = 0, alive_now = 0;
    struct Bomb {
        Bomb() = default;
        Bomb(const Bomb&) {
            if (++built == 3) throw 1;
            ++alive_now;
        }
        ~Bomb() { --alive_now; }
    };
    Bomb src[4];
    alive_now = 0;
    built = 0;
    alignas(Bomb) std::byte raw[4 * sizeof(Bomb)];
    EXPECT_THROW(std::uninitialized_copy(src, src + 4, reinterpret_cast<Bomb*>(raw)), int);
    EXPECT_EQ(alive_now, 0);  // the two that were built got destroyed
}

// "std::start_lifetime_as (C++23)": interprets a byte buffer as a trivially-copyable type.
TEST(RawMemory, StartLifetimeAs) {
    CPPDOCS_REQUIRE(__cpp_lib_start_lifetime_as);
#ifdef __cpp_lib_start_lifetime_as
    struct PacketHeader { std::uint16_t type, length; };
    alignas(PacketHeader) std::byte buffer[512]{};
    PacketHeader in{7, 20};
    std::memcpy(buffer, &in, sizeof in);
    auto* hdr = std::start_lifetime_as<PacketHeader>(buffer);
    EXPECT_EQ(hdr->type, 7);
    EXPECT_EQ(hdr->length, 20);
#endif
}
