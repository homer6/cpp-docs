#include <algorithm>
#include <string>
#include <utility>
#include <vector>

struct Names {
    std::vector<std::pair<int, std::string>> entries;   // kept sorted by id

    const std::string* find(int id) const {
        auto it = std::ranges::lower_bound(entries, id, {}, &std::pair<int, std::string>::first);
        return it != entries.end() && it->first == id ? &it->second : nullptr;
    }
};
