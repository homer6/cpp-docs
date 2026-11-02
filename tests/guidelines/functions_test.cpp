// Checks claims made by docs/guidelines/core-guidelines/examples/functions.md.
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

// Counts copies and moves so a test can check that a call didn't copy.
struct Counts {
    int copies = 0;
    int moves = 0;
};
Counts counts;

struct Counted {
    int value = 0;
    Counted() = default;
    explicit Counted(int v) : value(v) {}
    Counted(const Counted& o) : value(o.value) { ++counts.copies; }
    Counted(Counted&& o) noexcept : value(o.value) { ++counts.moves; }
    Counted& operator=(const Counted& o) {
        value = o.value;
        ++counts.copies;
        return *this;
    }
    Counted& operator=(Counted&& o) noexcept {
        value = o.value;
        ++counts.moves;
        return *this;
    }
    ~Counted() = default;
};

void reset_counts() { counts = {}; }

// F.4
constexpr int cube(int x) { return x * x * x; }

// F.6: same as Counted, but its move constructor isn't noexcept.
struct LooseMove {
    int value = 0;
    LooseMove() = default;
    LooseMove(const LooseMove& o) : value(o.value) { ++counts.copies; }
    LooseMove(LooseMove&& o) : value(o.value) { ++counts.moves; }   // can't throw, doesn't say so
    LooseMove& operator=(const LooseMove&) = default;
    LooseMove& operator=(LooseMove&&) = default;
    ~LooseMove() = default;
};

// F.7
struct Gauge {
    int value = 0;
};
int level(const Gauge& g) { return g.value; }
int level_shared(const std::shared_ptr<Gauge>& g) { return g->value; }

// F.8
constexpr double with_tax(double price, double rate) { return price * (1 + rate); }

// F.16
int by_ref(const Counted& c) { return c.value; }
int by_value(Counted c) { return c.value; }

// F.17
void normalize_copy(std::string path) {
    if (!path.empty() && path.back() == '/') path.pop_back();
    EXPECT_EQ(path.back(), 'p');
}
void normalize(std::string& path) {
    if (!path.empty() && path.back() == '/') path.pop_back();
}

// F.18
struct Inbox {
    std::vector<Counted> items;
    void take(std::vector<Counted>&& v) { items = std::move(v); }
};
struct ConstInbox {
    Counted item;
    void take(const Counted& c) { item = std::move(c); }   // the BAD version: std::move of a const& copies
};
template <class Box, class Arg>
concept can_take = requires(Box b, Arg&& a) { b.take(std::forward<Arg>(a)); };

// F.19
template <class T>
void push_forwarded(std::vector<Counted>& v, T&& c) {
    v.push_back(std::forward<T>(c));
}
template <class T>
void push_unforwarded(std::vector<Counted>& v, T&& c) {
    v.push_back(c);
}

// F.20
Counted make_counted(int v) {
    Counted out{v};
    out.value += 1;
    return out;
}

// F.21
struct PortResult {
    int port = 0;
    std::string error;
};
PortResult parse_port(std::string_view s) {
    int port = 0;
    for (char ch : s) {
        if (ch < '0' || ch > '9') return {0, "not a number"};
        port = port * 10 + (ch - '0');
    }
    return {port, ""};
}

// F.60
struct Style {
    int size = 12;
};
int size_or_default(const Style* s) { return s ? s->size : 12; }
int size_of(const Style& s) { return s.size; }

// F.23
template <class P>
class not_null {
    P p_;

public:
    not_null(P p) : p_(p) {}
    not_null(std::nullptr_t) = delete;
    P get() const { return p_; }
};

// F.24
int sum(std::span<const int> values) {
    int total = 0;
    for (int v : values) total += v;
    return total;
}

// F.25
std::size_t spaces(std::string_view s) { return static_cast<std::size_t>(std::ranges::count(s, ' ')); }

// F.26 and F.27
int destroyed = 0;
struct Tracked {
    int id = 0;
    explicit Tracked(int i) : id(i) {}
    Tracked(const Tracked&) = delete;
    Tracked& operator=(const Tracked&) = delete;
    ~Tracked() { ++destroyed; }
};
std::unique_ptr<Tracked> make_tracked(int id) { return std::make_unique<Tracked>(id); }

