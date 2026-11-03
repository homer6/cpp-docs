#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <unistd.h>

// Runs as a setuid helper; path is supplied by the invoking user.
std::optional<std::string> read_if_allowed(const std::string& path) {
    if (access(path.c_str(), R_OK) != 0) return std::nullopt;   // may the real user read it?
    std::ifstream in{path};
    return std::string{std::istreambuf_iterator<char>{in}, {}};
}
