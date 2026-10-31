// Checks claims made by docs/examples/strings/string_view.md.
#include <string>
#include <string_view>

#include <gtest/gtest.h>

// "Cheap slicing": substr returns views into the same buffer; remove_prefix moves the start.
TEST(StringView, CheapSlicing) {
    std::string_view sv = "key=value";
    auto eq = sv.find('=');
    std::string_view key = sv.substr(0, eq);
    std::string_view val = sv.substr(eq + 1);
    EXPECT_EQ(key, "key");
    EXPECT_EQ(val, "value");
    EXPECT_EQ(key.data(), sv.data());          // no allocation: same buffer
    EXPECT_EQ(val.data(), sv.data() + eq + 1);

    const char* before = sv.data();
    sv.remove_prefix(4);
    EXPECT_EQ(sv, "value");
    EXPECT_EQ(sv.data(), before + 4);          // just moves the pointer
}

// "Same read API as string": starts_with / ends_with (C++20), contains (C++23).
TEST(StringView, ReadApi) {
    std::string_view sv = "value";
    EXPECT_EQ(sv.size(), 5u);
    EXPECT_EQ(sv.front(), 'v');
    EXPECT_EQ(sv.back(), 'e');
    EXPECT_TRUE(sv.starts_with("val"));
    EXPECT_TRUE(sv.ends_with("ue"));
    EXPECT_TRUE(sv.contains("alu"));
}

// "Gotchas": not null-terminated — a sub-view's data() runs on past its end.
TEST(StringView, NotNullTerminated) {
    std::string_view key = std::string_view{"key=value"}.substr(0, 3);
    EXPECT_EQ(key.size(), 3u);
    EXPECT_EQ(key.data()[3], '=');             // no terminator after "key"
    EXPECT_EQ(std::string(key).c_str()[3], '\0');  // copy into a string to get one
}

// "Gotchas": remove_prefix/remove_suffix shrink the view, not the data.
TEST(StringView, RemoveSuffixLeavesDataAlone) {
    std::string s = "hello world";
    std::string_view sv = s;
    sv.remove_suffix(6);
    EXPECT_EQ(sv, "hello");
    EXPECT_EQ(s, "hello world");
}
