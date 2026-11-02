// Checks claims made by docs/guidelines/core-guidelines/examples/interfaces.md.
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

// I.1
bool round_up = false;   // the hidden input the page warns about
int to_int_hidden(double d) { return round_up ? int(std::ceil(d)) : int(d); }
enum class Rounding { down, up };
int to_int(double d, Rounding r) { return r == Rounding::up ? int(std::ceil(d)) : int(std::floor(d)); }

// I.3
int table_builds = 0;
struct Table { Table() { ++table_builds; } };
const Table& lookup_table() {
    static const Table t;
    return t;
}

// I.4
std::chrono::milliseconds stored_timeout{};
void set_timeout(std::chrono::milliseconds t) { stored_timeout = t; }

// I.7
std::mutex m;
int counter = 0;
void increment(bool fail) {
    std::scoped_lock lock{m};
    ++counter;
    if (fail) throw std::runtime_error("failed");
}
bool locked_elsewhere(std::mutex& mx) {
    bool got = false;
    std::thread t{[&] { got = mx.try_lock(); if (got) mx.unlock(); }};
    t.join();
    return got;
}

// I.9
template <class T>
concept Addable = std::regular<T> && requires(T a, T b) { { a + b } -> std::convertible_to<T>; };
template <Addable T>
T total(const std::vector<T>& v) {
    T sum{};
    for (const T& x : v) sum = sum + x;
    return sum;
}
template <class T>
T total_unconstrained(const std::vector<T>& v) {
    T sum{};
    for (const T& x : v) sum = sum + x;
    return sum;
}
struct Name { };
template <class V>
concept can_total = requires(const V& v) { total(v); };
template <class V>
concept can_total_unconstrained = requires(const V& v) { total_unconstrained(v); };

// I.10
int parse_port(const std::string& s) {
    std::size_t used = 0;
    int port = std::stoi(s, &used);
    if (used != s.size() || port < 1 || port > 65535) throw std::out_of_range("not a port: " + s);
    return port;
}

// I.11
int plugins_alive = 0;
struct Plugin {
    Plugin() { ++plugins_alive; }
    virtual ~Plugin() { --plugins_alive; }
};
std::unique_ptr<Plugin> load_plugin() { return std::make_unique<Plugin>(); }
std::vector<double> load_samples() { return std::vector<double>(1000); }

// I.12
std::size_t length(std::string_view s) { return s.size(); }
struct Engine { int rpm = 0; };
void start(Engine& e) { e.rpm = 800; }

// I.13
struct Shape { virtual ~Shape() = default; int id = 0; };
struct Circle : Shape { double r = 1; };
int draw(std::span<const Circle> circles) {
    int n = 0;
    for (const Circle& c : circles) n += c.id + 1;
    return n;
}

// I.22
constexpr int square(int x) { return x * x; }
constinit int retry_limit = square(3);
int config_builds = 0;
const std::string& config() {
    static const std::string c = [] { ++config_builds; return std::string{"loaded"}; }();
    return c;
}

// I.24
void copy_bytes(std::span<const char> from, std::span<char> to) {
    for (std::size_t i = 0; i < from.size() && i < to.size(); ++i) to[i] = from[i];
}

// I.25
struct Point { int x, y; };
class IShape {
public:
    virtual Point center() const = 0;
    virtual ~IShape() = default;
};
int shapes_alive = 0;
class Disc : public IShape {
    Point c_;
public:
    explicit Disc(Point c) : c_{c} { ++shapes_alive; }
    ~Disc() override { --shapes_alive; }
    Point center() const override { return c_; }
};

// I.26
extern "C" {
struct split_result {
    std::size_t count;
    int error;
};
}

// I.27
class Parser {
public:
    Parser();
    ~Parser();
    Parser(Parser&&) noexcept;
    Parser& operator=(Parser&&) noexcept;
    int parse(const std::string& text);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
struct Parser::Impl {
    std::vector<std::string> seen;
};
Parser::Parser() : impl_{std::make_unique<Impl>()} {}
Parser::~Parser() = default;
Parser::Parser(Parser&&) noexcept = default;
Parser& Parser::operator=(Parser&&) noexcept = default;
int Parser::parse(const std::string& text) {
    impl_->seen.push_back(text);
    return static_cast<int>(impl_->seen.size());
}

// I.30: the page's Input, with a string stream standing in for a file.
class Input {
    std::unique_ptr<std::istream> owned_;
    std::istream* in_;
public:
    explicit Input(std::istream& borrowed) : in_{&borrowed} {}
    explicit Input(const std::string& text)
        : owned_{std::make_unique<std::istringstream>(text)}, in_{owned_.get()} {}
    std::istream& get() { return *in_; }
};

}  // namespace

