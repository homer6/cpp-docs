// Checks claims made by docs/guidelines/core-guidelines/examples/templates.md.
#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <format>
#include <forward_list>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>
#include <memory>
#include <memory_resource>
#include <numbers>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

// T.1
template <class T>
concept Number = std::regular<T> && requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
    { a - b } -> std::convertible_to<T>;
    { a * b } -> std::convertible_to<T>;
    { a / b } -> std::convertible_to<T>;
};

template <std::ranges::input_range R>
    requires Number<std::ranges::range_value_t<R>>
auto total(const R& r) {
    std::ranges::range_value_t<R> s{};
    for (const auto& x : r) s += x;
    return s;
}

template <class R>
concept can_total = requires(const R& r) { total(r); };

// T.2
template <std::input_iterator I, std::sentinel_for<I> S, class V>
    requires std::equality_comparable_with<std::iter_reference_t<I>, const V&>
I find_value(I first, S last, const V& value) {
    for (; first != last; ++first)
        if (*first == value) return first;
    return first;
}

// T.3
template <std::semiregular T>
class Buffer {
public:
    explicit Buffer(std::size_t n) : items_(n) {}
    T& operator[](std::size_t i) { return items_[i]; }
    std::size_t size() const { return items_.size(); }

private:
    std::vector<T> items_;
};

// T.4
namespace et {
template <class E>
concept VecExpr = requires(const E& e, std::size_t i) {
    { e[i] } -> std::convertible_to<double>;
    { e.size() } -> std::convertible_to<std::size_t>;
};

struct Vec {
    std::vector<double> v;
    double operator[](std::size_t i) const { return v[i]; }
    std::size_t size() const { return v.size(); }
    Vec(std::initializer_list<double> il) : v(il) {}
    template <VecExpr E>
    Vec(const E& e) : v(e.size()) {
        for (std::size_t i = 0; i < v.size(); ++i) v[i] = e[i];
    }
};

template <class L, class R>
struct Sum {
    std::conditional_t<std::is_same_v<L, Vec>, const Vec&, L> l;
    std::conditional_t<std::is_same_v<R, Vec>, const Vec&, R> r;
    double operator[](std::size_t i) const { return l[i] + r[i]; }
    std::size_t size() const { return l.size(); }
};

template <VecExpr L, VecExpr R>
Sum<L, R> operator+(const L& l, const R& r) { return {l, r}; }
}  // namespace et

// T.5
struct Shape {
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

template <class T>
    requires requires(const T& t) { { t.area() } -> std::convertible_to<double>; }
class ShapeModel final : public Shape {
public:
    explicit ShapeModel(T t) : t_(std::move(t)) {}
    double area() const override { return t_.area(); }

private:
    T t_;
};

template <class T>
std::unique_ptr<Shape> make_shape(T t) { return std::make_unique<ShapeModel<T>>(std::move(t)); }

struct Rect { double w, h; double area() const { return w * h; } };
struct Square { double s; double area() const { return s * s; } };

// T.10
template <Number T>
T twice(T x) { return x + x; }
template <class T>
concept can_twice = requires(T t) { twice(t); };

// T.11
template <class C>
concept HomemadeSortable = requires(C& c) { c.begin(); c.end(); };
template <class R>
concept std_sortable = std::ranges::random_access_range<R> && std::sortable<std::ranges::iterator_t<R>>;

// T.13
template <std::integral T>
T half(T x) { return x / 2; }
template <typename T>
    requires std::integral<T>
T half_long(T x) { return x / 2; }
template <class T>
concept can_half = requires(T t) { half(t); };

// T.20
template <class T>
concept Addable = requires(T a, T b) { a + b; };

// T.21
struct Money {
    long cents;
    friend Money operator+(Money a, Money b) { return {a.cents + b.cents}; }
    friend Money operator-(Money a, Money b) { return {a.cents - b.cents}; }
    Money& operator+=(Money b) { cents += b.cents; return *this; }
    Money& operator-=(Money b) { cents -= b.cents; return *this; }
    friend auto operator<=>(const Money&, const Money&) = default;
};
struct HalfMoney {   // the BAD version: + only
    long cents;
    friend HalfMoney operator+(HalfMoney a, HalfMoney b) { return {a.cents + b.cents}; }
};
template <class T>
concept Additive = requires(T a, T b) { a + b; a - b; a += b; a -= b; };

// T.22
template <class T>
concept Monoid = std::regular<T> && requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
};
template <Monoid T>
bool monoid_laws_hold(const T& a, const T& b, const T& c) {
    return (a + b) + c == a + (b + c) && a + T{} == a && T{} + a == a;
}

// T.23
template <class T> concept AShape = requires(const T& t) { t.area(); };
template <class T> concept APolygon = AShape<T> && requires(const T& t) { t.sides(); };
struct Circle { double area() const { return 3.0; } };
struct Quad { double area() const { return 4.0; } int sides() const { return 4; } };
int describe(const AShape auto&) { return 0; }
int describe(const APolygon auto&) { return 1; }

// T.24
template <class R> inline constexpr bool enable_sorted = false;
template <class R>
concept SortedRange = std::ranges::forward_range<R> && enable_sorted<std::remove_cvref_t<R>>;
struct SortedInts {
    std::vector<int> v;
    auto begin() const { return v.begin(); }
    auto end() const { return v.end(); }
};
template <> inline constexpr bool enable_sorted<SortedInts> = true;
enum class Search { linear, binary };
Search search_kind(const std::ranges::forward_range auto&) { return Search::linear; }
Search search_kind(const SortedRange auto&) { return Search::binary; }

