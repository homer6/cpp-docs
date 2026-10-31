// Checks claims made by docs/examples/language/templates/templates-basics.md.
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {
template <class T> T max_of(T a, T b) { return a > b ? a : b; }

template <class T> class Box {
    T value_;
public:
    explicit Box(T v) : value_{std::move(v)} {}
    const T& get() const { return value_; }
};

template <class T, std::size_t N> struct Array { T data[N]; };
template <int Base> int scaled(int x) { return x * Base; }
template <double Scale> constexpr double times(double x) { return x * Scale; }  // C++20 FP NTTP
struct Point { int x, y; };
template <Point P> constexpr int sum_of() { return P.x + P.y; }                 // class-type NTTP

template <std::integral T> constexpr const char* half_kind(T) { return "integral"; }
template <class T> requires std::floating_point<T> constexpr const char* half_kind(T) { return "floating"; }
template <std::integral T> T half(T x) { return x / 2; }
template <class T> requires std::floating_point<T> T half(T x) { return x / 2; }

template <class T> typename T::value_type first(const T& c) {  // `typename` disambiguator
    typename T::value_type x = c.front();
    return x;
}
struct Holder { template <class U> U as() const { return U{7}; } };
template <class H> int call_as(const H& h) { return h.template as<int>(); }  // `template` disambiguator

template <class T> concept HalfAccepts = requires(T t) { half(t); };
}  // namespace

// "Function templates": T is deduced from the arguments or supplied explicitly.
TEST(TemplatesBasics, FunctionTemplateDeduction) {
    static_assert(std::is_same_v<decltype(max_of(3, 7)), int>);
    static_assert(std::is_same_v<decltype(max_of(2.5, 1.0)), double>);
    static_assert(std::is_same_v<decltype(max_of<long>(3, 7)), long>);
    EXPECT_EQ(max_of(3, 7), 7);
    EXPECT_EQ(max_of(2.5, 1.0), 2.5);
}

// "Class templates": Box<int>, and CTAD deduces Box<std::string> from "hi"s.
TEST(TemplatesBasics, ClassTemplateAndCtad) {
    using namespace std::string_literals;
    Box<int> b{42};
    Box bs{"hi"s};
    static_assert(std::is_same_v<decltype(bs), Box<std::string>>);
    EXPECT_EQ(b.get(), 42);
    EXPECT_EQ(bs.get(), "hi");
}

// "Non-type template parameters": sizes, values, and (C++20) floating-point and class types.
TEST(TemplatesBasics, NonTypeTemplateParameters) {
    static_assert(sizeof(Array<int, 4>) == 4 * sizeof(int));
    EXPECT_EQ(scaled<10>(5), 50);
    static_assert(times<2.5>(2.0) == 5.0);
    static_assert(sum_of<Point{1, 2}>() == 3);
}

// "Constraining templates": the integral and floating_point overloads are picked by concept.
TEST(TemplatesBasics, ConstrainedOverloads) {
    static_assert(half_kind(4) == std::string_view{"integral"});
    static_assert(half_kind(4.0) == std::string_view{"floating"});
    EXPECT_EQ(half(7), 3);
    EXPECT_EQ(half(7.0), 3.5);
    static_assert(!HalfAccepts<std::string>);  // neither overload accepts a string
}

// "Gotchas": `typename T::value_type` and `obj.template method<U>()` in dependent contexts.
TEST(TemplatesBasics, Disambiguators) {
    EXPECT_EQ(first(std::vector<int>{4, 5}), 4);
    EXPECT_EQ(call_as(Holder{}), 7);
}
