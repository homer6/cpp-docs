#include <map>
#include <mutex>
#include <stdexcept>
#include <string>

std::mutex m;
std::map<std::string, int> counts;

void add(const std::string& key) {
    m.lock();
    if (key.empty()) throw std::invalid_argument("empty key");
    ++counts[key];
    m.unlock();
}
