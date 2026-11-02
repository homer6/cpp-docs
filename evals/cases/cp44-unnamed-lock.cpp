#include <mutex>

std::mutex m;
int hits = 0;

void hit() {
    std::lock_guard<std::mutex>{m};
    ++hits;
}
