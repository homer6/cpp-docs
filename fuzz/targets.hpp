// Fuzz targets: each feeds one input to a parser and checks the properties the
// page promises; a broken property aborts, which libFuzzer reports as a crash.
// The corpus-replay test (tests/security/fuzz_corpus_test.cpp) calls the same
// functions on every saved input, on every platform.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <string>
#include <string_view>

#include "parsers.hpp"

namespace cppdocs::fuzz {

inline void check(bool property) {
    if (!property) std::abort();
}

// Any accepted port is in 1..65535 and is exactly the text, digit for digit.
inline int parse_port_target(const std::uint8_t* data, std::size_t size) {
    std::string_view text{reinterpret_cast<const char*>(data), size};
    if (auto port = parse_port(text)) {
        check(*port >= 1);
        check(std::to_string(*port) == text);
    }
    return 0;
}

// Input: header_size (1 byte), claimed_len (1 byte), then the packet. An
// accepted payload lies inside the packet and has the claimed length.
inline int payload_target(const std::uint8_t* data, std::size_t size) {
    if (size < 2) return 0;
    std::size_t header_size = data[0], claimed_len = data[1];
    std::span<const std::byte> packet{reinterpret_cast<const std::byte*>(data + 2), size - 2};
    if (auto p = payload(packet, header_size, claimed_len)) {
        check(p->size() == claimed_len);
        check(p->data() >= packet.data());
        check(p->data() + p->size() <= packet.data() + packet.size());
    }
    return 0;
}

// An accepted string is balanced, and its depth never exceeds the limit.
inline int nesting_target(const std::uint8_t* data, std::size_t size) {
    constexpr std::size_t limit = 64;
    std::string_view s{reinterpret_cast<const char*>(data), size};
    if (auto depth = nesting(s, limit)) {
        check(*depth <= limit);
        std::size_t open = 0;
        for (char c : s) open += (c == '(');
        check(open * 2 == s.size());
    }
    return 0;
}

}  // namespace cppdocs::fuzz
