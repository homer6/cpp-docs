// Checks claims made by docs/security/verification.md.
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "parsers.hpp"

namespace {

std::string corpus_input(std::string_view target, std::string_view name) {
    const auto path = std::filesystem::path{CPPDOCS_FUZZ_DIR} / "corpus" / target / name;
    std::ifstream in{path, std::ios::binary};
    EXPECT_TRUE(in) << "missing corpus input " << path;
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

// The payload targets' input layout: header size, claimed length, then the packet.
struct PayloadInput {
    std::size_t header_size;
    std::size_t claimed_len;
    std::vector<std::byte> packet;
};

PayloadInput split(std::string_view raw) {
    EXPECT_GE(raw.size(), 2u);
    PayloadInput p{static_cast<unsigned char>(raw[0]), static_cast<unsigned char>(raw[1]), {}};
    for (char c : raw.substr(2)) p.packet.push_back(static_cast<std::byte>(c));
    return p;
}

}  // namespace

// "Measure what the fuzzing reached": every rejection in parse_port has a saved
// input that takes it, so the replay test exercises each one on every platform.
TEST(SecurityVerification, ParsePortCorpusReachesEveryRejection) {
    using cppdocs::fuzz::parse_port;
    EXPECT_FALSE(parse_port(corpus_input("parse_port", "leading-zero")));  // the "05" fuzz finding
    EXPECT_FALSE(parse_port(corpus_input("parse_port", "overflow")));      // from_chars: out of range
    EXPECT_FALSE(parse_port(corpus_input("parse_port", "trailing")));      // trailing data
    EXPECT_FALSE(parse_port(corpus_input("parse_port", "zero")));          // below the domain
    EXPECT_FALSE(parse_port(corpus_input("parse_port", "above-max")));     // 65536: above the domain
    EXPECT_EQ(parse_port(corpus_input("parse_port", "max-65535")), 65535);
}

// "Measure what the fuzzing reached": coverage showed no saved input took the
// "claimed length too long" rejection; "too-long" now claims one byte more than
// the packet holds after its header, so it's that check (not the header check)
// that rejects it.
TEST(SecurityVerification, PayloadCorpusReachesEveryRejection) {
    using cppdocs::fuzz::payload;
    const auto past_end = split(corpus_input("payload", "header-past-end"));
    EXPECT_GT(past_end.header_size, past_end.packet.size());
    EXPECT_FALSE(payload(past_end.packet, past_end.header_size, past_end.claimed_len));

    const auto too_long = split(corpus_input("payload", "too-long"));
    ASSERT_LE(too_long.header_size, too_long.packet.size());
    EXPECT_EQ(too_long.claimed_len, too_long.packet.size() - too_long.header_size + 1);
    EXPECT_FALSE(payload(too_long.packet, too_long.header_size, too_long.claimed_len));

    const auto exact = split(corpus_input("payload", "exact"));
    EXPECT_TRUE(payload(exact.packet, exact.header_size, exact.claimed_len));
}

// "Choose the sanitizer for the property": unsigned arithmetic wraps by
// definition, so -fsanitize=undefined has nothing to report; catching an
// unintended wrap takes Clang's opt-in -fsanitize=unsigned-integer-overflow.
TEST(SecurityVerification, UnsignedWrapIsDefined) {
    static_assert(std::numeric_limits<unsigned>::max() + 1u == 0u);
    static_assert(static_cast<unsigned char>(300) == 44);   // conversion to unsigned: modulo 2^N
    SUCCEED();
}

// "Run the instrumented build": when the suite is configured with a sanitizer,
// the compiler really instrumented this file (the macro is the compiler's own).
TEST(SecurityVerification, SanitizedBuildIsInstrumented) {
#if defined(CPPDOCS_SANITIZE)
    const std::string_view requested = CPPDOCS_SANITIZE;
    if (requested.find("address") == std::string_view::npos) GTEST_SKIP() << "no ASan in " << requested;
#  if defined(__SANITIZE_ADDRESS__)
    SUCCEED();
#  elif defined(__has_feature)
#    if __has_feature(address_sanitizer)
    SUCCEED();
#    else
    ADD_FAILURE() << "CPPDOCS_SANITIZE asks for ASan but this file isn't instrumented";
#    endif
#  else
    ADD_FAILURE() << "CPPDOCS_SANITIZE asks for ASan but this file isn't instrumented";
#  endif
#else
    GTEST_SKIP() << "not a sanitized build (CPPDOCS_SANITIZE is empty)";
#endif
}
