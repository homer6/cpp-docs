// building.md "import std; and import std.compat;": a module unit that uses `import std;` (built only when CMake
// provides the std module; see modules.cmake).
module;
#include <climits>
export module stdlib_demo;
import std;

export std::string joined(const std::vector<std::string>& parts) {
    std::string out;
    for (const auto& p : parts) out += p;
    return out;
}
export int int_max() { return INT_MAX; }   // the macro came from the global module fragment
