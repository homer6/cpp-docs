#include <cstdint>

enum class Color : std::uint8_t { red = 1, green = 2 };

constexpr std::uint8_t wire(Color c) { return static_cast<std::uint8_t>(c); }
static_assert(wire(Color::green) == 2);
