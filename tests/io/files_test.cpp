// Checks claims made by docs/examples/io/files.md.
// Every test works in its own temp directory, removed in TearDown.
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <gtest/gtest.h>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {
int process_id() {
#if defined(_WIN32)
    return ::_getpid();
#else
    return ::getpid();
#endif
}

// The bytes a text-mode stream writes for `s`. Windows opens files in text mode
// by default and writes each '\n' as "\r\n"; slurp() reads in binary mode, so it
// sees the CRs. Elsewhere text and binary mode are the same.
std::string text_mode(std::string s) {
#if defined(_WIN32)
    for (std::size_t i = 0; (i = s.find('\n', i)) != std::string::npos; i += 2) s.insert(i, 1, '\r');
#endif
    return s;
}
}  // namespace

class Files : public testing::Test {
protected:
    fs::path dir;
    void SetUp() override {
        dir = fs::temp_directory_path() /
              ("cppdocs-files-" + std::to_string(process_id()) + "-" +
               testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::create_directories(dir);
    }
    void TearDown() override { fs::remove_all(dir); }
    std::string slurp(const fs::path& p) {
        std::ifstream in{p, std::ios::binary};
        return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
    }
};

// "Write a file": opened on construction, closed (and flushed) by the destructor.
TEST_F(Files, WriteClosesInDestructor) {
    {
        std::ofstream out{dir / "data.txt"};
        ASSERT_TRUE(out);
        out << "line 1\n" << 42 << '\n';
    }
    EXPECT_EQ(slurp(dir / "data.txt"), text_mode("line 1\n42\n"));
}

// "Read a file": line by line, then token by token; read_all slurps everything.
TEST_F(Files, ReadLinesTokensAll) {
    { std::ofstream{dir / "nums.txt"} << "1 2\n3\n"; }
    std::ifstream in{dir / "nums.txt"};
    std::string line;
    int lines = 0;
    while (std::getline(in, line)) ++lines;
    EXPECT_EQ(lines, 2);
    std::ifstream in2{dir / "nums.txt"};
    int n, sum = 0;
    while (in2 >> n) sum += n;
    EXPECT_EQ(sum, 6);
    EXPECT_EQ(slurp(dir / "nums.txt"), text_mode("1 2\n3\n"));
}

// "Gotchas": opening a missing file leaves the stream falsy.
TEST_F(Files, OpenFailureIsFalsy) {
    std::ifstream in{dir / "missing.txt"};
    EXPECT_FALSE(in);
    EXPECT_FALSE(in.is_open());
}

// "Open modes" + "Gotchas": ofstream truncates by default; ios::app appends; ios::ate seeks to end.
TEST_F(Files, TruncateAppendAte) {
    auto p = dir / "log.txt";
    { std::ofstream{p} << "first\n"; }
    { std::ofstream{p} << "second\n"; }
    EXPECT_EQ(slurp(p), text_mode("second\n"));
    { std::ofstream{p, std::ios::app} << "third\n"; }
    EXPECT_EQ(slurp(p), text_mode("second\nthird\n"));
    std::ifstream at{p, std::ios::ate};
#if defined(_WIN32)
    EXPECT_EQ(at.tellg(), std::streampos(15));  // text mode wrote "second\r\nthird\r\n"
#else
    EXPECT_EQ(at.tellg(), std::streampos(13));
#endif
}

// "Binary I/O & positioning": raw write/read round-trips; seekg/tellg give the size.
TEST_F(Files, BinaryRoundTripAndSize) {
    auto p = dir / "f.bin";
    {
        std::ofstream out{p, std::ios::binary};
        int value = 42;
        out.write(reinterpret_cast<const char*>(&value), sizeof value);
    }
    std::ifstream in{p, std::ios::binary};
    int value = 0;
    in.read(reinterpret_cast<char*>(&value), sizeof value);
    EXPECT_EQ(value, 42);
    in.seekg(0, std::ios::end);
    EXPECT_EQ(in.tellg(), std::streampos(sizeof(int)));
    in.seekg(0);
    EXPECT_EQ(in.tellg(), std::streampos(0));

    std::fstream io{p, std::ios::in | std::ios::out | std::ios::binary};  // existing file, no truncation
    ASSERT_TRUE(io);
    io.seekg(0, std::ios::end);
    EXPECT_EQ(io.tellg(), std::streampos(sizeof(int)));
}
