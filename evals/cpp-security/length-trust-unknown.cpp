#include <cstdint>
#include <istream>
#include <vector>

std::vector<char> read_record(std::istream& in) {
    std::uint32_t length = 0;
    in.read(reinterpret_cast<char*>(&length), sizeof length);
    std::vector<char> data(length);
    in.read(data.data(), static_cast<std::streamsize>(data.size()));
    return data;
}