// F.42
struct Node {
    std::string name;
    Node* left = nullptr;
    Node* right = nullptr;
};
const Node* find(const Node* t, const std::string& name) {
    if (!t || t->name == name) return t;
    if (const Node* p = find(t->left, name)) return p;
    return find(t->right, name);
}

// F.44
struct Wheel {
    int psi = 30;
};
struct Car {
    Wheel wheels[4];
    Wheel& wheel(int i) { return wheels[i]; }
    const Wheel& wheel(int i) const { return wheels[i]; }
};

// F.45
template <class F>
auto logged(F f) {
    return f();
}

// F.47
struct Meter {
    int v = 0;
    Meter& operator=(const Meter& o) {
        v = o.v;
        return *this;
    }
};

template <class T, class U>
using assign_result_t = decltype(std::declval<T>() = std::declval<U>());

// F.48
Counted build_named() {
    Counted c{7};
    c.value += 1;
    return c;
}
Counted build_moved() {
    Counted c{7};
    c.value += 1;
    // The BAD form, std::move(c), spelled as the cast it is so -Wpessimizing-move
    // doesn't flag the measurement itself.
    return static_cast<Counted&&>(c);
}

// F.49
const Counted load_const() { return Counted{3}; }
Counted load() { return Counted{3}; }

// F.51
std::string pad(const std::string& s, std::size_t width, char fill = ' ') {
    return s.size() >= width ? s : std::string(width - s.size(), fill) + s;
}

// F.53
std::function<std::string()> make_greeter(std::string name) {
    return [name = std::move(name)] { return "hi " + name; };
}

// F.54
struct Counter {
    int count = 0;
    auto live_reader() {
        return [this] { return count; };
    }
    auto snapshot_reader() {
        return [*this] { return count; };
    }
};

// F.55
template <class... Ts>
constexpr auto add_all(Ts... xs) {
    return (xs + ... + 0);
}

}  // namespace

// F.4: a constexpr function runs at compile time in a constant expression and at run time otherwise.
TEST(CoreGuidelinesF, F4_ConstexprRunsAtCompileAndRunTime) {
    static_assert(cube(3) == 27);
    int bound[cube(2)] = {};
    EXPECT_EQ(std::size(bound), 8u);
    volatile int n = 4;
    EXPECT_EQ(cube(n), 64);
}

// F.6: vector moves its elements when it grows only if the move constructor is noexcept; otherwise it copies.
TEST(CoreGuidelinesF, F6_NoexceptMoveLetsVectorMove) {
    static_assert(std::is_nothrow_move_constructible_v<Counted>);
    static_assert(!std::is_nothrow_move_constructible_v<LooseMove>);

    std::vector<Counted> good(4);
    reset_counts();
    good.reserve(good.capacity() + 1);
    EXPECT_EQ(counts.copies, 0);
    EXPECT_EQ(counts.moves, 4);

    std::vector<LooseMove> bad(4);
    reset_counts();
    bad.reserve(bad.capacity() + 1);
    EXPECT_EQ(counts.copies, 4);
    EXPECT_EQ(counts.moves, 0);
}

// F.7: a T& parameter accepts any T (stack, unique_ptr, shared_ptr); a shared_ptr parameter rejects a plain T.
TEST(CoreGuidelinesF, F7_ReferenceAcceptsAnyOwnership) {
    Gauge local{1};
    auto shared = std::make_shared<Gauge>(Gauge{2});
    auto unique = std::make_unique<Gauge>(Gauge{3});
    EXPECT_EQ(level(local) + level(*shared) + level(*unique), 6);
    EXPECT_EQ(level_shared(shared), 2);
    static_assert(std::is_invocable_v<decltype(&level), Gauge&>);
    static_assert(!std::is_invocable_v<decltype(&level_shared), Gauge&>);
}