// T.25
template <class T> int kind(T) { return 0; }
template <class T> requires std::integral<T> int kind(T) { return 1; }
template <class T> requires std::floating_point<T> int kind(T) { return 2; }

// T.26
template <class T>
concept TraitEquality =
    std::is_same_v<decltype(std::declval<const T&>() == std::declval<const T&>()), bool>;
template <class T>
concept UseEquality = requires(const T& a, const T& b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};
struct Truth { bool v; operator bool() const { return v; } };
struct Bits {
    int v;
    friend Truth operator==(const Bits& a, const Bits& b) { return {a.v == b.v}; }
    friend Truth operator!=(const Bits& a, const Bits& b) { return {a.v != b.v}; }
};

// T.30
template <class T> concept NegText = !std::is_arithmetic_v<T>;
template <class T> concept Text = std::convertible_to<const T&, std::string_view>;
template <class T> concept SmallInt = std::integral<T> && sizeof(T) == 1 && !std::same_as<T, bool>;

// T.31
template <class T>
concept ListStringish = std::same_as<T, std::string> || std::same_as<T, std::string_view> ||
                        std::same_as<T, const char*>;
template <class T> concept Stringish = std::convertible_to<const T&, std::string_view>;
template <class T> concept Arithmetic = std::integral<T> || std::floating_point<T>;

// T.40
bool above_seven(int x) { return x > 7; }
bool below_three(int x) { return x < 3; }

// T.41
template <class T>
concept Printable = requires(std::ostream& os, const T& t) { os << t; };
template <std::ranges::forward_range R>
    requires std::totally_ordered<std::ranges::range_value_t<R>>
auto largest(const R& r) { return *std::ranges::max_element(r); }
struct Version {
    int major, minor;
    auto operator<=>(const Version&) const = default;
};

// T.42
template <class R> using Elem = std::ranges::range_value_t<R>;
template <class C>
Elem<C> first_or_default(const C& c) {
    Elem<C> x{};
    if (!c.empty()) x = *c.begin();
    return x;
}

// T.43
template <class T> using List = std::vector<T>;
using Handler = void (*)(int);

// T.47
namespace geo_bad {
struct Shape {};
struct Circle : Shape {};
template <class T>
std::string describe(const T&) { return "something"; }
}  // namespace geo_bad
namespace app_bad {
[[maybe_unused]] std::string describe(const geo_bad::Shape&) { return "a shape"; }   // never chosen: the point of T.47
std::string f() { return describe(geo_bad::Circle{}); }
}  // namespace app_bad
namespace geo_good {
struct Shape {};
struct Circle : Shape {};
template <class T>
    requires requires(const T& t) { t.name(); }
std::string describe(const T& t) { return t.name(); }
}  // namespace geo_good
namespace app_good {
std::string describe(const geo_good::Shape&) { return "a shape"; }
std::string f() { return describe(geo_good::Circle{}); }
}  // namespace app_good

// T.48
template <class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
T gcd(T a, T b) { return b == 0 ? a : gcd(b, a % b); }
template <class T>
concept can_gcd = requires(T a) { gcd(a, a); };

// T.49
template <std::regular_invocable<int> F>
void transform_all(std::vector<int>& v, F f) {
    for (int& x : v) x = std::invoke(f, x);
}
void transform_erased(std::vector<int>& v, const std::function<int(int)>& f) {
    for (int& x : v) x = f(x);
}

// T.60
template <class T>
T clamp01(T x) { return std::clamp(x, T{0}, T{1}); }

// T.61
template <class T> struct Pool : std::allocator<T> {};
template <class T> struct Node { T value; Node* next; };
template <class T, class Alloc = std::allocator<T>>
struct ScaryList {
    using node_type = Node<T>;
    Node<T>* head = nullptr;
};
template <class T, class Alloc = std::allocator<T>>
struct NestedList {
    struct Node { T value; Node* next; };
};

// T.62
struct ParserBase { enum class Status { ok, error }; };
template <class T> struct Parser : ParserBase {};
template <class T> struct NestedParser { enum class Status { ok, error }; };

// T.64
template <class T>
struct Codec {
    static_assert(std::is_trivially_copyable_v<T>);
    static std::vector<char> encode(const T& v) {
        std::vector<char> out(sizeof v);
        std::memcpy(out.data(), &v, sizeof v);
        return out;
    }
    static T decode(const std::vector<char>& bytes) {
        T v;
        std::memcpy(&v, bytes.data(), sizeof v);
        return v;
    }
};
template <>
struct Codec<std::string> {
    static std::vector<char> encode(const std::string& s) { return {s.begin(), s.end()}; }
    static std::string decode(const std::vector<char>& bytes) { return {bytes.begin(), bytes.end()}; }
};

// T.65: returns how many increments it took
template <class It>
int skip(It& it, int n, std::random_access_iterator_tag) { it += n; return 1; }
template <class It>
int skip(It& it, int n, std::input_iterator_tag) {
    int steps = 0;
    for (; n > 0; --n, ++steps) ++it;
    return steps;
}
template <class It>
int skip(It& it, int n) { return skip(it, n, typename std::iterator_traits<It>::iterator_category{}); }

