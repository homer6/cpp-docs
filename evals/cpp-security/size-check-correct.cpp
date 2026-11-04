#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

// offset and length come from a packet header sent by the client.
std::optional<std::span<const std::byte>> field(std::span<const std::byte> packet,
                                                std::uint64_t offset, std::uint64_t length) {
    if (offset > packet.size()) return std::nullopt;
    if (length > packet.size() - offset) return std::nullopt;
    return packet.subspan(offset, length);
}
