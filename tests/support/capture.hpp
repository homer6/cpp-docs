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
// POSIX only (macOS, Linux, the gcc:16 container).
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

#include <unistd.h>

namespace cppdocs {

class CaptureStdout {
public:
    CaptureStdout() {
        std::cout.flush();
        std::fflush(stdout);
        file_ = std::tmpfile();
        if (file_ == nullptr) throw std::runtime_error("tmpfile failed");
        saved_fd_ = ::dup(STDOUT_FILENO);
        if (saved_fd_ < 0 || ::dup2(::fileno(file_), STDOUT_FILENO) < 0)
            throw std::runtime_error("dup2 failed");
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
        ::dup2(saved_fd_, STDOUT_FILENO);
        ::close(saved_fd_);
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