// F.8: a pure function depends only on its arguments, so it can run at compile time and gives the same result every call.
TEST(CoreGuidelinesF, F8_PureFunctionIsRepeatable) {
    static_assert(with_tax(100.0, 0.25) == 125.0);
    EXPECT_DOUBLE_EQ(with_tax(80.0, 0.25), with_tax(80.0, 0.25));
    EXPECT_DOUBLE_EQ(with_tax(80.0, 0.25), 100.0);
}

// F.16: passing by const& doesn't copy; passing by value copies an lvalue argument once.
TEST(CoreGuidelinesF, F16_ConstRefAvoidsCopies) {
    const Counted c{5};
    reset_counts();
    EXPECT_EQ(by_ref(c), 5);
    EXPECT_EQ(counts.copies, 0);
    EXPECT_EQ(by_value(c), 5);
    EXPECT_EQ(counts.copies, 1);
    EXPECT_EQ(by_ref(Counted{6}), 6);   // const& still accepts a temporary
}

// F.17: a T& parameter changes the caller's object; a by-value parameter changes only a copy.
TEST(CoreGuidelinesF, F17_InOutByReference) {
    std::string path = "/tmp/";
    normalize_copy(path);
    EXPECT_EQ(path, "/tmp/");
    normalize(path);
    EXPECT_EQ(path, "/tmp");
}

// F.18: an X&& parameter won't bind an lvalue without std::move, and moves without copying; std::move of a const& copies.
TEST(CoreGuidelinesF, F18_RvalueRefParameterMoves) {
    static_assert(!can_take<Inbox, std::vector<Counted>&>);
    static_assert(can_take<Inbox, std::vector<Counted>&&>);

    std::vector<Counted> batch(3);
    Inbox box;
    reset_counts();
    box.take(std::move(batch));
    EXPECT_EQ(box.items.size(), 3u);
    EXPECT_EQ(counts.copies, 0);

    Counted c{1};
    ConstInbox bad;
    reset_counts();
    bad.take(std::move(c));
    EXPECT_EQ(counts.copies, 1);
    EXPECT_EQ(counts.moves, 0);
}

// F.19: std::forward moves an rvalue argument and copies an lvalue; without it, rvalues are copied too.
TEST(CoreGuidelinesF, F19_ForwardPreservesRvalues) {
    std::vector<Counted> v;
    v.reserve(8);
    Counted lv{1};

    reset_counts();
    push_forwarded(v, Counted{2});
    EXPECT_EQ(counts.copies, 0);
    EXPECT_EQ(counts.moves, 1);
    push_forwarded(v, lv);
    EXPECT_EQ(counts.copies, 1);

    reset_counts();
    push_unforwarded(v, Counted{3});
    EXPECT_EQ(counts.copies, 1);
    EXPECT_EQ(counts.moves, 0);
}

// F.20: returning a local by value never copies it: it's elided or moved.
TEST(CoreGuidelinesF, F20_ReturnValueDoesNotCopy) {
    reset_counts();
    Counted c = make_counted(4);
    EXPECT_EQ(c.value, 5);
    EXPECT_EQ(counts.copies, 0);
    EXPECT_LE(counts.moves, 1);
}

// F.21: a returned struct carries several named results, unpacked with structured bindings.
TEST(CoreGuidelinesF, F21_StructReturnsSeveralValues) {
    auto [port, error] = parse_port("8080");
    EXPECT_EQ(port, 8080);
    EXPECT_TRUE(error.empty());
    const PortResult bad = parse_port("80x");
    EXPECT_EQ(bad.port, 0);
    EXPECT_EQ(bad.error, "not a number");
}

// F.60: a pointer parameter accepts nullptr for "no argument"; a reference parameter can't be given nullptr.
TEST(CoreGuidelinesF, F60_PointerAllowsNoArgument) {
    const Style big{20};
    EXPECT_EQ(size_or_default(nullptr), 12);
    EXPECT_EQ(size_or_default(&big), 20);
    EXPECT_EQ(size_of(big), 20);
    static_assert(std::is_invocable_v<decltype(&size_or_default), std::nullptr_t>);
    static_assert(!std::is_invocable_v<decltype(&size_of), std::nullptr_t>);
}

