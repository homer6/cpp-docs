#include <string>
#include <vector>

std::string join(const std::vector<std::string>& parts) {
    std::string out;
    for (const auto& p : parts) {
        if (!out.empty()) out += ", ";
        out += p;
    }
    return out;
}
