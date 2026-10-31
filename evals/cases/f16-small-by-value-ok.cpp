#include <string_view>

struct Rgba { unsigned char r, g, b, a; };      // 4 bytes
struct Vec2 { double x, y; };                    // 16 bytes

bool starts_with_hash(std::string_view s) { return !s.empty() && s[0] == '#'; }
Rgba darker(Rgba c) { return {static_cast<unsigned char>(c.r / 2), static_cast<unsigned char>(c.g / 2), static_cast<unsigned char>(c.b / 2), c.a}; }
double dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
