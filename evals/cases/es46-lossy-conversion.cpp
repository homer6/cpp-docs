#include <vector>

int average(const std::vector<double>& xs) {
    double sum = 0;
    for (double x : xs) sum += x;
    int avg = sum / xs.size();
    return avg;
}