// I.1: a hidden global mode changes the result of the same call; an explicit parameter doesn't.
TEST(CoreGuidelinesI, I1_ExplicitInterfaces) {
    round_up = false;
    const int first = to_int_hidden(7.2);
    round_up = true;
    const int second = to_int_hidden(7.2);
    round_up = false;
    EXPECT_NE(first, second);
    EXPECT_EQ(to_int(7.2, Rounding::down), 7);
    EXPECT_EQ(to_int(7.2, Rounding::up), 8);
}

// I.3: a function-local static is initialized once, on the first call.
TEST(CoreGuidelinesI, I3_FunctionLocalStaticOnFirstUse) {
    EXPECT_EQ(table_builds, 0);
    const Table& a = lookup_table();
    const Table& b = lookup_table();
    EXPECT_EQ(table_builds, 1);
    EXPECT_EQ(&a, &b);
}

// I.4: a duration parameter converts other units exactly and rejects a bare int or a lossy unit.
TEST(CoreGuidelinesI, I4_StronglyTypedParameters) {
    using namespace std::chrono_literals;
    set_timeout(2s);
    EXPECT_EQ(stored_timeout.count(), 2000);
    set_timeout(1500ms);
    EXPECT_EQ(stored_timeout.count(), 1500);
    static_assert(!std::is_convertible_v<int, std::chrono::milliseconds>);
    static_assert(!std::is_convertible_v<std::chrono::microseconds, std::chrono::milliseconds>);
    static_assert(std::is_convertible_v<std::chrono::seconds, std::chrono::milliseconds>);
}

// I.7: with RAII the postcondition "the mutex is unlocked on exit" holds on every path.
TEST(CoreGuidelinesI, I7_PostconditionEnforcedByRaii) {
    counter = 0;
    increment(false);
    EXPECT_TRUE(locked_elsewhere(m));   // unlocked after a normal return
    EXPECT_THROW(increment(true), std::runtime_error);
    EXPECT_TRUE(locked_elsewhere(m));   // and after an exception
    EXPECT_EQ(counter, 2);
}

// I.9: a concept-constrained template rejects a bad argument at the interface; an unconstrained one claims to accept it.
TEST(CoreGuidelinesI, I9_ConceptsDocumentRequirements) {
    EXPECT_EQ(total(std::vector<int>{1, 2, 3}), 6);
    static_assert(Addable<int> && !Addable<Name>);
    static_assert(can_total<std::vector<int>> && !can_total<std::vector<Name>>);
    // The unconstrained signature looks callable; the error would only appear in its body.
    static_assert(can_total_unconstrained<std::vector<Name>>);
}

// I.10: a failure to do the required task throws, so it can't be ignored.
TEST(CoreGuidelinesI, I10_ExceptionsSignalFailure) {
    EXPECT_EQ(parse_port("8080"), 8080);
    EXPECT_THROW(parse_port("http"), std::invalid_argument);
    EXPECT_THROW(parse_port("70000"), std::out_of_range);
    EXPECT_THROW(parse_port("80x"), std::out_of_range);
}

// I.11: ownership returned as a value or a unique_ptr is released without the caller deleting anything.
TEST(CoreGuidelinesI, I11_NoOwnershipByRawPointer) {
    static_assert(std::is_same_v<decltype(load_plugin()), std::unique_ptr<Plugin>>);
    load_plugin();   // the returned owner is dropped at once
    EXPECT_EQ(plugins_alive, 0);
    {
        auto p = load_plugin();
        EXPECT_EQ(plugins_alive, 1);
    }
    EXPECT_EQ(plugins_alive, 0);
    EXPECT_EQ(load_samples().size(), 1000u);
}

