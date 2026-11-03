// Checks claims made by docs/guidelines/core-guidelines/examples/expr.md.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <version>
#if __has_include(<bit>)
#include <bit>
#endif
#include <span>

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {

// ES.23, ES.46, ES.64: does To{f} compile for an f of type From?
template <class To, class From>
concept brace_init_from = requires(From f) { To{f}; };

// ES.49: does static_cast<To>(f) compile?
template <class To, class From>
concept static_castable = requires(From f) { static_cast<To>(f); };

int destroyed = 0;
struct Tracked {
    ~Tracked() { ++destroyed; }
};

bool load_tracked(bool cancel) {
    auto t = std::make_unique<Tracked>();
    if (cancel) return false;
    throw std::runtime_error{"failed"};
}

constexpr int square(int x) { return x * x; }

template <class... Args>
std::string join(const Args&... args) {
    std::string out;
    ((out += args), ...);
    return out;
}

bool is_set(unsigned flags, unsigned mask) { return (flags & mask) != 0; }

void clear(std::span<int> values) {
    for (int& v : values) v = 0;
}

char to_char(int code) {
    if (code < std::numeric_limits<char>::min() || code > std::numeric_limits<char>::max())
        throw std::out_of_range{"code"};
    return static_cast<char>(code);
}

int pick(int) { return 1; }
int pick(const char*) { return 2; }

[[nodiscard]] int status() { return 7; }

struct Account {
    virtual ~Account() = default;
};
struct Savings : Account {};
struct Unrelated {};

struct Prices {
    mutable std::map<int, double> cache;
    mutable int computed = 0;
    double lookup(int id) const {
        auto [it, added] = cache.try_emplace(id, 0.0);
        if (added) {
            ++computed;
            it->second = id * 1.5;
        }
        return it->second;
    }
};

struct ShapeByValue {
    virtual ~ShapeByValue() = default;
    virtual double area() const { return 0; }
};
struct SquareByValue : ShapeByValue {
    double side = 2;
    double area() const override { return side * side; }
};
double area_by_value(ShapeByValue s) { return s.area(); }
double area_by_ref(const ShapeByValue& s) { return s.area(); }

struct Shape {
    Shape() = default;
    Shape(const Shape&) = delete;
    Shape& operator=(const Shape&) = delete;
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

struct Guard {
    int& held;
    explicit Guard(int& h) : held{h} { ++held; }
    ~Guard() { --held; }
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
};

std::uint8_t invert(std::uint8_t b) { return static_cast<std::uint8_t>(~b); }

constexpr std::int64_t wide_area(std::int32_t w, std::int32_t h) { return std::int64_t{w} * h; }

constexpr std::optional<int> checked_add(int a, int b) {
    if (b > 0 ? a > std::numeric_limits<int>::max() - b : a < std::numeric_limits<int>::min() - b)
        return std::nullopt;
    return a + b;
}

constexpr int sum_backwards() {
    int a[3]{1, 2, 3};
    int sum = 0;
    for (int i = 2; i >= 0; --i) sum += a[i];
    return sum;
}

constexpr std::optional<int> per_person(int total, int people) {
    if (people == 0) return std::nullopt;
    return total / people;
}

unsigned area_unsigned(unsigned w, unsigned h) { return w * h; }

int area_signed(int w, int h) {
    if (w < 0 || h < 0) throw std::invalid_argument{"negative side"};
    return w * h;
}

}  // namespace

// ES.10: in `int* p, q;` the * applies to p only, so q is a plain int.
TEST(CoreGuidelinesES, ES10_StarBindsToOneDeclarator) {
    int value = 3;
    int* p = &value, q = value;
    static_assert(std::is_same_v<decltype(p), int*>);
    static_assert(std::is_same_v<decltype(q), int>);
    EXPECT_EQ(*p, q);
}

// ES.11: auto deduces const_iterator from a const map, which doesn't convert to iterator.
TEST(CoreGuidelinesES, ES11_AutoDeducesTheRightIterator) {
    using Map = std::map<std::string, int>;
    const Map m{{"a", 1}};
    auto it = m.begin();
    static_assert(std::is_same_v<decltype(it), Map::const_iterator>);
    static_assert(!std::is_convertible_v<Map::const_iterator, Map::iterator>);
    EXPECT_EQ(it->second, 1);
}

// ES.20: an initialized object has a known value; {} and = {} zero-initialize.
TEST(CoreGuidelinesES, ES20_InitializedObjectsHaveKnownValues) {
    int n{};
    int buf[4] = {};
    EXPECT_EQ(n, 0);
    for (int b : buf) EXPECT_EQ(b, 0);
}

// ES.23: {} rejects narrowing; vector{3} has one element, vector(3) three; auto x{7} is an int.
TEST(CoreGuidelinesES, ES23_BracesRejectNarrowing) {
    static_assert(!brace_init_from<int, double>);
    static_assert(!brace_init_from<char, int>);
    static_assert(brace_init_from<long long, int>);
    EXPECT_EQ(std::vector<int>(3).size(), 3u);
    EXPECT_EQ(std::vector<int>{3}.size(), 1u);
    auto x{7};
    auto list = {7};
    static_assert(std::is_same_v<decltype(x), int>);
    static_assert(std::is_same_v<decltype(list), std::initializer_list<int>>);
    EXPECT_EQ(x, 7);
}

// ES.24: unique_ptr frees its object on an early return and when an exception leaves the scope.
TEST(CoreGuidelinesES, ES24_UniquePtrFreesOnEveryPath) {
    destroyed = 0;
    EXPECT_FALSE(load_tracked(true));
    EXPECT_EQ(destroyed, 1);
    EXPECT_THROW(load_tracked(false), std::runtime_error);
    EXPECT_EQ(destroyed, 2);
}

// ES.27: a built-in array decays to a pointer; std::array doesn't and keeps its size.
TEST(CoreGuidelinesES, ES27_StdArrayDoesNotDecay) {
    static_assert(std::is_convertible_v<int (&)[3], int*>);
    static_assert(!std::is_convertible_v<std::array<int, 3>&, int*>);
    std::array<int, 3> a{1, 2, 3};
    EXPECT_EQ(a.size(), 3u);
}

// ES.28: an immediately invoked lambda can compute a value that is then const.
TEST(CoreGuidelinesES, ES28_LambdaInitializesConst) {
    const std::vector<int> sizes{3, 8, 5, 6};
    const int largest_even = [&] {
        int best = 0;
        for (int s : sizes)
            if (s % 2 == 0 && s > best) best = s;
        return best;
    }();
    static_assert(std::is_const_v<decltype(largest_even)>);
    EXPECT_EQ(largest_even, 8);
}

// ES.31: a function-like macro pastes text, so SQUARE(1 + 2) is 5; a constexpr function gives 9.
TEST(CoreGuidelinesES, ES31_MacroArgumentsArePastedText) {
#define CPPDOCS_ES31_SQUARE(x) x * x
    EXPECT_EQ(CPPDOCS_ES31_SQUARE(1 + 2), 5);
#undef CPPDOCS_ES31_SQUARE
    static_assert(square(1 + 2) == 9);
}

// ES.34: a variadic template checks and uses each argument's real type, std::string included.
TEST(CoreGuidelinesES, ES34_VariadicTemplateIsTypeSafe) {
    EXPECT_EQ(join("a", std::string{"b"}, 'c'), "abc");
    EXPECT_EQ(join(), "");
}

// ES.41: != binds tighter than &, so flags & mask != 0 means flags & (mask != 0).
TEST(CoreGuidelinesES, ES41_ComparisonBindsTighterThanBitAnd) {
    const unsigned flags = 2;
    const unsigned mask = 2;
    EXPECT_EQ(flags & static_cast<unsigned>(mask != 0), 0u);   // what the unparenthesized form computes
    EXPECT_TRUE(is_set(flags, mask));                          // (flags & mask) != 0
}

// ES.42: a span carries its size, so a function can visit exactly the elements it was given.
TEST(CoreGuidelinesES, ES42_SpanCarriesItsSize) {
    std::array<int, 4> a{1, 2, 3, 4};
    std::span<int> all{a};
    EXPECT_EQ(all.size(), 4u);
    clear(all.subspan(1));
    EXPECT_EQ(a, (std::array<int, 4>{1, 0, 0, 0}));
}

// ES.44: a braced initializer list and (since C++17) a << chain are evaluated left to right.
TEST(CoreGuidelinesES, ES44_BracedListAndShiftChainsAreOrdered) {
    int counter = 0;
    auto next = [&] { return ++counter; };
    std::array<int, 2> ids{next(), next()};
    EXPECT_EQ(ids[0], 1);
    EXPECT_EQ(ids[1], 2);
    std::ostringstream os;
    os << next() << next();
    EXPECT_EQ(os.str(), "34");
}

// ES.46: braces reject int to char; an explicit conversion truncates; a checked one throws.
TEST(CoreGuidelinesES, ES46_LossyConversionsAreRejectedOrChecked) {
    static_assert(!brace_init_from<char, int>);
    EXPECT_EQ(static_cast<int>(7.9), 7);
    EXPECT_EQ(to_char(65), 'A');
    EXPECT_THROW(to_char(300), std::out_of_range);
}

// ES.47: 0 picks the int overload; nullptr picks the pointer overload and never converts to int.
TEST(CoreGuidelinesES, ES47_NullptrPicksThePointerOverload) {
    EXPECT_EQ(pick(0), 1);
    EXPECT_EQ(pick(nullptr), 2);
    static_assert(!std::is_convertible_v<std::nullptr_t, int>);
}

// ES.48: std::bit_cast reads a double's bits without a pointer cast; std::ignore discards a [[nodiscard]] result.
TEST(CoreGuidelinesES, ES48_BitCastInsteadOfPointerCast) {
    std::ignore = status();
    CPPDOCS_REQUIRE(__cpp_lib_bit_cast);
#ifdef __cpp_lib_bit_cast
    if constexpr (std::numeric_limits<double>::is_iec559) {
        EXPECT_EQ(std::bit_cast<std::uint64_t>(2.0), 0x4000000000000000ull);
    }
#endif
}

// ES.49: static_cast refuses unrelated class pointers, which a C-style cast would accept.
TEST(CoreGuidelinesES, ES49_NamedCastsRejectTheWrongKind) {
    static_assert(!static_castable<Unrelated*, Account*>);
    static_assert(static_castable<Savings*, Account*>);
    Account plain;
    Savings savings;
    Account* a = &plain;
    Account* s = &savings;
    EXPECT_EQ(dynamic_cast<Savings*>(a), nullptr);
    EXPECT_EQ(dynamic_cast<Savings*>(s), &savings);
}

// ES.50: a mutable cache can be updated from a const member function without const_cast.
TEST(CoreGuidelinesES, ES50_MutableInsteadOfConstCast) {
    const Prices prices;
    EXPECT_DOUBLE_EQ(prices.lookup(2), 3.0);
    EXPECT_DOUBLE_EQ(prices.lookup(2), 3.0);
    EXPECT_EQ(prices.computed, 1);
    static_assert(std::is_invocable_v<decltype(&Prices::lookup), const Prices&, int>);
}

// ES.56: std::move on a const object yields const&&, so it copies; moving a unique_ptr empties it.
TEST(CoreGuidelinesES, ES56_MoveOnConstCopies) {
    const std::string kept = "a string long enough to live on the heap";
    static_assert(std::is_same_v<decltype(std::move(kept)), const std::string&&>);
    std::string copy = std::move(kept);
    EXPECT_EQ(kept, copy);
    auto w = std::make_unique<int>(1);
    auto taken = std::move(w);
    EXPECT_FALSE(w);
    EXPECT_EQ(*taken, 1);
}

// ES.62: std::less gives a consistent order even for pointers into different arrays.
TEST(CoreGuidelinesES, ES62_StdLessOrdersAnyPointers) {
    int first[2]{};
    int second[2]{};
    std::less<const int*> before;
    EXPECT_NE(before(first, second), before(second, first));
    EXPECT_TRUE(&first[0] < &first[1]);
}

// ES.63: passing a derived object by value slices it, losing the override; a non-copyable base can't be sliced.
TEST(CoreGuidelinesES, ES63_CopyingSlicesTheDerivedPart) {
    SquareByValue sq;
    EXPECT_DOUBLE_EQ(area_by_value(sq), 0.0);
    EXPECT_DOUBLE_EQ(area_by_ref(sq), 4.0);
    static_assert(!std::is_copy_constructible_v<Shape>);
    static_assert(!std::is_copy_assignable_v<Shape>);
}

// ES.64: T{e} rejects narrowing, while T(e) silently truncates.
TEST(CoreGuidelinesES, ES64_BraceConstructionRejectsNarrowing) {
    static_assert(!brace_init_from<int, double>);
    static_assert(brace_init_from<std::int64_t, int>);
    const double d = 7.9;
    EXPECT_EQ(int(d), 7);
    EXPECT_EQ(std::lround(d), 8);
}

// ES.84: T{args}; makes a temporary destroyed at the end of the statement; a named object lives to the end of the scope.
TEST(CoreGuidelinesES, ES84_UnnamedTemporaryDiesImmediately) {
    int held = 0;
    Guard{held};
    EXPECT_EQ(held, 0);
    {
        Guard guard{held};
        EXPECT_EQ(held, 1);
    }
    EXPECT_EQ(held, 0);
}

// ES.100: int op unsigned is done in unsigned, so -3 - 7u wraps; std::cmp_less compares the real values.
TEST(CoreGuidelinesES, ES100_MixedArithmeticIsUnsigned) {
    int balance = -3;
    unsigned fee = 7;
    static_assert(std::is_same_v<decltype(balance - fee), unsigned>);
    EXPECT_EQ(balance - fee, std::numeric_limits<unsigned>::max() - 9);
    const std::vector<int> v{1, 2};
    EXPECT_TRUE(std::cmp_less(-1, v.size()));
    EXPECT_FALSE(std::cmp_greater(-1, 0u));
    const std::vector<int> empty;
    EXPECT_EQ(std::ssize(empty) - 1, -1);
}

// ES.101: unsigned shifts bring in zeros; signed right shift copies the sign bit; ~ on uint8_t yields an int.
TEST(CoreGuidelinesES, ES101_UnsignedForBitManipulation) {
    std::uint32_t word = 0xF000'0000u;
    EXPECT_EQ(word >> 28, 0xFu);
    std::int32_t negative = std::numeric_limits<std::int32_t>::min();
    EXPECT_EQ(negative >> 28, -8);   // arithmetic shift, defined since C++20
    std::uint8_t b = 0b1010'1010;
    static_assert(std::is_same_v<decltype(~b), int>);
    EXPECT_EQ(~b, -171);
    EXPECT_EQ(invert(b), 0b0101'0101);
}

// ES.102: unsigned 5 - 7 wraps to a large value; signed 5 - 7 is -2.
TEST(CoreGuidelinesES, ES102_UnsignedSubtractionWraps) {
    unsigned in_stock = 5;
    unsigned ordered = 7;
    EXPECT_EQ(in_stock - ordered, std::numeric_limits<unsigned>::max() - 1);
    int s_in_stock = 5;
    int s_ordered = 7;
    EXPECT_EQ(s_in_stock - s_ordered, -2);
}

// ES.103: a wider type or a check before the operation avoids signed overflow; unsigned wraps instead.
TEST(CoreGuidelinesES, ES103_CheckOrWidenBeforeOverflow) {
    static_assert(wide_area(100'000, 100'000) == 10'000'000'000);
    static_assert(!checked_add(std::numeric_limits<int>::max(), 1));
    static_assert(!checked_add(std::numeric_limits<int>::min(), -1));
    static_assert(*checked_add(40, 2) == 42);
    unsigned top = std::numeric_limits<unsigned>::max();
    ++top;
    EXPECT_EQ(top, 0u);
    CPPDOCS_REQUIRE(__cpp_lib_saturation_arithmetic);
    // C++26 renamed add_sat to saturating_add (macro 202603L; GCC 16). Older
    // libraries (202311L: GCC 15) have only the first name (#269).
#if defined(__cpp_lib_saturation_arithmetic) && __cpp_lib_saturation_arithmetic >= 202603L
    EXPECT_EQ(std::saturating_add(std::numeric_limits<int>::max(), 1), std::numeric_limits<int>::max());
#elif defined(__cpp_lib_saturation_arithmetic)
    EXPECT_EQ(std::add_sat(std::numeric_limits<int>::max(), 1), std::numeric_limits<int>::max());
#endif
}

// ES.104: a countdown that stops at 0 never reads index -1; unsigned 0 - 1 wraps to the maximum.
TEST(CoreGuidelinesES, ES104_CountdownStopsAtZero) {
    static_assert(sum_backwards() == 6);
    unsigned zero = 0;
    --zero;
    EXPECT_EQ(zero, std::numeric_limits<unsigned>::max());
}

// ES.105: checking the divisor first handles zero; floating-point division by zero gives infinity.
TEST(CoreGuidelinesES, ES105_CheckTheDivisor) {
    static_assert(!per_person(10, 0));
    static_assert(*per_person(10, 2) == 5);
    if constexpr (std::numeric_limits<double>::is_iec559) {
        auto divide = [](double a, double b) { return a / b; };
        EXPECT_TRUE(std::isinf(divide(1.0, 0.0)));
    }
}

// ES.106: unsigned doesn't prevent negatives: -2 converts to 2^32 - 2 and the area wraps; a signed check catches it.
TEST(CoreGuidelinesES, ES106_UnsignedDoesNotPreventNegatives) {
    const int height = -2;
    const unsigned as_unsigned = static_cast<unsigned>(height);
    EXPECT_EQ(as_unsigned, std::numeric_limits<unsigned>::max() - 1);
    EXPECT_EQ(area_unsigned(as_unsigned, 2u), std::numeric_limits<unsigned>::max() - 3);
    EXPECT_THROW(area_signed(height, 2), std::invalid_argument);
    EXPECT_EQ(area_signed(3, 2), 6);
}

// ES.107: size() - 1 on an empty vector wraps; a signed index from std::ssize counts down safely.
TEST(CoreGuidelinesES, ES107_SignedIndexCountsDownSafely) {
    const std::vector<int> empty;
    EXPECT_EQ(empty.size() - 1, std::numeric_limits<std::size_t>::max());
    static_assert(std::is_signed_v<decltype(std::ssize(empty))>);
    const std::vector<int> v{1, 2, 3};
    std::vector<int> reversed;
    for (std::ptrdiff_t i = std::ssize(v) - 1; i >= 0; --i) reversed.push_back(v[static_cast<std::size_t>(i)]);
    EXPECT_EQ(reversed, (std::vector<int>{3, 2, 1}));
    int visits = 0;
    for (std::ptrdiff_t i = std::ssize(empty) - 1; i >= 0; --i) ++visits;
    EXPECT_EQ(visits, 0);
}
