// Checks claims made by docs/guidelines/core-guidelines/examples/performance.md.
//
// Performance itself isn't asserted (timings are flaky and machine-dependent);
// these check the properties the examples rely on: types, compile-time values,
// aliasing, allocations, layout and contiguity.
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <list>
#include <memory>
#include <numeric>
#include <span>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

namespace {

// Per.11
constexpr std::array<int, 8> make_squares() {
    std::array<int, 8> t{};
    for (int i = 0; i < 8; ++i) t[static_cast<std::size_t>(i)] = i * i;
    return t;
}
constexpr std::array<int, 8> squares = make_squares();

template <int N>
constexpr int as_template_argument = N;

// Per.12
void add_twice_by_ref(int& total, const int& step) {
    total += step;
    total += step;
}
void add_twice_by_value(int& total, int step) {
    total += step;
    total += step;
}

// Per.13
struct Point {
    float x, y;
};

// Per.15
class Framer {
public:
    explicit Framer(std::size_t max_packet) { frame_.reserve(max_packet + 1); }
    std::span<const char> on_packet(std::span<const char> in) {
        frame_.clear();
        frame_.insert(frame_.end(), in.begin(), in.end());
        frame_.push_back('\n');
        return frame_;
    }
    const char* storage() const { return frame_.data(); }
    std::size_t capacity() const { return frame_.capacity(); }

private:
    std::vector<char> frame_;
};

// Per.17
struct SessionBad {
    char user_name[128];
    int last_seen;
};
struct SessionGood {
    int last_seen;
    char user_name[128];
};

// Per.18
struct OrderBad {
    bool paid;
    double amount;
    bool shipped;
    int quantity;
};
struct OrderGood {
    double amount;
    int quantity;
    bool paid;
    bool shipped;
};

struct NoLess {
    int v;
};

}  // namespace

// Per.10: a typed sort knows the element type: it sorts doubles correctly and rejects types without <.
TEST(CoreGuidelinesPer, Per10_TypedSortIsChecked) {
    std::vector<double> prices{3.5, 0.25, 2.0, 10.0};
    std::ranges::sort(prices);
    EXPECT_EQ(prices, (std::vector<double>{0.25, 2.0, 3.5, 10.0}));
    static_assert(std::sortable<std::vector<double>::iterator>);
    static_assert(!std::sortable<std::vector<NoLess>::iterator>);
}

// Per.11: a constexpr table is computed by the compiler: usable in static_assert and as a template argument.
TEST(CoreGuidelinesPer, Per11_ConstexprTableIsCompileTime) {
    static_assert(squares[3] == 9);
    static_assert(as_template_argument<squares[7]> == 49);
    EXPECT_EQ(squares[5], 25);
}

// Per.12: a reference parameter may alias another argument and change the result; a value parameter can't.
TEST(CoreGuidelinesPer, Per12_AliasingChangesTheResult) {
    int x = 5;
    add_twice_by_ref(x, x);
    EXPECT_EQ(x, 20);  // 5 + 5, then 10 + 10: step changed under us
    int y = 5;
    add_twice_by_value(y, y);
    EXPECT_EQ(y, 15);  // 5 + 5 + 5
}

// Per.13: values stored directly in a vector sit next to each other; no pointer to follow per element.
TEST(CoreGuidelinesPer, Per13_ValuesAreContiguous) {
    std::vector<Point> pts{{1, 2}, {3, 4}, {5, 6}};
    EXPECT_EQ(&pts[1], &pts[0] + 1);
    EXPECT_EQ(&pts[2], &pts[0] + 2);
    float s = 0;
    for (const auto& p : pts) s += p.x;
    EXPECT_FLOAT_EQ(s, 9.0f);
}

// Per.14: after reserve(n), n push_backs don't reallocate: the storage stays put.
TEST(CoreGuidelinesPer, Per14_ReserveAvoidsReallocation) {
    std::vector<int> v;
    v.reserve(1000);
    const int* storage = v.data();
    const auto cap = v.capacity();
    for (int i = 0; i < 1000; ++i) v.push_back(i);
    EXPECT_EQ(v.data(), storage);
    EXPECT_EQ(v.capacity(), cap);
}

// Per.15: clear() keeps a vector's capacity, so a buffer allocated up front is reused with no new allocation.
TEST(CoreGuidelinesPer, Per15_PreallocatedBufferIsReused) {
    Framer f(64);
    const char* storage = f.storage();
    const auto cap = f.capacity();
    const char a[] = {'a', 'b', 'c'};
    const char b[] = {'x'};
    auto out1 = f.on_packet(a);
    EXPECT_EQ(out1.size(), 4u);
    auto out2 = f.on_packet(b);
    ASSERT_EQ(out2.size(), 2u);
    EXPECT_EQ(out2[0], 'x');
    EXPECT_EQ(out2[1], '\n');
    EXPECT_EQ(f.storage(), storage);
    EXPECT_EQ(f.capacity(), cap);
}

// Per.16: a vector's elements are contiguous; a list's aren't even random-access.
TEST(CoreGuidelinesPer, Per16_VectorIsCompact) {
    static_assert(std::contiguous_iterator<std::vector<int>::iterator>);
    static_assert(!std::random_access_iterator<std::list<int>::iterator>);
    std::vector<int> v{1, 2, 3, 4};
    for (std::size_t i = 0; i < v.size(); ++i) EXPECT_EQ(&v[i], v.data() + i);
    std::list<int> l(v.begin(), v.end());
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), std::accumulate(l.begin(), l.end(), 0));
}

// Per.17: the first member is at offset 0; a member declared after a 128-byte array is at least 128 bytes in.
TEST(CoreGuidelinesPer, Per17_FirstMemberAtOffsetZero) {
    static_assert(std::is_standard_layout_v<SessionGood> && std::is_standard_layout_v<SessionBad>);
    EXPECT_EQ(offsetof(SessionGood, last_seen), 0u);
    EXPECT_GE(offsetof(SessionBad, last_seen), 128u);
}

// Per.18: declaring members largest first wastes less padding: the same members take less space.
TEST(CoreGuidelinesPer, Per18_MemberOrderChangesSize) {
    static_assert(sizeof(OrderGood) < sizeof(OrderBad));
    if constexpr (alignof(double) == 8 && sizeof(int) == 4) {
        EXPECT_EQ(sizeof(OrderGood), 16u);
        EXPECT_EQ(sizeof(OrderBad), 24u);
    }
}

// Per.19: in row-major storage, row neighbours are adjacent and column neighbours are `cols` apart; both loop orders sum the same.
TEST(CoreGuidelinesPer, Per19_RowMajorLayout) {
    constexpr int rows = 3, cols = 4;
    std::vector<double> m(rows * cols);
    std::iota(m.begin(), m.end(), 1.0);
    auto at = [&](int r, int c) { return &m[static_cast<std::size_t>(r * cols + c)]; };
    EXPECT_EQ(at(1, 2) + 1, at(1, 3));
    EXPECT_EQ(at(1, 2) + cols, at(2, 2));
    double by_rows = 0, by_cols = 0;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) by_rows += *at(r, c);
    for (int c = 0; c < cols; ++c)
        for (int r = 0; r < rows; ++r) by_cols += *at(r, c);
    EXPECT_DOUBLE_EQ(by_rows, 78.0);
    EXPECT_DOUBLE_EQ(by_cols, by_rows);
}
