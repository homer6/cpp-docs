#include <memory>

struct Point { double x = 0, y = 0; };

double length_squared(double x, double y) {
    auto p = std::make_unique<Point>();
    p->x = x;
    p->y = y;
    return p->x * p->x + p->y * p->y;
}