// T.67
template <class T>
struct Same {
    bool operator()(const T& a, const T& b) const { return a == b; }
};
template <>
struct Same<const char*> {
    bool operator()(const char* a, const char* b) const { return std::strcmp(a, b) == 0; }
};
template <class T>
struct NaiveSame {
    bool operator()(const T& a, const T& b) const { return a == b; }
};

// T.68
template <class T>
T make_default() {
    T result{};
    return result;
}
template <class T, class U>
concept brace_converts = requires(const U& u) { T{u}; };
template <class T, class U>
concept paren_converts = requires(const U& u) { T(u); };

// T.69
namespace lib {
namespace detail { template <class T> int normalize(T) { return 1; } }
template <class T> int process(T v) { return detail::normalize(v); }
template <class T> void reorder(T& a, T& b) {
    using std::swap;
    swap(a, b);
}
}  // namespace lib
namespace user {
struct Angle { double deg; };
template <class T> int normalize(T) { return 2; }
struct Counted { int v; };
int swaps = 0;
void swap(Counted& a, Counted& b) { ++swaps; std::swap(a.v, b.v); }
}  // namespace user

// T.80
template <class T>
struct Container {
    std::vector<T> items;
    std::size_t size() const { return items.size(); }
};
template <class T>
void sort(Container<T>& c) { std::ranges::sort(c.items); }
struct Point { int x, y; };

// T.81
struct Base { int id = 0; };
struct Derived : Base { double extra = 0; };
template <std::derived_from<Base> T, std::size_t N>
void reset(std::span<T, N> items) { for (T& x : items) x.id = 0; }

// T.82
struct Core {
    long handled = 0;
    void handle(int) { ++handled; }
};
template <class Next>
struct Counting : Next {
    long seen = 0;
    void handle(int e) { ++seen; Next::handle(e); }
};
template <class Next>
struct Filtering : Next {
    void handle(int e) { if (e >= 0) Next::handle(e); }
};
using Pipeline = Filtering<Counting<Core>>;

// T.83
class Sink {
public:
    virtual ~Sink() = default;
    template <class T>
    void write(const T& value) { write_text(std::format("{}", value)); }

private:
    virtual void write_text(std::string_view text) = 0;
};
class Collect final : public Sink {
public:
    std::vector<std::string> lines;

private:
    void write_text(std::string_view text) override { lines.emplace_back(text); }
};

// T.84
struct NodeBase { NodeBase* next = nullptr; };
class ListBase {
protected:
    NodeBase* head_ = nullptr;
    std::size_t size_ = 0;
    void link_front(NodeBase* n) { n->next = head_; head_ = n; ++size_; }

public:
    std::size_t size() const { return size_; }
};
template <class T>
struct ValueNode : NodeBase { T value; explicit ValueNode(const T& v) : value(v) {} };
template <class T>
class StableList : public ListBase {
public:
    StableList() = default;
    StableList(const StableList&) = delete;
    StableList& operator=(const StableList&) = delete;
    void push_front(const T& v) { link_front(new ValueNode<T>(v)); }
    T& front() { return static_cast<ValueNode<T>*>(head_)->value; }
    ~StableList() {
        while (head_) {
            auto* n = static_cast<ValueNode<T>*>(head_);
            head_ = n->next;
            delete n;
        }
    }
};

// T.100
template <class... Ts>
std::string join(const Ts&... args) {
    std::ostringstream out;
    ((out << args), ...);
    return out.str();
}

// T.101
struct Holder {
    std::unique_ptr<int> p;
    explicit Holder(std::unique_ptr<int> q) : p(std::move(q)) {}
};
template <class T, class... Args>
T build(Args&&... args) { return T(std::forward<Args>(args)...); }
struct Probe {
    bool from_lvalue;
    explicit Probe(int&) : from_lvalue(true) {}
    explicit Probe(int&&) : from_lvalue(false) {}
};

// T.102
template <class... Ts>
std::size_t total_size(const Ts&...) { return (std::size_t{0} + ... + sizeof(Ts)); }
template <class... Ts>
constexpr bool all_integral = (std::is_integral_v<Ts> && ...);

// T.103
int sum(std::initializer_list<int> xs) {
    int s = 0;
    for (int x : xs) s += x;
    return s;
}
template <class T>
concept sum_accepts = requires(T t) { sum({1, t}); };

// T.120
template <class T, class... Ts>
concept one_of = (std::same_as<T, Ts> || ...);
template <class T, class... Ts> struct tmp_one_of : std::false_type {};
template <class T, class U, class... Ts>
struct tmp_one_of<T, U, Ts...>
    : std::conditional_t<std::is_same_v<T, U>, std::true_type, tmp_one_of<T, Ts...>> {};

// T.121
int step(std::forward_iterator auto) { return 1; }
int step(std::random_access_iterator auto) { return 2; }

// T.122
template <class T>
using In = std::conditional_t<std::is_trivially_copyable_v<T> && sizeof(T) <= 2 * sizeof(void*), T,
                              const T&>;

// T.123
constexpr long power(long base, int exp) {
    long r = 1;
    while (exp-- > 0) r *= base;
    return r;
}
template <long B, int E> struct Power { static constexpr long value = B * Power<B, E - 1>::value; };
template <long B> struct Power<B, 0> { static constexpr long value = 1; };

// T.124
template <class T> struct strip_ref { using type = T; };
template <class T> struct strip_ref<T&> { using type = T; };

// T.140
struct ShorterFirst {
    bool operator()(const std::string& a, const std::string& b) const { return a.size() < b.size(); }
};