// F.23: a not_null that deletes its nullptr constructor rejects a literal nullptr at compile time, at no size cost.
TEST(CoreGuidelinesF, F23_NotNullRejectsNullptr) {
    static_assert(!std::is_constructible_v<not_null<int*>, std::nullptr_t>);
    static_assert(std::is_constructible_v<not_null<int*>, int*>);
    static_assert(sizeof(not_null<int*>) == sizeof(int*));
    int x = 3;
    const not_null<int*> p{&x};
    EXPECT_EQ(*p.get(), 3);
}

// F.24: a span carries its size: built from an array or a vector, it covers exactly their elements.
TEST(CoreGuidelinesF, F24_SpanCarriesItsSize) {
    int a[3] = {1, 2, 3};
    std::vector<int> v{4, 5};
    EXPECT_EQ(sum(a), 6);
    EXPECT_EQ(sum(v), 9);
    EXPECT_EQ(std::span<const int>(a).size(), 3u);
    EXPECT_EQ(sum(std::span<const int>(a).subspan(1)), 5);
    static_assert(sizeof(std::span<const int>) == sizeof(const int*) + sizeof(std::size_t));
}

// F.25: a string_view knows its length, so it needs no zero terminator; a C string's length comes from finding one.
TEST(CoreGuidelinesF, F25_StringViewNeedsNoTerminator) {
    const char buffer[5] = {'a', ' ', 'b', ' ', 'c'};   // no terminator
    EXPECT_EQ(spaces(std::string_view(buffer, sizeof buffer)), 2u);
    const char* zstr = "x y";
    EXPECT_EQ(std::strlen(zstr), 3u);
    EXPECT_EQ(spaces(zstr), 1u);
}

// F.26: a unique_ptr can't be copied; moving it transfers ownership, and the object is destroyed exactly once.
TEST(CoreGuidelinesF, F26_UniquePtrTransfersOwnership) {
    static_assert(!std::is_copy_constructible_v<std::unique_ptr<Tracked>>);
    destroyed = 0;
    {
        auto p = make_tracked(1);
        auto q = std::move(p);
        EXPECT_EQ(p, nullptr);
        EXPECT_EQ(q->id, 1);
    }
    EXPECT_EQ(destroyed, 1);
}

// F.27: with shared_ptr the object lives until its last owner lets go, then is destroyed once.
TEST(CoreGuidelinesF, F27_LastSharedOwnerDeletes) {
    destroyed = 0;
    auto a = std::make_shared<Tracked>(2);
    auto b = a;
    EXPECT_EQ(a.use_count(), 2);
    a.reset();
    EXPECT_EQ(destroyed, 0);
    EXPECT_EQ(b->id, 2);
    b.reset();
    EXPECT_EQ(destroyed, 1);
}

// F.42: a returned T* points at an existing object in the caller's structure, or is nullptr for "not found".
TEST(CoreGuidelinesF, F42_PointerReturnsAPosition) {
    Node leaf{"b"};
    Node root{"a", &leaf, nullptr};
    EXPECT_EQ(find(&root, "b"), &leaf);
    EXPECT_EQ(find(&root, "z"), nullptr);
}

// F.44: returning T& gives the caller the object itself, so changes through it stick.
TEST(CoreGuidelinesF, F44_ReferenceReturnAliasesTheObject) {
    static_assert(std::is_same_v<decltype(std::declval<Car&>().wheel(0)), Wheel&>);
    Car car;
    car.wheel(0).psi = 35;
    EXPECT_EQ(car.wheels[0].psi, 35);
    EXPECT_EQ(&car.wheel(1), &car.wheels[1]);
}

// F.45: an auto return type gives a value, not a reference, so the result outlives the wrapped call.
TEST(CoreGuidelinesF, F45_AutoReturnIsAValue) {
    auto make = [] { return std::string("ok"); };
    static_assert(std::is_same_v<decltype(logged(make)), std::string>);
    const std::string r = logged(make);
    EXPECT_EQ(r, "ok");
}

