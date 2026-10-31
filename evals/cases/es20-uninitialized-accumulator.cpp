#include <vector>

int sum(const std::vector<int>& v) {
    int total;                 // uninitialized
    for (int x : v) total += x;
    return total;
}
