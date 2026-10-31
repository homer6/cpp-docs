// modules.md "Export forms".
module;
#include <cmath>
export module shapes;

export struct Point { double x, y; };

export double length(Point p) { return std::sqrt(p.x * p.x + p.y * p.y); }

export {
    constexpr double pi = 3.141592653589793;
    double area(double r) { return pi * r * r; }
}

export namespace geo_ns {
    template <class T> T square(T v) { return v * v; }
}

export using std::sqrt;
