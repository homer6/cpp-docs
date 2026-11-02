// Checks claims made by docs/guidelines/core-guidelines/examples/profile.md.
#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

namespace {
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape { double r = 2; };
struct Square : Shape { double side = 3; };

template <class To, class From>
concept static_downcastable = requires(From* p) { static_cast<To*>(p); };

template <class To, class From>
concept static_castable = requires(From f) { static_cast<To>(f); };

template <class To, class From>
concept brace_constructible_from = requires(From f) { To{f}; };

std::int8_t level(int x) {
    if (!std::in_range<std::int8_t>(x)) throw std::out_of_range("level");
    return static_cast<std::int8_t>(x);
}

class Report {
public:
    int total() const {
        if (!cache_) cache_ = compute();
        return *cache_;
    }
    int computations() const { return computations_; }
private:
    int compute() const { ++computations_; return 42; }
    mutable std::optional<int> cache_;
    mutable int computations_ = 0;
};

struct Counter {
    int count = 0;
    void bump() { ++count; }
};

template <std::integral... Ts>
int sum(Ts... xs) { return (0 + ... + xs); }

template <class... Args>
concept summable = requires(Args... a) { sum(a...); };

int sum_tail(std::span<const int> s, std::size_t from) {
    int total = 0;
    for (int x : s.subspan(from)) total += x;
    return total;
}
}  // namespace

