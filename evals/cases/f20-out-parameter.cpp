#include <string>
#include <vector>

void split(const std::string& s, char sep, std::vector<std::string>& out) {
    out.clear();
    std::string cur;
    for (char c : s) {
        if (c == sep) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
}
