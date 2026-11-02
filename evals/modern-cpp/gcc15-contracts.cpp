#include <cassert>
#include <cmath>

double safe_sqrt(double x) {
    assert(x >= 0 && "safe_sqrt: x must not be negative");
    return std::sqrt(x);
}
