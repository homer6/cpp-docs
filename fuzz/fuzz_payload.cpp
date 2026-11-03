// libFuzzer entry point for cppdocs::fuzz::payload_target (fuzz/targets.hpp).
#include <cstddef>
#include <cstdint>

#include "targets.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    return cppdocs::fuzz::payload_target(data, size);
}
