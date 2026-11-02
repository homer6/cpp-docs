// Checks claims made by docs/guidelines/core-guidelines/examples/faq.md.
#include <array>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace {
template <class T> using owner = T;

void shout(std::span<char> s) {
    if (!s.empty()) s[0] = 'H';
}
}  // namespace

// FAQ.55: a string_view only reads its characters; a span<char> can write them.
TEST(CoreGuidelinesFAQ, FAQ55_StringViewReadsSpanWrites) {
    static_assert(std::is_same_v<decltype(std::declval<std::string_view&>()[0]), const char&>);
    static_assert(std::is_same_v<decltype(std::declval<std::span<char>&>()[0]), char&>);
    std::string text = "hello";
    shout(text);
    EXPECT_EQ(text, "Hello");
}

// FAQ.56: an owner<T*> alias is only a label; the type is still T*.
TEST(CoreGuidelinesFAQ, FAQ56_OwnerIsAnAlias) {
    static_assert(std::is_same_v<owner<int*>, int*>);
    SUCCEED();
}

// FAQ.57: a std::array holds its elements inside itself, wherever the array object lives (here, the heap).
TEST(CoreGuidelinesFAQ, FAQ57_ArrayElementsLiveInsideTheObject) {
    auto on_heap = std::make_unique<std::array<int, 4>>();
    const void* begin = on_heap.get();
    const void* end = on_heap.get() + 1;
    const void* first = on_heap->data();
    const std::less<const void*> less;
    EXPECT_FALSE(less(first, begin));
    EXPECT_TRUE(less(first, end));
}
