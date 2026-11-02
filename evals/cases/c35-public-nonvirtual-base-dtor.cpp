#include <memory>

struct Shape {
    virtual double area() const = 0;
    ~Shape() = default;
};
struct Circle : Shape {
    double r = 1;
    double area() const override { return 3.14159 * r * r; }
};
std::unique_ptr<Shape> make() { return std::make_unique<Circle>(); }
