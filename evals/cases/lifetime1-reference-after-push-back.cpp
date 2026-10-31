#include <vector>

int first_after_grow(std::vector<int>& v) {
    v.push_back(1);
    int& first = v[0];
    v.push_back(2);            // may reallocate
    return first;              // possibly dangling
}
