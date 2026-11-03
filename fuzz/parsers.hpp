// The parsers from docs/security/parsing-and-validation.md and bounds.md,
// shared by the fuzz targets and the corpus-replay test, so the fuzzer runs
// the same code the pages show and the tests check (#307).
#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <system_error>

namespace cppdocs::fuzz {

// parsing-and-validation.md, "Parse into constrained values".
inline std::optional<std::uint16_t> parse_port(std::string_view text) {
    if (text.size() > 1 && text.front() == '0') return std::nullopt;   // "080" and "80" mustn't both mean 80
    unsigned value = 0;
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{}) return std::nullopt;
    if (end != text.data() + text.size()) return std::nullopt;
    if (value == 0 || value > 65535) return std::nullopt;
    return static_cast<std::uint16_t>(value);
}

// bounds.md, "Carry the extent".
inline std::optional<std::span<const std::byte>>
payload(std::span<const std::byte> packet, std::size_t header_size, std::size_t claimed_len) {
    if (header_size > packet.size()) return std::nullopt;
    std::size_t available = packet.size() - header_size;
    if (claimed_len > available) return std::nullopt;
    return packet.subspan(header_size, claimed_len);
}

// parsing-and-validation.md, "Bound the parser's work".
inline std::optional<std::size_t> nesting(std::string_view s, std::size_t max_depth) {
    std::size_t depth = 0, deepest = 0;
    for (char c : s) {
        if (c == '(') {
            if (++depth > max_depth) return std::nullopt;
            if (depth > deepest) deepest = depth;
        } else if (c == ')') {
            if (depth == 0) return std::nullopt;
            --depth;
        } else {
            return std::nullopt;
        }
    }
    if (depth != 0) return std::nullopt;
    return deepest;
}

}  // namespace cppdocs::fuzz