// T.142?
template <std::floating_point T>
inline constexpr T tau = 2 * std::numbers::pi_v<T>;

// T.143
template <class It>
int count_all(It first, It last) {
    int n = 0;
    for (auto i = first; i != last; ++i) ++n;
    return n;
}
template <class It>
concept less_comparable_iter = requires(It a, It b) { a < b; };

// T.144
template <class T> int spec_which(T) { return 0; }
template <> [[maybe_unused]] int spec_which<int*>(int*) { return 1; }   // never chosen: the point of T.144
template <class T> int spec_which(T*) { return 2; }

template <class T> int over_which(T) { return 0; }
template <class T> int over_which(T*) { return 2; }
int over_which(int*) { return 1; }

// T.150
struct Temperature {
    double deg;
    auto operator<=>(const Temperature&) const = default;
};
struct PlainTemperature { double deg; };

}  // namespace

// T.1: one template sums any range of numbers; it refuses a range of strings.
TEST(CoreGuidelinesT, T1_OneTemplateForEveryNumberRange) {
    EXPECT_EQ(total(std::vector<int>{1, 2, 3}), 6);
    EXPECT_DOUBLE_EQ(total(std::list<double>{0.5, 1.5}), 2.0);
    static_assert(std::is_same_v<decltype(total(std::vector<long>{})), long>);
    static_assert(can_total<std::vector<int>> && !can_total<std::vector<std::string>>);
}

// T.2: one find algorithm works for a list, a vector and an array of strings.
TEST(CoreGuidelinesT, T2_OneAlgorithmManyContainers) {
    std::list<int> l{3, 1, 4};
    std::vector<int> v{3, 1, 4};
    const std::string a[] = {"x", "y"};
    EXPECT_EQ(*find_value(l.begin(), l.end(), 4), 4);
    EXPECT_EQ(find_value(v.begin(), v.end(), 9), v.end());
    EXPECT_EQ(find_value(std::begin(a), std::end(a), std::string{"y"}) - std::begin(a), 1);
}

// T.3: a container template keeps the element type: element access is typed, no casts.
TEST(CoreGuidelinesT, T3_ContainerKnowsItsElementType) {
    Buffer<double> b(4);
    b[2] = 1.5;
    static_assert(std::is_same_v<decltype(b[0]), double&>);
    EXPECT_EQ(b.size(), 4u);
    EXPECT_DOUBLE_EQ(b[2], 1.5);
}

// T.4: a + b + c builds an expression tree (no temporary Vec) and evaluates in one pass.
TEST(CoreGuidelinesT, T4_ExpressionTemplateBuildsTree) {
    et::Vec a{1, 2}, b{10, 20}, c{100, 200};
    static_assert(std::is_same_v<decltype(a + b + c), et::Sum<et::Sum<et::Vec, et::Vec>, et::Vec>>);
    et::Vec d = a + b + c;
    EXPECT_DOUBLE_EQ(d[0], 111);
    EXPECT_DOUBLE_EQ(d[1], 222);
}

// T.5: a template generates the implementations of a virtual interface for any type with area().
TEST(CoreGuidelinesT, T5_TemplateImplementsVirtualInterface) {
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(make_shape(Rect{2, 3}));
    shapes.push_back(make_shape(Square{4}));
    double sum_areas = 0;
    for (const auto& s : shapes) sum_areas += s->area();
    EXPECT_DOUBLE_EQ(sum_areas, 22.0);
    static_assert(std::is_base_of_v<Shape, ShapeModel<Rect>> && std::is_base_of_v<Shape, ShapeModel<Square>>);
}

// T.10: a concept on the parameter rejects a wrong argument at the call (it's not callable).
TEST(CoreGuidelinesT, T10_ConceptsConstrainArguments) {
    EXPECT_EQ(twice(21), 42);
    static_assert(can_twice<int> && can_twice<double>);
    static_assert(!can_twice<std::vector<int>> && !can_twice<std::string>);
}

// T.11: std::sortable needs random access; the homemade concept wrongly accepts std::list.
TEST(CoreGuidelinesT, T11_StandardConceptsAreComplete) {
    static_assert(HomemadeSortable<std::list<int>>);
    static_assert(!std_sortable<std::list<int>>);
    static_assert(std_sortable<std::vector<int>>);
    std::vector<int> v{3, 1, 2};
    std::ranges::sort(v);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
}

// T.12: a constrained auto deduces the same type as auto, and the constraint is checked.
TEST(CoreGuidelinesT, T12_ConstrainedAutoDeducesAndChecks) {
    std::vector<int> v{1, 2, 3};
    std::unsigned_integral auto n = v.size();
    auto m = v.size();
    static_assert(std::is_same_v<decltype(n), decltype(m)>);
    static_assert(!std::unsigned_integral<double> && !std::unsigned_integral<int>);
    EXPECT_EQ(n, 3u);
    EXPECT_EQ(n, m);
}

// T.13: the shorthand and the requires-clause forms mean the same: integral only.
TEST(CoreGuidelinesT, T13_ShorthandIsEquivalent) {
    EXPECT_EQ(half(7), 3);
    EXPECT_EQ(half_long(7), 3);
    static_assert(can_half<int> && can_half<long> && !can_half<double>);
}

// T.20: the one-operator Addable accepts std::string (it concatenates); Number doesn't.
TEST(CoreGuidelinesT, T20_SingleOperationConceptAcceptsTooMuch) {
    static_assert(Addable<std::string> && Addable<int>);
    static_assert(!Number<std::string> && Number<int> && Number<double>);
    EXPECT_EQ(std::string{"4"} + std::string{"2"}, "42");
}