// I.12: a reference or string_view parameter can't be given nullptr (string_view(nullptr_t) is deleted since C++23).
TEST(CoreGuidelinesI, I12_ParametersThatCantBeNull) {
    static_assert(!std::is_constructible_v<std::string_view, std::nullptr_t>);
    static_assert(!std::is_invocable_v<decltype(&length), std::nullptr_t>);
    static_assert(!std::is_invocable_v<decltype(&start), std::nullptr_t>);
    Engine e;
    start(e);
    EXPECT_EQ(e.rpm, 800);
    EXPECT_EQ(length("abc"), 3u);
}

// I.13: a derived array decays to a base pointer, but a span of the base can't be made from it; a span keeps the count.
TEST(CoreGuidelinesI, I13_SpanNotSinglePointer) {
    static_assert(std::is_convertible_v<Circle (&)[3], Shape*>);   // the dangerous decay
    static_assert(!std::is_convertible_v<Circle (&)[3], std::span<const Shape>>);
    static_assert(std::is_convertible_v<Circle (&)[3], std::span<const Circle>>);
    Circle circles[3];
    EXPECT_EQ(draw(circles), 3);
}

// I.22: a constinit global is constant-initialized; a function-local static is built on first use.
TEST(CoreGuidelinesI, I22_SimpleGlobalInitialization) {
    EXPECT_EQ(retry_limit, 9);
    static_assert(square(3) == 9);
    EXPECT_EQ(config_builds, 0);
    EXPECT_EQ(config(), "loaded");
    EXPECT_EQ(config(), "loaded");
    EXPECT_EQ(config_builds, 1);
}

// I.24: with a const source, swapping the source and destination arguments doesn't compile.
TEST(CoreGuidelinesI, I24_SwappedArgumentsAreRejected) {
    static_assert(std::is_invocable_v<decltype(&copy_bytes), std::span<const char>, std::span<char>>);
    static_assert(!std::is_invocable_v<decltype(&copy_bytes), std::span<char>, std::span<const char>>);
    const char in[3] = {'a', 'b', 'c'};
    char out[3] = {};
    copy_bytes(in, out);
    EXPECT_EQ(std::string_view(out, 3), "abc");
}

// I.25: an interface with only pure virtual functions is abstract with a virtual destructor; deleting through it destroys the derived object.
TEST(CoreGuidelinesI, I25_PureInterfaces) {
    static_assert(std::is_abstract_v<IShape>);
    static_assert(std::has_virtual_destructor_v<IShape>);
    {
        std::unique_ptr<IShape> s = std::make_unique<Disc>(Point{1, 2});
        EXPECT_EQ(s->center().y, 2);
        EXPECT_EQ(shapes_alive, 1);
    }
    EXPECT_EQ(shapes_alive, 0);
}

// I.26: a C-style interface struct is standard-layout and trivially copyable.
TEST(CoreGuidelinesI, I26_CStyleAbiTypes) {
    static_assert(std::is_standard_layout_v<split_result>);
    static_assert(std::is_trivially_copyable_v<split_result>);
    split_result r{2, 0};
    EXPECT_EQ(r.count, 2u);
}

// I.27: a Pimpl class's layout is just the pointer, and it moves but doesn't copy.
TEST(CoreGuidelinesI, I27_Pimpl) {
    // Not guaranteed by the standard, but true of every implementation we test.
    EXPECT_EQ(sizeof(Parser), sizeof(void*));
    static_assert(!std::is_copy_constructible_v<Parser>);
    static_assert(std::is_nothrow_move_constructible_v<Parser>);
    static_assert(std::is_nothrow_move_assignable_v<Parser>);
    Parser p;
    EXPECT_EQ(p.parse("a"), 1);
    Parser q = std::move(p);
    EXPECT_EQ(q.parse("b"), 2);
    p = std::move(q);
    EXPECT_EQ(p.parse("c"), 3);
}

// I.30: one class hides "sometimes owned, sometimes borrowed": it reads both ways and never destroys a borrowed stream.
TEST(CoreGuidelinesI, I30_EncapsulateRuleViolations) {
    std::string word;
    Input owned{std::string{"hello"}};
    owned.get() >> word;
    EXPECT_EQ(word, "hello");

    std::istringstream outside{"world"};
    {
        Input borrowed{outside};
        borrowed.get() >> word;
    }
    EXPECT_EQ(word, "world");
    outside.clear();
    outside.str("again");
    outside >> word;   // still usable: the Input didn't destroy it
    EXPECT_EQ(word, "again");
}
