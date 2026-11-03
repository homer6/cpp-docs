// Checks claims made by docs/security/verification.md, "Fuzz the security
// boundaries": every saved fuzz input (fuzz/corpus/<target>/, seeds and
// minimized crashes) still satisfies its target's properties, on every
// platform, without needing libFuzzer (#307).
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "targets.hpp"

namespace {

using Target = int (*)(const std::uint8_t*, std::size_t);

std::size_t replay(const std::string& name, Target target) {
    std::filesystem::path dir = std::filesystem::path{CPPDOCS_FUZZ_DIR} / "corpus" / name;
    std::size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator{dir}) {
        if (!entry.is_regular_file()) continue;
        std::ifstream in{entry.path(), std::ios::binary};
        std::vector<char> bytes{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
        SCOPED_TRACE(entry.path().string());
        // A broken property aborts the test binary (as it would crash the fuzzer).
        target(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
        ++count;
    }
    return count;
}

}  // namespace

// "Fuzz the security boundaries": the parse_port corpus replays cleanly.
TEST(SecurityFuzzCorpus, ParsePort) {
    EXPECT_GE(replay("parse_port", cppdocs::fuzz::parse_port_target), 5u);
}

// "Fuzz the security boundaries": the payload corpus replays cleanly.
TEST(SecurityFuzzCorpus, Payload) {
    EXPECT_GE(replay("payload", cppdocs::fuzz::payload_target), 3u);
}

// "Fuzz the security boundaries": the nesting corpus replays cleanly.
TEST(SecurityFuzzCorpus, Nesting) {
    EXPECT_GE(replay("nesting", cppdocs::fuzz::nesting_target), 3u);
}

// "Fuzz the security boundaries": the targets' parsers give the answers the pages state.
TEST(SecurityFuzzCorpus, ParsersMatchThePages) {
    using namespace cppdocs::fuzz;
    EXPECT_EQ(parse_port("8080"), 8080);
    EXPECT_FALSE(parse_port("8080abc"));
    EXPECT_EQ(nesting("(()())", 8), 2u);
    EXPECT_FALSE(nesting(std::string(65, '(') + std::string(65, ')'), 64));
}