// T.21: a complete set of operations works; a type with only + lacks += and -.
TEST(CoreGuidelinesT, T21_CompleteOperationSet) {
    static_assert(Additive<Money> && std::totally_ordered<Money>);
    static_assert(!Additive<HalfMoney>);
    Money m{100};
    m += Money{50};
    m -= Money{30};
    EXPECT_EQ(m.cents, 120);
    EXPECT_EQ((Money{1} + Money{2}).cents, 3);
    EXPECT_EQ((Money{5} - Money{2}).cents, 3);
    EXPECT_EQ((HalfMoney{1} + HalfMoney{2}).cents, 3);
    EXPECT_TRUE(Money{1} < Money{2});
    EXPECT_TRUE(Money{2} >= Money{2});
}

// T.22: the stated laws hold for int; std::string satisfies the syntax, but its + isn't commutative.
TEST(CoreGuidelinesT, T22_AxiomsCheckedOnSamples) {
    EXPECT_TRUE(monoid_laws_hold(1, 2, 3));
    EXPECT_TRUE(monoid_laws_hold(std::string{"a"}, std::string{"b"}, std::string{"c"}));
    static_assert(Monoid<std::string>);
    const std::string a = "a", b = "b";
    EXPECT_NE(a + b, b + a);   // so a commutative algorithm must not assume it of every Monoid
}

// T.23: a concept that adds a requirement refines the other; the more refined overload wins.
TEST(CoreGuidelinesT, T23_RefinedConceptIsPreferred) {
    EXPECT_EQ(describe(Circle{}), 0);
    EXPECT_EQ(describe(Quad{}), 1);
}

// T.24: an opt-in trait separates "sorted" from a plain range; the sorted overload is chosen only for it.
TEST(CoreGuidelinesT, T24_TraitSeparatesSemantics) {
    static_assert(SortedRange<SortedInts> && !SortedRange<std::vector<int>>);
    EXPECT_EQ(search_kind(SortedInts{{1, 2, 3}}), Search::binary);
    EXPECT_EQ(search_kind(std::vector<int>{3, 1, 2}), Search::linear);
}

// T.25: a general template plus constrained special cases picks the right overload for each type.
TEST(CoreGuidelinesT, T25_GeneralPlusSpecialCases) {
    EXPECT_EQ(kind(std::string{}), 0);
    EXPECT_EQ(kind(1), 1);
    EXPECT_EQ(kind(2.5), 2);
}

// T.26: a use-pattern concept accepts == returning a type convertible to bool; the trait version doesn't.
TEST(CoreGuidelinesT, T26_UsePatternsAllowConversions) {
    static_assert(UseEquality<Bits> && !TraitEquality<Bits>);
    static_assert(UseEquality<int> && TraitEquality<int>);
    EXPECT_TRUE(Bits{1} == Bits{1});
    EXPECT_TRUE(Bits{1} != Bits{2});
}

// T.30: a concept that is a negation matches unrelated types; a positive one with a small ! doesn't.
TEST(CoreGuidelinesT, T30_NegationOnlyForMinorDifference) {
    static_assert(NegText<std::vector<int>>);   // the BAD concept's overreach
    static_assert(!Text<std::vector<int>> && Text<std::string> && Text<const char*>);
    static_assert(SmallInt<char> && SmallInt<signed char> && SmallInt<unsigned char>);
    static_assert(!SmallInt<bool> && !SmallInt<int>);
    SUCCEED();
}

// T.31: a closed list of alternatives misses char arrays and std::pmr::string; the common property doesn't.
TEST(CoreGuidelinesT, T31_DisjunctionOnlyForAlternatives) {
    static_assert(!ListStringish<char[6]> && Stringish<char[6]>);
    static_assert(Stringish<std::pmr::string> && !ListStringish<std::pmr::string>);
    static_assert(Arithmetic<int> && Arithmetic<double> && !Arithmetic<std::string>);
    SUCCEED();
}

// T.40: a lambda carries state and each has its own type; function pointers share one type.
TEST(CoreGuidelinesT, T40_FunctionObjectsCarryState) {
    const std::vector<int> v{1, 5, 8, 9};
    int limit = 4;
    auto above = [limit](int x) { return x > limit; };
    EXPECT_EQ(std::ranges::count_if(v, above), 3);
    EXPECT_EQ(std::ranges::count_if(v, above_seven), 2);
    EXPECT_EQ(std::ranges::count_if(v, below_three), 1);
    static_assert(std::is_same_v<decltype(&above_seven), decltype(&below_three)>);
    auto other = [](int x) { return x > 4; };
    static_assert(!std::is_same_v<decltype(above), decltype(other)>);
    static_assert(std::is_class_v<decltype(above)>);
    EXPECT_TRUE(other(5));
}

// T.41: requiring only an order lets a type that can't be printed use the algorithm.
TEST(CoreGuidelinesT, T41_OnlyEssentialRequirements) {
    static_assert(!Printable<Version> && std::totally_ordered<Version>);
    const Version v = largest(std::vector<Version>{{1, 2}, {2, 0}, {1, 9}});
    EXPECT_EQ(v.major, 2);
    EXPECT_EQ(v.minor, 0);
}

