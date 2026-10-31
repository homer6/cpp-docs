#include <cstddef>
#include <istream>

constexpr std::size_t max = 8 * 1024;

std::size_t read_block(std::istream& in, char (&out)[max]) {
    char buf[max];             // filled by read() immediately below
    in.read(buf, max);
    std::size_t n = static_cast<std::size_t>(in.gcount());
    for (std::size_t i = 0; i < n; ++i) out[i] = buf[i];
    return n;
}
