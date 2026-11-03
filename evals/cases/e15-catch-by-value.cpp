#include <cstdio>
#include <stdexcept>

void run(void (*task)()) {
    try {
        task();
    } catch (std::exception e) {
        std::puts(e.what());
    }
}