// T.42: an alias template names the dependent element type once.
TEST(CoreGuidelinesT, T42_AliasNamesTheType) {
    static_assert(std::is_same_v<Elem<std::vector<int>>, int>);
    static_assert(std::is_same_v<Elem<std::list<std::string>>, std::string>);
    EXPECT_EQ(first_or_default(std::vector<int>{7, 8}), 7);
    EXPECT_EQ(first_or_default(std::list<std::string>{}), "");
}

// T.43: a using alias can be a template and names the same types a typedef would.
TEST(CoreGuidelinesT, T43_UsingAliasCanBeTemplate) {
    static_assert(std::is_same_v<List<int>, std::vector<int>>);
    typedef void (*OldHandler)(int);
    static_assert(std::is_same_v<Handler, OldHandler>);
    SUCCEED();
}

// T.44: class template argument deduction and make_tuple deduce the argument types.
TEST(CoreGuidelinesT, T44_DeduceTemplateArguments) {
    std::pair p{3, 2.5};
    auto t = std::make_tuple(1, std::string{"one"});
    static_assert(std::is_same_v<decltype(p), std::pair<int, double>>);
    static_assert(std::is_same_v<decltype(t), std::tuple<int, std::string>>);
    EXPECT_EQ(std::get<1>(t), "one");
    EXPECT_EQ(p.first, 3);
}

// T.47: an unconstrained template found by ADL beats a function needing derived-to-base; a constrained one doesn't.
TEST(CoreGuidelinesT, T47_UnconstrainedTemplateHijacksCall) {
    EXPECT_EQ(app_bad::f(), "something");
    EXPECT_EQ(app_good::f(), "a shape");
}

// T.48: enable_if removes gcd from overload resolution for non-integral types.
TEST(CoreGuidelinesT, T48_EnableIfFakesConcept) {
    EXPECT_EQ(gcd(12, 18), 6);
    static_assert(can_gcd<int> && can_gcd<long> && !can_gcd<double>);
}

// T.49: std::function erases the callable's type; a template parameter keeps it. Both give the same result.
TEST(CoreGuidelinesT, T49_TemplateKeepsCallableType) {
    auto add1 = [](int x) { return x + 1; };
    auto add2 = [](int x) { return x + 2; };
    static_assert(std::is_same_v<std::function<int(int)>, decltype(std::function{add1})>);
    static_assert(std::is_same_v<decltype(std::function{add1}), decltype(std::function{add2})>);
    std::vector<int> a{1, 2}, b{1, 2};
    transform_all(a, add1);
    transform_erased(b, add1);
    EXPECT_EQ(a, b);
    EXPECT_EQ(a, (std::vector<int>{2, 3}));
    transform_all(a, add2);
    EXPECT_EQ(a, (std::vector<int>{4, 5}));
}

// T.60: a template that names std::clamp means the same thing wherever it's used.
TEST(CoreGuidelinesT, T60_QualifiedNamesInTemplates) {
    EXPECT_DOUBLE_EQ(clamp01(1.5), 1.0);
    EXPECT_DOUBLE_EQ(clamp01(-0.5), 0.0);
    EXPECT_FLOAT_EQ(clamp01(0.25f), 0.25f);
}

// T.61: a node type that depends on T only is shared by lists with different allocators; a nested one isn't.
TEST(CoreGuidelinesT, T61_ScaryMemberIsShared) {
    static_assert(std::is_same_v<ScaryList<int>::node_type, ScaryList<int, Pool<int>>::node_type>);
    static_assert(!std::is_same_v<NestedList<int>::Node, NestedList<int, Pool<int>>::Node>);
    Node<int> second{2, nullptr}, first{1, &second};
    ScaryList<int, Pool<int>> l;
    l.head = &first;
    EXPECT_EQ(l.head->next->value, 2);
}

// T.62: an enum in a non-template base is one type for every Parser<T>; a nested one differs per T.
TEST(CoreGuidelinesT, T62_NonDependentMembersInBase) {
    static_assert(std::is_same_v<Parser<int>::Status, Parser<std::string>::Status>);
    static_assert(std::is_same_v<Parser<int>::Status, ParserBase::Status>);
    static_assert(!std::is_same_v<NestedParser<int>::Status, NestedParser<std::string>::Status>);
    EXPECT_NE(ParserBase::Status::ok, Parser<char>::Status::error);
}

// T.64: the specialization for std::string round-trips the text; the primary template handles int.
TEST(CoreGuidelinesT, T64_SpecializationAlternativeImplementation) {
    EXPECT_EQ(Codec<int>::decode(Codec<int>::encode(42)), 42);
    const auto bytes = Codec<std::string>::encode("hello");
    EXPECT_EQ(bytes.size(), 5u);
    EXPECT_EQ(Codec<std::string>::decode(bytes), "hello");
}

// T.65: tag dispatch jumps a vector iterator in one step and walks a list iterator.
TEST(CoreGuidelinesT, T65_TagDispatchSelectsImplementation) {
    std::vector<int> v{0, 1, 2, 3};
    std::list<int> l{0, 1, 2, 3};
    auto vi = v.begin();
    auto li = l.begin();
    EXPECT_EQ(skip(vi, 3), 1);
    EXPECT_EQ(skip(li, 3), 3);
    EXPECT_EQ(*vi, 3);
    EXPECT_EQ(*li, 3);
}

// T.67: == on const char* compares addresses; the specialization compares the text.
TEST(CoreGuidelinesT, T67_SpecializeForIrregularTypes) {
    const char x[] = "abc";
    const char y[] = "abc";
    EXPECT_FALSE(NaiveSame<const char*>{}(x, y));
    EXPECT_TRUE(Same<const char*>{}(x, y));
    EXPECT_TRUE(Same<int>{}(1, 1));
}

