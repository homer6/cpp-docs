// Checks claims made by docs/security/bounds.md.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace {

std::optional<std::span<const std::byte>>
payload(std::span<const std::byte> packet, std::size_t header_size, std::size_t claimed_len) {
    if (header_size > packet.size()) return std::nullopt;
    std::size_t available = packet.size() - header_size;
    if (claimed_len > available) return std::nullopt;
    return packet.subspan(header_size, claimed_len);
}

std::optional<int> lookup(const std::vector<int>& table, std::size_t index) {
    if (index >= table.size()) return std::nullopt;
    return table[index];
}

}  // namespace

// "Carry the extent": a length from the header is checked against what was received,
// including lengths and header sizes chosen to overflow a naive header + len check.
TEST(SecurityBounds, PayloadLengthCheckedAgainstPacket) {
    std::vector<std::byte> packet(10);
    auto p = payload(packet, 4, 6);
    ASSERT_TRUE(p);
    EXPECT_EQ(p->size(), 6u);
    EXPECT_EQ(p->data(), packet.data() + 4);
    EXPECT_FALSE(payload(packet, 4, 7));    // the header claims one byte too many
    EXPECT_FALSE(payload(packet, 11, 0));   // header larger than the packet
    constexpr auto big = std::numeric_limits<std::size_t>::max();
    EXPECT_FALSE(payload(packet, 4, big));  // 4 + big would wrap to 3
    EXPECT_TRUE(payload(packet, 10, 0));    // empty payload at the end is valid
}

// "Check before the access": an explicit check makes a bad index an ordinary outcome.
TEST(SecurityBounds, ExplicitCheckRejectsBadIndex) {
    std::vector<int> table{10, 20, 30, 40};
    EXPECT_EQ(lookup(table, 3), 40);
    EXPECT_FALSE(lookup(table, 4));
    EXPECT_FALSE(lookup(table, std::numeric_limits<std::size_t>::max()));
}

// "Check before the access": at() checks too, but reports a bad index by throwing.
TEST(SecurityBounds, AtThrowsOutOfRange) {
    std::vector<int> table(4);
    EXPECT_THROW((void)table.at(4), std::out_of_range);
}

// "Respect invalidation": push_back past capacity reallocates, so pointers and references
// taken before it no longer point into the vector.
TEST(SecurityBounds, PushBackPastCapacityReallocates) {
    std::vector<int> v;
    v.reserve(1);
    v.push_back(1);
    const auto before = reinterpret_cast<std::uintptr_t>(v.data());
    const auto cap = v.capacity();
    while (v.size() < cap) v.push_back(0);
    v.push_back(2);                         // one past capacity
    EXPECT_GT(v.capacity(), cap);
    EXPECT_NE(reinterpret_cast<std::uintptr_t>(v.data()), before);
}

// "Respect invalidation": reserve allocates capacity but creates no elements.
TEST(SecurityBounds, ReserveDoesNotCreateElements) {
    std::vector<int> v;
    v.reserve(16);
    EXPECT_GE(v.capacity(), 16u);
    EXPECT_EQ(v.size(), 0u);
    EXPECT_FALSE(lookup(v, 0));
}
