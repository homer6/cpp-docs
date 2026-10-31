// Checks claims made by docs/examples/io/filesystem.md.
// Every test works in its own temp directory, removed in TearDown.
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <system_error>
#include <type_traits>

#include <gtest/gtest.h>
#include <unistd.h>

namespace fs = std::filesystem;

class Filesystem : public testing::Test {
protected:
    fs::path dir;
    void SetUp() override {
        dir = fs::temp_directory_path() /
              ("cppdocs-filesystem-" + std::to_string(::getpid()) + "-" +
               testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::create_directories(dir);
    }
    void TearDown() override { fs::remove_all(dir); }
    void touch(const fs::path& p, const std::string& text = "abc") { std::ofstream{p} << text; }
};

// "Paths": decomposition and operator/.
TEST(FilesystemPath, Decomposition) {
    fs::path p = "dir/sub/file.txt";
    EXPECT_EQ(p / "more", fs::path("dir/sub/file.txt/more"));
    EXPECT_EQ(p.filename(), "file.txt");
    EXPECT_EQ(p.stem(), "file");
    EXPECT_EQ(p.extension(), ".txt");
    EXPECT_EQ(p.parent_path(), "dir/sub");
    EXPECT_FALSE(p.is_absolute());
    EXPECT_TRUE(fs::path("/tmp").is_absolute());  // POSIX
}

// "Querying the filesystem": exists, kinds, size, write time, absolute/canonical.
TEST_F(Filesystem, Querying) {
    auto f = dir / "a.txt";
    touch(f);
    EXPECT_TRUE(fs::exists(f));
    EXPECT_TRUE(fs::is_regular_file(f));
    EXPECT_TRUE(fs::is_directory(dir));
    EXPECT_EQ(fs::file_size(f), 3u);
    static_assert(std::is_same_v<decltype(fs::last_write_time(f)), fs::file_time_type>);
    static_assert(std::chrono::is_clock_v<fs::file_time_type::clock>);
    EXPECT_TRUE(fs::current_path().is_absolute());
    EXPECT_TRUE(fs::absolute("x").is_absolute());
    EXPECT_EQ(fs::canonical(dir / "." / "a.txt"), fs::canonical(f));
}

// "Directory iteration": directory_iterator lists one level, recursive walks subdirectories.
TEST_F(Filesystem, Iteration) {
    fs::create_directories(dir / "sub");
    touch(dir / "top.txt");
    touch(dir / "sub" / "deep.txt", "12345");
    std::set<std::string> flat, all;
    std::uintmax_t size = 0;
    for (const fs::directory_entry& e : fs::directory_iterator{dir}) {
        flat.insert(e.path().filename().string());
        if (e.is_regular_file()) size = e.file_size();
    }
    for (const auto& e : fs::recursive_directory_iterator{dir}) all.insert(e.path().filename().string());
    EXPECT_EQ(flat, (std::set<std::string>{"sub", "top.txt"}));
    EXPECT_EQ(all, (std::set<std::string>{"sub", "top.txt", "deep.txt"}));
    EXPECT_EQ(size, 3u);
}

// "Modifying operations": the return values in the comments.
TEST_F(Filesystem, Modifying) {
    EXPECT_TRUE(fs::create_directory(dir / "out"));
    EXPECT_FALSE(fs::create_directory(dir / "out"));  // already there: false, no error
    EXPECT_TRUE(fs::create_directories(dir / "a/b/c"));
    touch(dir / "src.txt");
    fs::copy(dir / "src.txt", dir / "dst.txt");
    touch(dir / "a/b/c/x.txt");
    fs::copy(dir / "a", dir / "backup", fs::copy_options::recursive);
    EXPECT_TRUE(fs::exists(dir / "backup/b/c/x.txt"));
    fs::rename(dir / "dst.txt", dir / "new.txt");
    EXPECT_TRUE(fs::exists(dir / "new.txt"));
    EXPECT_TRUE(fs::remove(dir / "new.txt"));
    EXPECT_FALSE(fs::remove(dir / "new.txt"));        // didn't exist
    EXPECT_EQ(fs::remove_all(dir / "a"), 4u);          // a, b, c, x.txt
}

// "Error handling — two flavors": the throwing form throws filesystem_error
// with path1() and code(); the error_code overload doesn't throw.
TEST_F(Filesystem, ThrowingAndErrorCode) {
    auto bad = dir / "missing" / "out";
    try {
        fs::create_directory(bad);
        FAIL() << "expected filesystem_error";
    } catch (const fs::filesystem_error& e) {
        EXPECT_EQ(e.path1(), bad);
        EXPECT_EQ(e.code(), std::errc::no_such_file_or_directory);
    }
    std::error_code ec;
    EXPECT_NO_THROW(fs::create_directory(bad, ec));
    EXPECT_TRUE(ec);
}

// "Gotchas": u8string() gives UTF-8 (char8_t) regardless of the native encoding.
TEST(FilesystemPath, U8String) {
    fs::path p{u8"café.txt"};
    static_assert(std::is_same_v<decltype(p.u8string()), std::u8string>);
    EXPECT_EQ(p.u8string(), u8"café.txt");
}