// T.68: T{} value-initializes; T{u} rejects narrowing and pointer-to-integer, while T(u) accepts them.
TEST(CoreGuidelinesT, T68_BracesAvoidAmbiguityAndCasts) {
    EXPECT_EQ(make_default<int>(), 0);
    EXPECT_EQ(make_default<std::string>(), "");
    static_assert(!brace_converts<int, double> && paren_converts<int, double>);
    static_assert(!brace_converts<long long, const char*> && paren_converts<long long, const char*>);
    static_assert(brace_converts<long, int>);
}

// T.69: a qualified helper call ignores a same-named function in the argument's namespace; using std::swap; swap() is a deliberate customization point.
TEST(CoreGuidelinesT, T69_QualifyHelpersKeepCustomizationPoints) {
    EXPECT_EQ(lib::process(user::Angle{370}), 1);
    EXPECT_EQ(normalize(user::Angle{0}), 2);   // the user's function exists and ADL would find it
    user::Counted a{1}, b{2};
    lib::reorder(a, b);
    EXPECT_EQ(user::swaps, 1);
    EXPECT_EQ(a.v, 2);
    int i = 1, j = 2;
    lib::reorder(i, j);
    EXPECT_EQ(i, 2);
}

// T.80: without virtuals, Container<Point> instantiates although Point has no <; sort is checked only when used.
TEST(CoreGuidelinesT, T80_NoVirtualsInTemplatedClass) {
    Container<Point> c{{{1, 2}}};
    EXPECT_EQ(c.size(), 1u);
    static_assert(!std::totally_ordered<Point> && !std::is_polymorphic_v<Container<Point>>);
    Container<int> ci{{3, 1, 2}};
    sort(ci);
    EXPECT_EQ(ci.items, (std::vector<int>{1, 2, 3}));
}

// T.81: Derived* converts to Base* (the hole), but span<Derived> doesn't convert to span<Base>.
TEST(CoreGuidelinesT, T81_SpanBlocksArrayToBaseDecay) {
    static_assert(std::is_convertible_v<Derived*, Base*>);
    static_assert(!std::is_convertible_v<std::span<Derived>, std::span<Base>>);
    static_assert(sizeof(Derived) != sizeof(Base));
    Derived items[3];
    for (int i = 0; i < 3; ++i) { items[i].id = i + 1; items[i].extra = 0.5; }
    reset(std::span{items});
    for (const Derived& d : items) {
        EXPECT_EQ(d.id, 0);
        EXPECT_DOUBLE_EQ(d.extra, 0.5);
    }
}

// T.82: a linearized hierarchy has no vtable and every layer runs in order.
TEST(CoreGuidelinesT, T82_LinearHierarchyWithoutVirtuals) {
    static_assert(!std::is_polymorphic_v<Pipeline>);
    static_assert(std::is_base_of_v<Counting<Core>, Pipeline> && std::is_base_of_v<Core, Pipeline>);
    Pipeline p;
    for (int e : {1, -1, 2, -2, 3}) p.handle(e);
    EXPECT_EQ(p.seen, 3);
    EXPECT_EQ(p.handled, 3);
}

// T.83: a non-virtual member template forwards every type to one virtual function.
TEST(CoreGuidelinesT, T83_TemplateFrontVirtualCore) {
    Collect c;
    Sink& s = c;
    s.write(42);
    s.write(std::string{"hi"});
    s.write('x');
    EXPECT_EQ(c.lines, (std::vector<std::string>{"42", "hi", "x"}));
}

// T.84: every List<T> shares the non-template core: one size() for all of them.
TEST(CoreGuidelinesT, T84_NonTemplateCoreShared) {
    static_assert(std::is_base_of_v<ListBase, StableList<int>> && std::is_base_of_v<ListBase, StableList<std::string>>);
    StableList<int> li;
    StableList<std::string> ls;
    li.push_front(1);
    li.push_front(2);
    ls.push_front("a");
    EXPECT_EQ(li.front(), 2);
    EXPECT_EQ(ls.front(), "a");
    std::size_t (ListBase::*sz)() const = &ListBase::size;
    EXPECT_EQ((li.*sz)(), 2u);
    EXPECT_EQ((ls.*sz)(), 1u);
}

// T.100: a variadic template handles arguments of mixed types, each by its own type.
TEST(CoreGuidelinesT, T100_VariadicKeepsTypes) {
    EXPECT_EQ(join("x = ", 1, ", y = ", 2.5, ", s = ", std::string{"z"}), "x = 1, y = 2.5, s = z");
    EXPECT_EQ(join(), "");
}

// T.101: forwarding references move a move-only argument and keep lvalues as lvalues.
TEST(CoreGuidelinesT, T101_PerfectForwarding) {
    Holder h = build<Holder>(std::make_unique<int>(7));
    ASSERT_NE(h.p, nullptr);
    EXPECT_EQ(*h.p, 7);
    int i = 0;
    EXPECT_TRUE(build<Probe>(i).from_lvalue);
    EXPECT_FALSE(build<Probe>(1).from_lvalue);
}

