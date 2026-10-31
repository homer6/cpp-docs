#include <cstdio>
#include <string>

bool first_line(const char* path, std::string& out) {
    std::FILE* f = std::fopen(path, "r");
    if (!f) return false;
    char buf[256] = {};
    if (!std::fgets(buf, sizeof buf, f)) {
        return false;          // leaks f
    }
    out = buf;
    std::fclose(f);
    return true;
}