// Type.1: std::bit_cast copies an object's bits into another type, defined where reinterpret_cast punning is UB.
TEST(CoreGuidelinesPro, Type1_BitCastInsteadOfReinterpretCast) {
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    const float f = 1.0f;
    const auto bits = std::bit_cast<std::uint32_t>(f);
    EXPECT_EQ(std::bit_cast<float>(bits), f);
    if constexpr (std::numeric_limits<float>::is_iec559) {
        EXPECT_EQ(bits, 0x3f80'0000u);
        constexpr auto at_compile_time = std::bit_cast<std::uint32_t>(1.0f);   // usable in constant expressions
        EXPECT_EQ(at_compile_time, 0x3f80'0000u);
    }
}

// Type.1: a narrowing value is caught by std::in_range or brace-init instead of a silent static_cast.
TEST(CoreGuidelinesPro, Type1_CheckArithmeticConversions) {
    EXPECT_FALSE(std::in_range<std::int8_t>(300));
    EXPECT_TRUE(std::in_range<std::int8_t>(100));
    EXPECT_THROW(level(300), std::out_of_range);
    EXPECT_EQ(level(-5), -5);
    static_assert(brace_constructible_from<std::int64_t, int>);   // widening is fine
    static_assert(!brace_constructible_from<int, double>);        // {} rejects narrowing
    static_assert(static_castable<int, double>);                  // static_cast doesn't
}

// Type.1: an upcast or identity pointer conversion is implicit, so no cast is needed.
TEST(CoreGuidelinesPro, Type1_ImplicitPointerConversionsNeedNoCast) {
    static_assert(std::is_convertible_v<Circle*, Shape*>);
    static_assert(std::is_convertible_v<Circle*, const void*>);
    Circle c;
    Shape* s = &c;
    EXPECT_EQ(s, static_cast<Shape*>(&c));
}

// Type.2: static_cast compiles for any base-to-derived downcast; dynamic_cast checks and gives null or bad_cast.
TEST(CoreGuidelinesPro, Type2_DynamicCastChecksDowncasts) {
    static_assert(static_downcastable<Circle, Shape>);   // compiles whatever the object really is
    Square sq;
    Shape& s = sq;
    EXPECT_EQ(dynamic_cast<Circle*>(&s), nullptr);
    EXPECT_THROW(static_cast<void>(dynamic_cast<Circle&>(s)), std::bad_cast);
    Circle c;
    Shape* p = &c;
    ASSERT_NE(dynamic_cast<Circle*>(p), nullptr);
    EXPECT_DOUBLE_EQ(dynamic_cast<Circle*>(p)->r, 2.0);
}

// Type.3: a mutable member lets a const function update a cache without const_cast.
TEST(CoreGuidelinesPro, Type3_MutableInsteadOfConstCast) {
    const Report r;
    EXPECT_EQ(r.total(), 42);
    EXPECT_EQ(r.total(), 42);
    EXPECT_EQ(r.computations(), 1);
}

// Type.4: a named cast refuses to drop const where a C-style cast would; const_cast is the only one that can.
TEST(CoreGuidelinesPro, Type4_NamedCastsRefuseToDropConst) {
    static_assert(!static_castable<char*, const char*>);
    static_assert(requires(const char* p) { const_cast<char*>(p); });
    const std::string s{"hello"};
    std::string copy{s};
    copy[0] = 'H';
    EXPECT_EQ(copy, "Hello");
    EXPECT_EQ(s, "hello");
}

// Type.5: a variable initialized at its declaration has a value on every path; {} value-initializes to zero.
TEST(CoreGuidelinesPro, Type5_InitializeAtDeclaration) {
    const auto choose = [](bool flag) { const int x = flag ? 1 : 0; return x; };
    EXPECT_EQ(choose(true), 1);
    EXPECT_EQ(choose(false), 0);
    int zero{};
    std::array<int, 3> zeros{};
    EXPECT_EQ(zero, 0);
    EXPECT_EQ(zeros, (std::array<int, 3>{0, 0, 0}));
}

// Type.6: a default member initializer gives the member a value even under default-initialization.
TEST(CoreGuidelinesPro, Type6_DefaultMemberInitializers) {
    Counter c;
    EXPECT_EQ(c.count, 0);
    c.bump();
    EXPECT_EQ(c.count, 1);
}

// Type.7: std::variant records the live alternative and refuses to hand out another.
TEST(CoreGuidelinesPro, Type7_VariantInsteadOfUnion) {
    std::variant<int, double> n = 3;
    EXPECT_EQ(n.index(), 0u);
    EXPECT_EQ(std::get_if<double>(&n), nullptr);
    EXPECT_THROW(static_cast<void>(std::get<double>(n)), std::bad_variant_access);
    EXPECT_DOUBLE_EQ(std::visit([](auto v) { return static_cast<double>(v); }, n), 3.0);
}

// Type.8: a constrained variadic template keeps every argument's type and rejects the wrong ones.
TEST(CoreGuidelinesPro, Type8_VariadicTemplateInsteadOfVarargs) {
    EXPECT_EQ(sum(1, 2, 3), 6);
    EXPECT_EQ(sum(), 0);
    static_assert(summable<int, long>);
    static_assert(!summable<int, double>);
}

// Bounds.1: std::span carries its size, so subspan slices without losing track of the end.
TEST(CoreGuidelinesPro, Bounds1_SpanInsteadOfPointerArithmetic) {
    const std::array<int, 5> a{1, 2, 3, 4, 5};
    const std::span<const int> s{a};
    EXPECT_EQ(s.subspan(3).size(), 2u);
    EXPECT_EQ(sum_tail(s, 3), 9);
    EXPECT_EQ(s.first(2).back(), 2);
    EXPECT_EQ(s.last(1).front(), 5);
}

// Bounds.2: std::get<N> checks a constant index at compile time; at() checks a run-time one.
TEST(CoreGuidelinesPro, Bounds2_CheckedIndexing) {
    const std::array<int, 3> a{10, 20, 30};
    EXPECT_EQ(std::get<1>(a), 20);
    EXPECT_EQ(a.at(2), 30);
    EXPECT_THROW(static_cast<void>(a.at(3)), std::out_of_range);
}

// Bounds.3: an array decays to a bare pointer, but a span built from it keeps the length.
TEST(CoreGuidelinesPro, Bounds3_SpanKeepsArrayLength) {
    int data[4] = {1, 2, 3, 4};
    static_assert(std::is_same_v<std::decay_t<decltype(data)>, int*>);
    std::span s{data};
    static_assert(decltype(s)::extent == 4);
    EXPECT_EQ(s.size(), 4u);
}

// Bounds.4: std::string grows to fit and vector::at checks its index, where strcpy and [] don't.
TEST(CoreGuidelinesPro, Bounds4_BoundsCheckedLibraryFacilities) {
    const char* name = "a name much longer than sixteen characters";
    const std::string buf = name;
    EXPECT_EQ(buf.size(), std::char_traits<char>::length(name));
    const std::vector<int> v{1, 2, 3};
    EXPECT_THROW(static_cast<void>(v.at(3)), std::out_of_range);
}

// Lifetime.1: an index stays valid when a vector reallocates, unlike a pointer to its element.
TEST(CoreGuidelinesPro, Lifetime1_IndexSurvivesReallocation) {
    std::vector<int> v{7};
    v.shrink_to_fit();
    const std::size_t first = 0;
    for (int i = 0; i < 100; ++i) v.push_back(i);   // forces at least one reallocation
    EXPECT_GE(v.capacity(), 101u);
    EXPECT_EQ(v[first], 7);
}