// T.102: a fold expression handles every element, and the empty pack, without recursion.
TEST(CoreGuidelinesT, T102_FoldExpressions) {
    EXPECT_EQ(total_size(1, 2.0), sizeof(int) + sizeof(double));
    EXPECT_EQ(total_size(), 0u);
    static_assert(all_integral<int, long, char> && !all_integral<int, double>);
    static_assert(all_integral<>);
}

// T.103: an initializer_list<int> takes a list of ints and rejects a narrowing double.
TEST(CoreGuidelinesT, T103_InitializerListForHomogeneousLists) {
    EXPECT_EQ(sum({1, 2, 3}), 6);
    EXPECT_EQ(sum({}), 0);
    static_assert(sum_accepts<int> && sum_accepts<short>);
    static_assert(!sum_accepts<double>);
}

// T.120: a fold-expression concept gives the same answers as the recursive metaprogram.
TEST(CoreGuidelinesT, T120_SimplerThanMetaprogram) {
    static_assert(one_of<int, char, int> == tmp_one_of<int, char, int>::value);
    static_assert(one_of<long, char, int> == tmp_one_of<long, char, int>::value);
    static_assert(one_of<int, char, int> && !one_of<long, char, int> && !one_of<int>);
    SUCCEED();
}

// T.121: concept overloads pick the refined random-access version for a vector and the forward one for a forward_list.
TEST(CoreGuidelinesT, T121_ConceptsReplaceEnableIf) {
    EXPECT_EQ(step(std::vector<int>{}.begin()), 2);
    std::forward_list<int> fl;
    EXPECT_EQ(step(fl.begin()), 1);
    std::list<int> l;
    EXPECT_EQ(step(l.begin()), 1);
}

// T.122: an alias template computes "by value for small trivially copyable types, else by const&".
TEST(CoreGuidelinesT, T122_AliasComputesType) {
    static_assert(std::is_same_v<In<int>, int>);
    static_assert(std::is_same_v<In<double>, double>);
    static_assert(std::is_same_v<In<std::string>, const std::string&>);
    static_assert(std::is_same_v<In<std::vector<int>>, const std::vector<int>&>);
    SUCCEED();
}

// T.123: a constexpr function computes the same value as the recursive template, at compile time or run time.
TEST(CoreGuidelinesT, T123_ConstexprFunctionComputesValues) {
    static_assert(power(2, 10) == 1024);
    static_assert(power(3, 4) == Power<3, 4>::value);
    int e = 5;
    EXPECT_EQ(power(2, e), 32);
}

// T.124: std::remove_cvref_t handles every reference kind; the homemade trait misses T&&.
TEST(CoreGuidelinesT, T124_StandardTraitsCoverAllCases) {
    static_assert(std::is_same_v<std::remove_cvref_t<int&&>, int>);
    static_assert(std::is_same_v<std::remove_cvref_t<const int&>, int>);
    static_assert(std::is_same_v<strip_ref<int&>::type, int>);
    static_assert(!std::is_same_v<strip_ref<int&&>::type, int>);
    SUCCEED();
}

// T.140: one named operation serves several algorithms consistently.
TEST(CoreGuidelinesT, T140_NamedOperationReused) {
    std::vector<std::string> names{"ccc", "a", "bb"};
    std::ranges::sort(names, ShorterFirst{});
    EXPECT_EQ(names, (std::vector<std::string>{"a", "bb", "ccc"}));
    auto it = std::ranges::lower_bound(names, std::string{"xx"}, ShorterFirst{});
    EXPECT_EQ(*it, "bb");
}

// T.141: an unnamed lambda at the call site does the one-off job.
TEST(CoreGuidelinesT, T141_UnnamedLambdaInPlace) {
    std::vector<int> v{-1, 2, -3, 4};
    std::erase_if(v, [](int x) { return x < 0; });
    EXPECT_EQ(v, (std::vector<int>{2, 4}));
}

// T.142?: a variable template gives a value of the requested type.
TEST(CoreGuidelinesT, T142_VariableTemplateNamesValuePerType) {
    static_assert(std::is_same_v<decltype(tau<float>), const float>);
    static_assert(std::is_same_v<decltype(tau<double>), const double>);
    EXPECT_NEAR(tau<double>, 6.283185307179586, 1e-15);
}

// T.143: comparing iterators with != works for list and forward_list; < needs random access.
TEST(CoreGuidelinesT, T143_GenericIteratorComparison) {
    std::list<int> l{1, 2, 3};
    std::forward_list<int> fl{1, 2};
    EXPECT_EQ(count_all(l.begin(), l.end()), 3);
    EXPECT_EQ(count_all(fl.begin(), fl.end()), 2);
    static_assert(!less_comparable_iter<std::list<int>::iterator>);
    static_assert(less_comparable_iter<std::vector<int>::iterator>);
}

// T.144: an explicit specialization is ignored when another primary template is a better match; an overload isn't.
TEST(CoreGuidelinesT, T144_OverloadDontSpecialize) {
    int* p = nullptr;
    EXPECT_EQ(spec_which(p), 2);   // the specialization for int* is never chosen
    EXPECT_EQ(over_which(p), 1);
    EXPECT_EQ(over_which(5), 0);
}

// T.150: static_assert confirms a class models the concepts it was meant to; the forgetful version fails them.
TEST(CoreGuidelinesT, T150_StaticAssertClassMatchesConcept) {
    static_assert(std::regular<Temperature> && std::totally_ordered<Temperature>);
    static_assert(!std::regular<PlainTemperature> && !std::equality_comparable<PlainTemperature>);
    EXPECT_TRUE(Temperature{20} < Temperature{21});
}
