// Output capture for tests that check what a page says a snippet prints.
//
// std::cout and std::print(...) reach the terminal by different routes:
// std::cout goes through its streambuf, std::print(...) without a stream
// argument writes to the C stream `stdout` (it is specified in terms of
// vprint_unicode(stdout, ...)). Swapping std::cout's rdbuf would miss
// std::print and printf; redirecting only the C stream would miss a cout that
// has been unsynced from stdio. So CaptureStdout redirects file descriptor 1
// to a temporary file (catching stdout, printf, std::print and a synced
// std::cout) *and* swaps std::cout's rdbuf to a buffer that forwards into C
// stdout, so cout text lands in the same file in program order even if
// sync_with_stdio(false) was called before capturing. Both are flushed before
// reading. Don't call sync_with_stdio inside the captured code: libstdc++
// replaces cout's streambuf there, which bypasses the forwarding buffer (the
// text is still captured through fd 1, but may come out of order).
//
// Portable: POSIX dup/dup2 on macOS, Linux and FreeBSD; the CRT's _dup/_dup2
// on Windows (MSVC, clang-cl, MinGW), where fd 1 is also switched to binary
// mode while capturing so "\n" isn't written as "\r\n", and the temporary
// file goes in the temp directory (the CRT's tmpfile() uses the drive root).
//
//   auto out = cppdocs::capture_stdout([] { std::print("{} {}", 1, 2); });
//   EXPECT_EQ(out, "1 2");
#pragma once

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <utility>

#if defined(_WIN32)
#include <atomic>
#include <filesystem>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

namespace cppdocs {

namespace detail {
#if defined(_WIN32)
inline int dup_fd(int fd) { return ::_dup(fd); }
inline int dup2_fd(int from, int to) { return ::_dup2(from, to); }
inline int close_fd(int fd) { return ::_close(fd); }
inline int file_fd(std::FILE* f) { return ::_fileno(f); }
inline constexpr int stdout_fd = 1;

// tmpfile() would create the file in the drive root; "D" deletes it on close.
inline std::FILE* capture_file() {
    static std::atomic<unsigned> counter{0};
    auto path = std::filesystem::temp_directory_path() /
                ("cppdocs-capture-" + std::to_string(::_getpid()) + "-" +
                 std::to_string(counter++) + ".tmp");
    return ::_wfopen(path.c_str(), L"w+bD");
}
#else
inline int dup_fd(int fd) { return ::dup(fd); }
inline int dup2_fd(int from, int to) { return ::dup2(from, to); }
inline int close_fd(int fd) { return ::close(fd); }
inline int file_fd(std::FILE* f) { return ::fileno(f); }
inline constexpr int stdout_fd = STDOUT_FILENO;
inline std::FILE* capture_file() { return std::tmpfile(); }
#endif
}  // namespace detail

class CaptureStdout {
public:
    CaptureStdout() {
        std::cout.flush();
        std::fflush(stdout);
        file_ = detail::capture_file();
        if (file_ == nullptr) throw std::runtime_error("can't create the capture file");
        saved_fd_ = detail::dup_fd(detail::stdout_fd);
        if (saved_fd_ < 0 || detail::dup2_fd(detail::file_fd(file_), detail::stdout_fd) < 0)
            throw std::runtime_error("dup2 failed");
#if defined(_WIN32)
        saved_mode_ = ::_setmode(detail::stdout_fd, _O_BINARY);
#endif
        saved_buf_ = std::cout.rdbuf(&forward_);
    }

    CaptureStdout(const CaptureStdout&) = delete;
    CaptureStdout& operator=(const CaptureStdout&) = delete;

    ~CaptureStdout() { stop(); }

    // Stops capturing (idempotent) and returns everything written so far.
    std::string str() {
        stop();
        return text_;
    }

private:
    // Forwards every character std::cout writes into the C stream stdout.
    struct ForwardToStdio : std::streambuf {
        int_type overflow(int_type ch) override {
            if (traits_type::eq_int_type(ch, traits_type::eof())) return traits_type::not_eof(ch);
            return std::fputc(traits_type::to_char_type(ch), stdout) == EOF ? traits_type::eof() : ch;
        }
        std::streamsize xsputn(const char* s, std::streamsize n) override {
            return static_cast<std::streamsize>(std::fwrite(s, 1, static_cast<std::size_t>(n), stdout));
        }
        int sync() override { return std::fflush(stdout); }
    };

    void stop() {
        if (file_ == nullptr) return;
        std::cout.flush();
        std::cout.rdbuf(saved_buf_);
        std::fflush(stdout);
#if defined(_WIN32)
        if (saved_mode_ != -1) ::_setmode(detail::stdout_fd, saved_mode_);
#endif
        detail::dup2_fd(saved_fd_, detail::stdout_fd);
        detail::close_fd(saved_fd_);
        std::rewind(file_);
        char buf[4096];
        for (std::size_t n; (n = std::fread(buf, 1, sizeof buf, file_)) > 0;) text_.append(buf, n);
        std::fclose(file_);
        file_ = nullptr;
    }

    ForwardToStdio forward_;
    std::streambuf* saved_buf_ = nullptr;
    std::FILE* file_ = nullptr;
    int saved_fd_ = -1;
#if defined(_WIN32)
    int saved_mode_ = -1;
#endif
    std::string text_;
};

// Runs fn() and returns what it wrote to std::cout / stdout / std::print.
template <class F>
std::string capture_stdout(F&& fn) {
    CaptureStdout capture;
    std::forward<F>(fn)();
    return capture.str();
}

}  // namespace cppdocs
