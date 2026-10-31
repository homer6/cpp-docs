#include <cstdio>
#include <optional>
#include <utility>

// RAII without exceptions: a factory reports failure instead of throwing.
class File {
public:
    static std::optional<File> open(const char* path) {
        std::FILE* f = std::fopen(path, "r");
        if (!f) return std::nullopt;
        return File{f};
    }
    File(File&& o) noexcept : f_(std::exchange(o.f_, nullptr)) {}
    File& operator=(File&&) = delete;
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    ~File() { if (f_) std::fclose(f_); }
    std::FILE* get() const { return f_; }
private:
    explicit File(std::FILE* f) : f_(f) {}
    std::FILE* f_;
};