// F.47: an assignment operator returning T& chains like the built-in one.
TEST(CoreGuidelinesF, F47_AssignmentReturnsReference) {
    static_assert(std::is_same_v<assign_result_t<Meter&, const Meter&>, Meter&>);
    static_assert(std::is_same_v<assign_result_t<int&, int>, int&>);
    Meter a, b;
    const Meter seven{7};
    a = b = seven;
    EXPECT_EQ(a.v, 7);
    EXPECT_EQ(b.v, 7);
}

// F.48: `return local` moves at most once (often zero); `return std::move(local)` always moves.
TEST(CoreGuidelinesF, F48_ReturnStdMoveForcesAMove) {
    reset_counts();
    const Counted good = build_named();
    const int good_moves = counts.moves;
    EXPECT_EQ(counts.copies, 0);
    EXPECT_LE(good_moves, 1);

    reset_counts();
    const Counted bad = build_moved();
    EXPECT_EQ(counts.copies, 0);
    EXPECT_EQ(counts.moves, 1);
    EXPECT_EQ(good.value, bad.value);
}

// F.49: assigning from a function that returns const T copies; from one that returns T, it moves.
TEST(CoreGuidelinesF, F49_ConstReturnBlocksMove) {
    Counted x;
    reset_counts();
    x = load_const();
    EXPECT_EQ(counts.copies, 1);
    EXPECT_EQ(counts.moves, 0);

    reset_counts();
    x = load();
    EXPECT_EQ(counts.copies, 0);
    EXPECT_EQ(counts.moves, 1);
}

// F.50: a lambda can be defined locally and capture local variables, which a function can't.
TEST(CoreGuidelinesF, F50_LambdaCapturesLocals) {
    const std::vector<int> v{1, 5, 9, 12};
    int limit = 6;   // not a constant, so the lambda really has to capture it
    auto above = [limit](int x) { return x > limit; };
    EXPECT_EQ(std::ranges::count_if(v, above), 2);
}

// F.51: a default argument gives the short call and the full call one implementation.
TEST(CoreGuidelinesF, F51_DefaultArgumentOneBody) {
    EXPECT_EQ(pad("7", 3), "  7");
    EXPECT_EQ(pad("7", 3, ' '), pad("7", 3));
    EXPECT_EQ(pad("7", 3, '0'), "007");
    EXPECT_EQ(pad("1234", 3), "1234");
}

// F.52: a lambda that captures by reference copies nothing and updates the caller's variables; by value it copies.
TEST(CoreGuidelinesF, F52_LocalLambdaCapturesByReference) {
    const std::vector<int> items{1, 2, 3};
    const Counted weight{10};
    int total = 0;

    reset_counts();
    std::ranges::for_each(items, [&](int i) { total += i * weight.value; });
    EXPECT_EQ(total, 60);
    EXPECT_EQ(counts.copies, 0);

    int copy_total = 0;
    reset_counts();
    std::ranges::for_each(items, [weight, copy_total](int i) mutable { copy_total += i * weight.value; });
    EXPECT_GE(counts.copies, 1);
    EXPECT_EQ(copy_total, 0);   // the lambda added into its own copy
}

// F.53: a lambda that captures by value stays valid after the scope that made it has ended.
TEST(CoreGuidelinesF, F53_ValueCaptureOutlivesScope) {
    std::function<std::string()> greet;
    {
        std::string name = "ada";
        greet = make_greeter(name);
        name = "changed";
    }
    EXPECT_EQ(greet(), "hi ada");
}

// F.54: [this] reads members live through the pointer; [*this] captures a copy of the object.
TEST(CoreGuidelinesF, F54_ThisCaptureIsByReference) {
    Counter c;
    auto live = c.live_reader();
    auto snapshot = c.snapshot_reader();
    c.count = 42;
    EXPECT_EQ(live(), 42);
    EXPECT_EQ(snapshot(), 0);
}

// F.55: a variadic template sees each argument's real type, so mixing int and double gives a double sum.
TEST(CoreGuidelinesF, F55_VariadicTemplateIsTyped) {
    static_assert(add_all(1, 2, 3) == 6);
    static_assert(std::is_same_v<decltype(add_all(1, 2.5)), double>);
    EXPECT_DOUBLE_EQ(add_all(1, 2.5), 3.5);
}
