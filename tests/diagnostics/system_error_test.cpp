// Checks claims made by docs/examples/diagnostics/system_error.md.
#include <cerrno>
#include <filesystem>
#include <string>
#include <system_error>

#include <unistd.h>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

namespace {
// A unique temp directory, removed when the test ends.
struct TempDir {
    fs::path path = fs::temp_directory_path() /
                    ("cppdocs-system_error-" + std::to_string(::getpid()));
    TempDir() { fs::create_directories(path); }
    ~TempDir() { std::error_code ec; fs::remove_all(path, ec); }
};

enum class LibErr { ok = 0, bad_header = 1 };
struct LibCategory : std::error_category {
    const char* name() const noexcept override { return "lib"; }
    std::string message(int v) const override { return v == 1 ? "bad header" : "ok"; }
};
const LibCategory& lib_category() {
    static LibCategory c;
    return c;
}
}  // namespace

// "The error_code overload pattern": the non-throwing overload fills ec instead
// of throwing; ec is truthy only on failure.
TEST(SystemError, ErrorCodeOverload) {
    TempDir tmp;
    std::error_code ec;
    fs::create_directory(tmp.path / "out", ec);
    EXPECT_FALSE(ec);
    fs::create_directory(tmp.path / "missing" / "out", ec);
    ASSERT_TRUE(ec);
    EXPECT_FALSE(ec.message().empty());
    EXPECT_NE(ec.value(), 0);
    EXPECT_EQ(ec, std::errc::no_such_file_or_directory);
}

// "Comparing against portable conditions": ec == errc works across categories.
TEST(SystemError, CompareWithErrc) {
    TempDir tmp;
    std::error_code ec;
    (void)fs::file_size(tmp.path / "nope", ec);
    EXPECT_EQ(ec, std::errc::no_such_file_or_directory);
    std::error_code sys{ENOENT, std::system_category()};
    EXPECT_EQ(sys, std::errc::no_such_file_or_directory);  // via the category's equivalence
    EXPECT_NE(sys, std::errc::permission_denied);
}

// "Throwing form": system_error wraps the code and prefixes what() with the message.
TEST(SystemError, SystemErrorCarriesCode) {
    try {
        throw std::system_error{EACCES, std::generic_category(), "open failed"};
    } catch (const std::system_error& e) {
        EXPECT_EQ(e.code(), std::errc::permission_denied);
        EXPECT_EQ(std::string(e.what()).rfind("open failed", 0), 0u);
    }
}

// "error_code vs error_condition": the code is concrete, the condition portable.
TEST(SystemError, CodeVersusCondition) {
    std::error_code ec{ENOENT, std::system_category()};
    std::error_condition cond = ec.default_error_condition();
    EXPECT_EQ(cond.category(), std::generic_category());
    EXPECT_EQ(cond, std::errc::no_such_file_or_directory);
    std::error_code ok;
    EXPECT_FALSE(ok);  // value 0 = success
}

// "Gotchas": a custom error_category plugs library codes into the machinery.
TEST(SystemError, CustomCategory) {
    std::error_code ec{static_cast<int>(LibErr::bad_header), lib_category()};
    EXPECT_TRUE(ec);
    EXPECT_STREQ(ec.category().name(), "lib");
    EXPECT_EQ(ec.message(), "bad header");
    EXPECT_NE(ec, std::error_code(1, std::generic_category()));  // same value, different category
}
