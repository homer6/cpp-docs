#include <cstdio>

class Log {
public:
    explicit Log(const char* path) : f_(std::fopen(path, "w")) {}
    ~Log() { if (f_) std::fclose(f_); }
    void write(const char* s) { if (f_) std::fputs(s, f_); }
private:
    std::FILE* f_;
};
