#include <cstddef>
#include <span>

// Generated at build time (e.g. by a CMake step) from assets/logo.png.
constexpr unsigned char logo_png[] = {0x89, 0x50, 0x4E, 0x47};

constexpr std::span<const unsigned char> logo() { return logo_png; }
static_assert(logo().size() == 4);
