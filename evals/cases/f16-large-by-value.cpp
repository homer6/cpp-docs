#include <array>
#include <numeric>

double average(std::array<double, 4096> samples) {   // 32 KiB copied per call
    return std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
}
