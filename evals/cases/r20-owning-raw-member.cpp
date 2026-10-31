#include <cstddef>

class Buffer {
public:
    explicit Buffer(std::size_t n) : data_(new char[n]), size_(n) {}
    ~Buffer() { delete[] data_; }
    std::size_t size() const { return size_; }
private:
    char* data_;          // owning
    std::size_t size_;
};
