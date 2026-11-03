// Checks claims made by docs/security/filesystem-and-processes.md.
#include <filesystem>

#include <gtest/gtest.h>

namespace {

bool stays_inside(const std::filesystem::path& untrusted) {
    if (untrusted.empty() || untrusted.is_absolute() ||
        untrusted.has_root_name() || untrusted.has_root_directory())
        return false;
    for (const auto& part : untrusted.lexically_normal())
        if (part == "..") return false;
    return true;
}

}  // namespace

// "Keep user-named files inside their directory": relative names without ".." pass.
TEST(SecurityFilesystem, RelativeNamesPass) {
    EXPECT_TRUE(stays_inside("report.txt"));
    EXPECT_TRUE(stays_inside("2026/report.txt"));
    EXPECT_TRUE(stays_inside("a/./b.txt"));
    EXPECT_TRUE(stays_inside("a/b/../c.txt"));   // normalizes to a/c.txt, still inside
}

// "Keep user-named files inside their directory": absolute paths, root names and any
// ".." that survives normalization are rejected.
TEST(SecurityFilesystem, EscapesRejected) {
    EXPECT_FALSE(stays_inside(""));
    EXPECT_FALSE(stays_inside("../secret"));
    EXPECT_FALSE(stays_inside("a/../../secret"));
    EXPECT_FALSE(stays_inside("/etc/passwd"));
#if defined(_WIN32)
    EXPECT_FALSE(stays_inside("C:\\Windows\\win.ini"));
    EXPECT_FALSE(stays_inside("C:relative"));   // a root name without a root directory
    EXPECT_FALSE(stays_inside("\\server\\share"));
#endif
}

// "Keep user-named files inside their directory": joining a rejected absolute path to a
// base directory would replace the base entirely, which is why it's rejected first.
TEST(SecurityFilesystem, AbsolutePathReplacesBaseWhenJoined) {
#if defined(_WIN32)
    std::filesystem::path base = "C:\\uploads", hostile = "D:\\secret";
#else
    std::filesystem::path base = "/srv/uploads", hostile = "/etc/passwd";
#endif
    EXPECT_EQ(base / hostile, hostile);
}
