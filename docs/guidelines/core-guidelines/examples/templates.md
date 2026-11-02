# T: Templates and generic programming — examples

> Our examples for the [T rules](../templates.md): templates that say what
> they need with concepts, stay independent of their surroundings, and leave
> metaprogramming for when nothing simpler works.

## T.1: Use templates to raise the level of abstraction of code

Write the algorithm once, in terms of what it needs from a type (a number you
can add), instead of once per type. The template states the idea; each copy by
hand is a chance for the copies to drift apart.

```cpp bad
#include <vector>
// BAD: the same loop, copied per element type; a fix to one won't reach the other
int total_ints(const std::vector<int>& v) { int s = 0; for (int x : v) s += x; return s; }
double total_doubles(const std::vector<double>& v) { double s = 0; for (double x : v) s += x; return s; }
```

```cpp
#include <ranges>
template <class T>
concept Number = std::regular<T> && requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
    { a - b } -> std::convertible_to<T>;
    { a += b } -> std::same_as<T&>;
};

// GOOD: one definition for every range of numbers
template <std::ranges::input_range R>
    requires Number<std::ranges::range_value_t<R>>
auto total(const R& r) {
    std::ranges::range_value_t<R> s{};
    for (const auto& x : r) s += x;
    return s;
}
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.2: Use templates to express algorithms that apply to many argument types

An algorithm such as "find a value" doesn't care whether the elements live in
an array, a `std::list` or a `std::vector`. Written for one container, it can't
be used with the others.

```cpp compile-fail="cannot convert|no matching function"
#include <list>
// BAD: works only on arrays of int
const int* find_int(const int* first, const int* last, int value) {
    for (; first != last; ++first)
        if (*first == value) return first;
    return last;
}
std::list<int> ids{3, 1, 4};
auto it = find_int(ids.begin(), ids.end(), 4);   // error: a list iterator isn't an int*
```

```cpp
#include <iterator>
#include <list>
// GOOD: any input iterator whose elements compare with the value
template <std::input_iterator I, std::sentinel_for<I> S, class V>
    requires std::equality_comparable_with<std::iter_reference_t<I>, const V&>
I find_value(I first, S last, const V& value) {
    for (; first != last; ++first)
        if (*first == value) return first;
    return first;
}
std::list<int> ids{3, 1, 4};
auto it = find_value(ids.begin(), ids.end(), 4);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.3: Use templates to express containers and ranges

A container's element type belongs in its type. A `void*` buffer with a byte
size hides the element type from the compiler, so every access needs a cast
and nothing stops the wrong one.

```cpp bad
#include <cstdlib>
// BAD: the element type lives only in the programmer's head
struct Buffer {
    void* data;
    std::size_t count;
    std::size_t elem_size;
};
void fill(Buffer& b) {
    static_cast<double*>(b.data)[0] = 1.5;   // nothing checks that b really holds doubles
}
```

```cpp
#include <cstddef>
#include <vector>
// GOOD: the element type is part of the container's type
template <std::semiregular T>
class Buffer {
public:
    explicit Buffer(std::size_t n) : items_(n) {}
    T& operator[](std::size_t i) { return items_[i]; }
    std::size_t size() const { return items_.size(); }
private:
    std::vector<T> items_;
};
Buffer<double> b(4);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.4: Use templates to express syntax tree manipulation

Templates can build a tree of an expression at compile time and evaluate it
later in one pass (expression templates). `a + b + c` on vectors then makes no
temporary vector per `+`.

```cpp bad
#include <vector>
using Vec = std::vector<double>;
// BAD: each + allocates and fills a whole temporary vector
Vec operator+(const Vec& a, const Vec& b) {
    Vec r(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) r[i] = a[i] + b[i];
    return r;
}
Vec sum3(const Vec& a, const Vec& b, const Vec& c) { return a + b + c; }   // two temporaries
```

```cpp
#include <cstddef>
#include <type_traits>
#include <vector>
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
    Vec(const E& e) : v(e.size()) {                       // the one loop
        for (std::size_t i = 0; i < v.size(); ++i) v[i] = e[i];
    }
};

// A node of the tree: holds its operands (a Vec by reference, a subtree by value)
template <class L, class R>
struct Sum {
    std::conditional_t<std::is_same_v<L, Vec>, const Vec&, L> l;
    std::conditional_t<std::is_same_v<R, Vec>, const Vec&, R> r;
    double operator[](std::size_t i) const { return l[i] + r[i]; }
    std::size_t size() const { return l.size(); }
};

// GOOD: + builds a node; nothing is computed until a Vec is constructed
template <VecExpr L, VecExpr R>
Sum<L, R> operator+(const L& l, const R& r) { return {l, r}; }

Vec sum3(const Vec& a, const Vec& b, const Vec& c) { return a + b + c; }
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.5: Combine generic and OO techniques to amplify their strengths, not their costs

A virtual interface gives one run-time type for many implementations; a
template writes those implementations for you. Use the template to generate
the boilerplate behind a stable, non-template interface.

```cpp nocompile
// BAD: a hand-written adapter per type, all identical except for the name
struct Shape { virtual ~Shape() = default; virtual double area() const = 0; };
struct CircleShape : Shape { Circle c; double area() const override { return c.area(); } };
struct SquareShape : Shape { Square s; double area() const override { return s.area(); } };
// ... and one more for every new shape
```

```cpp
#include <memory>
#include <utility>
struct Shape {                                  // dynamic: one interface
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

template <class T>
    requires requires(const T& t) { { t.area() } -> std::convertible_to<double>; }
class ShapeModel final : public Shape {         // static: generated per type
public:
    explicit ShapeModel(T t) : t_(std::move(t)) {}
    double area() const override { return t_.area(); }
private:
    T t_;
};

// GOOD: wrap any type with an area() in the same interface
template <class T>
std::unique_ptr<Shape> make_shape(T t) { return std::make_unique<ShapeModel<T>>(std::move(t)); }
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.10: Specify concepts for all template arguments

A concept is the template's contract: it documents what a type must offer and
turns a misuse into a short error at the call instead of one deep inside the
body.

```cpp compile-fail="no match for|invalid operands"
#include <vector>
template <class T>                      // BAD: any T is accepted ...
T twice(T x) { return x + x; }
auto v = twice(std::vector<int>{1});    // ... and the error is reported inside twice
```

```cpp
#include <concepts>
template <class T>
concept Number = std::regular<T> && requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
    { a * b } -> std::convertible_to<T>;
};

template <Number T>                     // GOOD: the requirement is in the signature
T twice(T x) { return x + x; }
auto n = twice(21);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.11: Whenever possible use standard concepts

The standard library's concepts have been designed and reviewed carefully, and
other code already speaks them. A homemade one tends to miss something, as this
"sortable" check misses random access.

```cpp compile-fail="no match for 'operator-'|invalid operands to binary expression"
#include <algorithm>
#include <list>
template <class C>                       // BAD: a homemade, incomplete idea of "sortable"
concept SortableContainer = requires(C& c) { c.begin(); c.end(); };

void sort_all(SortableContainer auto& c) { std::sort(c.begin(), c.end()); }
void f() {
    std::list<int> l{3, 1, 2};
    sort_all(l);                         // passes the concept, then fails inside std::sort
}
```

```cpp
#include <algorithm>
#include <ranges>
// GOOD: the standard's concepts say exactly what std::ranges::sort needs
template <std::ranges::random_access_range R>
    requires std::sortable<std::ranges::iterator_t<R>>
void sort_all(R& r) { std::ranges::sort(r); }
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.12: Prefer concept names over `auto` for local variables

`auto` says only "some type". A constrained `auto` tells the reader what kind
of value it is, and the compiler checks it if the initializer's type ever
changes.

```cpp bad
#include <vector>
void f(const std::vector<int>& v) {
    auto n = v.size();            // BAD: a reader has to look up what size() returns,
    use(n);                       //      and nothing checks it stays a count
}
```

```cpp body
std::vector<int> v{1, 2, 3};
std::unsigned_integral auto n = v.size();     // GOOD: an unsigned count, checked
std::ranges::random_access_range auto& r = v;  // GOOD: says how r is used
use(n, r);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.13: Prefer the shorthand notation for simple, single-type argument concepts

When a concept constrains one type parameter, put it where the parameter is
introduced (`template<std::integral T>`) or use `std::integral auto`. A
separate `requires` clause for the same thing only adds words.

```cpp bad
#include <concepts>
template <typename T>
    requires std::integral<T>         // BAD: two lines to say "T is integral"
T half(T x) { return x / 2; }
```

```cpp
#include <concepts>
#include <ranges>
template <std::integral T>            // GOOD: "T is integral"
T half(T x) { return x / 2; }

void reverse_all(std::ranges::bidirectional_range auto& r) {   // GOOD: no template header needed
    std::ranges::reverse(r);
}
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.20: Avoid "concepts" without meaningful semantics

A concept that checks a single operator, such as "has `+`", says nothing
about meaning: `std::string` has a `+` that concatenates. Name a real idea
(a number) and require the operations that idea implies.

```cpp bad
#include <string>
template <class T>
concept Addable = requires(T a, T b) { a + b; };   // BAD: syntax only

template <Addable T>
T average(T a, T b) { return (a + b) / 2; }        // meant for numbers ...
auto s = std::string{"4"} + std::string{"2"};      // ... but std::string is "Addable" too: "42"
```

```cpp
#include <string>
template <class T>
concept Number = std::regular<T> && requires(T a, T b) {   // GOOD: what a number offers
    { a + b } -> std::convertible_to<T>;
    { a - b } -> std::convertible_to<T>;
    { a * b } -> std::convertible_to<T>;
    { a / b } -> std::convertible_to<T>;
};

template <Number T>
T average(T a, T b) { return (a + b) / 2; }
static_assert(!Number<std::string>);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.21: Require a complete set of operations for a concept

Operations come in families: a type with `+` is expected to have `+=` and `-`;
a type with `<` is expected to have `>` and `>=`. Supplying only part of a family
surprises the caller at the first operation you left out.

```cpp compile-fail="no match for 'operator\+='|no viable overloaded '\+='"
struct Money {
    long cents;
    friend Money operator+(Money a, Money b) { return {a.cents + b.cents}; }   // BAD: + only
};
void add(Money& total, Money m) { total += m; }   // error: no +=
```

```cpp
#include <compare>
struct Money {
    long cents;
    // GOOD: the arithmetic family ...
    friend Money operator+(Money a, Money b) { return {a.cents + b.cents}; }
    friend Money operator-(Money a, Money b) { return {a.cents - b.cents}; }
    Money& operator+=(Money b) { cents += b.cents; return *this; }
    Money& operator-=(Money b) { cents -= b.cents; return *this; }
    // ... and all six comparisons
    friend auto operator<=>(const Money&, const Money&) = default;
};
static_assert(std::totally_ordered<Money>);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.22: Specify axioms for concepts

The compiler checks a concept's syntax, never its meaning. Write the meaning
down next to the concept, as the laws a type must obey; the act of writing them
often shows the concept is wrong, and tests can check them for sample values.

```cpp bad
template <class T>
concept Monoid = requires(T a, T b) { a + b; T{}; };   // BAD: which + ? which T{} ? no laws stated
```

```cpp
#include <concepts>
// GOOD: the laws are written down. For all a, b, c of T:
//   (a + b) + c == a + (b + c)        (associative)
//   a + T{} == a  and  T{} + a == a   (T{} is the identity)
template <class T>
concept Monoid = std::regular<T> && requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
};

template <Monoid T, class R>
T fold_all(const R& r) {
    T acc{};
    for (const T& x : r) acc = acc + x;   // relies on the laws, e.g. to split the work
    return acc;
}
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.23: Differentiate a refined concept from its more general case by adding new use patterns.

The compiler picks the more refined of two overloads only when one concept is
defined in terms of the other and adds requirements. Two separately written
concepts with the same requirements are unrelated, so the call is ambiguous.

```cpp compile-fail="is ambiguous"
template <class T> concept Shape   = requires(const T& t) { t.area(); };
template <class T> concept Polygon = requires(const T& t) { t.area(); };   // BAD: nothing new
struct Square { double area() const { return 4; } int sides() const { return 4; } };

int describe(const Shape auto&)   { return 0; }
int describe(const Polygon auto&) { return 1; }
int r = describe(Square{});   // error: neither overload is more constrained
```

```cpp
template <class T> concept Shape = requires(const T& t) { t.area(); };
template <class T>
concept Polygon = Shape<T> && requires(const T& t) { t.sides(); };   // GOOD: refines Shape
struct Square { double area() const { return 4; } int sides() const { return 4; } };

int describe(const Shape auto&)   { return 0; }
int describe(const Polygon auto&) { return 1; }
int r = describe(Square{});   // 1: Polygon subsumes Shape
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.24: Use tag classes or traits to differentiate concepts that differ only in semantics.

"Sorted" isn't something the compiler can see in a type's operations: a sorted
range has the same syntax as any range. Let types opt in with a trait (as
`std::ranges::enable_view` does) and put the trait in the concept.

```cpp compile-fail="is ambiguous"
#include <ranges>
#include <vector>
template <class R>
concept SortedRange = std::ranges::forward_range<R>;   // BAD: same requirements as forward_range

bool contains(const std::ranges::forward_range auto&, int) { return false; }   // linear search
bool contains(const SortedRange auto&, int)                { return true; }    // binary search
bool b = contains(std::vector<int>{3, 1, 2}, 2);   // error: ambiguous (and it isn't even sorted)
```

```cpp
#include <algorithm>
#include <ranges>
#include <type_traits>
#include <vector>
template <class R> inline constexpr bool enable_sorted = false;   // the opt-in trait

template <class R>
concept SortedRange = std::ranges::forward_range<R> && enable_sorted<std::remove_cvref_t<R>>;

struct SortedInts { std::vector<int> v; auto begin() const { return v.begin(); } auto end() const { return v.end(); } };
template <> inline constexpr bool enable_sorted<SortedInts> = true;   // GOOD: says it's sorted

bool contains(const std::ranges::forward_range auto& r, int x) { return std::ranges::find(r, x) != std::ranges::end(r); }
bool contains(const SortedRange auto& r, int x) { return std::ranges::binary_search(r, x); }
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.25: Avoid complementary constraints

Overloads constrained with `C<T>` and `!C<T>` look complete, but `!C<T>` takes
no part in subsumption, so the next overload you add can collide with it.
Write an unconstrained general version and constrained special cases instead.

```cpp compile-fail="is ambiguous"
#include <concepts>
template <class T> requires std::integral<T>    int kind(T) { return 1; }
template <class T> requires (!std::integral<T>) int kind(T) { return 0; }   // BAD: complement
template <class T> requires std::floating_point<T> int kind(T) { return 2; }   // added later
int k = kind(2.5);   // error: !integral and floating_point both match, neither wins
```

```cpp
#include <concepts>
template <class T>                              int kind(T) { return 0; }   // GOOD: general case
template <class T> requires std::integral<T>       int kind(T) { return 1; }
template <class T> requires std::floating_point<T> int kind(T) { return 2; }
int k = kind(2.5);   // 2
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.26: Prefer to define concepts in terms of use-patterns rather than simple syntax

Write the requirement as the code a user would write (`{ a == b } ->
std::convertible_to<bool>`). That form follows the language's rules, including
conversions; a test built from type traits checks something narrower than what
the template really uses.

```cpp compile-fail="no matching function|constraints not satisfied"
#include <type_traits>
#include <utility>
template <class T>   // BAD: demands == return exactly bool
concept Equality = std::is_same_v<decltype(std::declval<const T&>() == std::declval<const T&>()), bool>;

struct Truth { bool v; operator bool() const { return v; } };
struct Bits {
    int v;
    friend Truth operator==(const Bits& a, const Bits& b) { return {a.v == b.v}; }
    friend Truth operator!=(const Bits& a, const Bits& b) { return {a.v != b.v}; }
};
bool same(const Equality auto& a, const Equality auto& b) { return a == b; }
bool s = same(Bits{1}, Bits{1});   // error: rejected, though a == b works fine in an if
```

```cpp
#include <concepts>
template <class T>   // GOOD: the expressions a user writes
concept Equality = requires(const T& a, const T& b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};

struct Truth { bool v; operator bool() const { return v; } };
struct Bits {
    int v;
    friend Truth operator==(const Bits& a, const Bits& b) { return {a.v == b.v}; }
    friend Truth operator!=(const Bits& a, const Bits& b) { return {a.v != b.v}; }
};
bool same(const Equality auto& a, const Equality auto& b) { return a == b; }
bool s = same(Bits{1}, Bits{1});
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.30: Use concept negation (`!C<T>`) sparingly to express a minor difference

The official rule has only its title so far. Define a concept by what a type
can do; use `!` only to carve a small exception out of a positive concept. A
concept that is mostly a negation matches far more types than you meant.

```cpp bad
#include <type_traits>
template <class T>
concept Text = !std::is_arithmetic_v<T>;   // BAD: "not a number" isn't "text":
                                           //      std::vector<int> and Widget pass too
```

```cpp
#include <concepts>
#include <string_view>
template <class T>
concept Text = std::convertible_to<const T&, std::string_view>;   // positive

template <class T>   // GOOD: negation for a minor exception: bool isn't a byte-sized number
concept SmallInt = std::integral<T> && sizeof(T) == 1 && !std::same_as<T, bool>;
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.31: Use concept disjunction (`C1<T> || C2<T>`) sparingly to express alternatives

The official rule has only its title so far. A long list of alternatives
(`same_as<T, A> || same_as<T, B> || …`) is a closed list that misses the next
type; look for the common property instead, and keep `||` for a few real
alternatives.

```cpp compile-fail="no matching function"
#include <concepts>
#include <string>
#include <string_view>
template <class T>   // BAD: a closed list of types
concept Stringish = std::same_as<T, std::string> || std::same_as<T, std::string_view>
                 || std::same_as<T, const char*>;
std::size_t length(const Stringish auto& s) { return std::string_view{s}.size(); }
auto n = length("hello");   // error: "hello" is a const char[6], not on the list
```

```cpp
#include <concepts>
#include <string_view>
template <class T>   // GOOD: the property all of them share
concept Stringish = std::convertible_to<const T&, std::string_view>;
std::size_t length(const Stringish auto& s) { return std::string_view{s}.size(); }
auto n = length("hello");

template <class T>   // fine: two genuine alternatives
concept Arithmetic = std::integral<T> || std::floating_point<T>;
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.40: Use function objects to pass operations to algorithms

A function object (a lambda, `std::less<>`) carries state such as a threshold,
and its type names exactly one operation, so the algorithm can inline it. A
function pointer has neither: state goes in a global and the call is indirect.

```cpp bad
#include <algorithm>
#include <vector>
int g_limit = 0;                                 // BAD: state smuggled in a global
bool above_limit(int x) { return x > g_limit; }
long count_above(const std::vector<int>& v, int limit) {
    g_limit = limit;
    return std::count_if(v.begin(), v.end(), above_limit);   // an indirect call per element
}
```

```cpp
#include <algorithm>
#include <functional>
#include <vector>
long count_above(const std::vector<int>& v, int limit) {
    // GOOD: the lambda carries its limit; its type is unique, so the call inlines
    return std::ranges::count_if(v, [limit](int x) { return x > limit; });
}
void sort_down(std::vector<int>& v) { std::ranges::sort(v, std::greater<>{}); }
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.41: Require only essential properties in a template's concepts

The concept should state what the algorithm fundamentally needs. Adding a
requirement for some incidental use (logging, debugging) shuts out types that
would work, and changes the interface each time the incidental code changes.

```cpp compile-fail="no matching function|constraints not satisfied"
#include <algorithm>
#include <ostream>
#include <ranges>
#include <vector>
template <class T>
concept Printable = requires(std::ostream& os, const T& t) { os << t; };

template <std::ranges::forward_range R>   // BAD: printing is only for a debug trace
    requires std::totally_ordered<std::ranges::range_value_t<R>>
          && Printable<std::ranges::range_value_t<R>>
auto largest(const R& r) { return *std::ranges::max_element(r); }

struct Version { int major, minor; auto operator<=>(const Version&) const = default; };
auto v = largest(std::vector<Version>{{1, 2}, {2, 0}});   // error: Version can't be printed
```

```cpp
#include <algorithm>
#include <ranges>
#include <vector>
template <std::ranges::forward_range R>   // GOOD: finding the largest needs an order, nothing else
    requires std::totally_ordered<std::ranges::range_value_t<R>>
auto largest(const R& r) { return *std::ranges::max_element(r); }

struct Version { int major, minor; auto operator<=>(const Version&) const = default; };
auto v = largest(std::vector<Version>{{1, 2}, {2, 0}});
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.42: Use template aliases to simplify notation and hide implementation details

A dependent type spelled out in full (`typename std::iterator_traits<typename
C::iterator>::value_type`) is noisy, easy to get wrong, and exposes how the
type is found. An alias template names it once.

```cpp compile-fail="need 'typename'|missing 'typename'"
#include <iterator>
template <class C>
auto first_or_default(const C& c) {
    // BAD: the long spelling, and the easy mistake: a missing typename
    std::iterator_traits<typename C::const_iterator>::value_type x{};
    if (!c.empty()) x = *c.begin();
    return x;
}
```

```cpp
#include <ranges>
#include <vector>
template <class R>
using Elem = std::ranges::range_value_t<R>;   // GOOD: named once

template <class C>
Elem<C> first_or_default(const C& c) {
    Elem<C> x{};
    if (!c.empty()) x = *c.begin();
    return x;
}

template <class T>
class Grid {
    std::vector<T> cells_;
public:
    using iterator = typename std::vector<T>::iterator;   // hides that cells are in a vector
};
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.43: Prefer `using` over `typedef` for defining aliases

`using` puts the new name first, reads like an assignment, and can be a
template; `typedef` can't.

```cpp compile-fail="template declaration of 'typedef'|typedef cannot be a template"
#include <vector>
template <class T>
typedef std::vector<T> List;   // BAD: a typedef can't be a template
```

```cpp
#include <vector>
template <class T>
using List = std::vector<T>;           // GOOD
using Handler = void (*)(int);         // GOOD: the name first, not buried in the type
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.44: Use function templates to deduce class template argument types (where feasible)

Let the compiler work out a class template's arguments, with a `make_` function
or (since C++17) class template argument deduction. Types written by hand are
noise and can be wrong.

```cpp compile-fail="no matching"
#include <string>
#include <tuple>
std::tuple<int, std::string, double> t{1, 2.5, "one"};   // BAD: types by hand, in the wrong order
```

```cpp
#include <string>
#include <tuple>
#include <utility>
std::pair p{3, 2.5};                                 // GOOD: std::pair<int, double>
auto t = std::make_tuple(1, std::string{"one"});     // GOOD: std::tuple<int, std::string>
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.46: (removed)

*No example: the rule was removed upstream; only its number remains.*

## T.47: Avoid highly visible unconstrained templates with common names

An unconstrained function template is an exact match for any argument, so when
argument-dependent lookup finds it, it beats a function that needs a
conversion, such as derived-to-base. Constrain it, or give it an uncommon
name.

```cpp bad
#include <string>
namespace geo {
    struct Shape {};
    struct Circle : Shape {};
    template <class T>
    std::string describe(const T&) { return "something"; }   // BAD: common name, takes anything
}
namespace app {
    std::string describe(const geo::Shape&) { return "a shape"; }
    std::string f() { return describe(geo::Circle{}); }   // "something": ADL found geo's template,
}                                                          // an exact match beats Circle -> Shape
```

```cpp
#include <string>
namespace geo {
    struct Shape {};
    struct Circle : Shape {};
    template <class T>
        requires requires(const T& t) { t.name(); }   // GOOD: only for types with a name()
    std::string describe(const T& t) { return t.name(); }
}
namespace app {
    std::string describe(const geo::Shape&) { return "a shape"; }
    std::string f() { return describe(geo::Circle{}); }   // "a shape"
}
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.48: If your compiler does not support concepts, fake them with `enable_if`

Before C++20, `std::enable_if` removes a template from overload resolution
when a condition is false, which gives an early error at the call much as a
concept does. With a C++20 compiler, write the concept.

```cpp compile-fail="invalid operands"
template <class T>                    // BAD: unconstrained; the error is inside the body
T gcd(T a, T b) { return b == 0 ? a : gcd(b, a % b); }
double d = gcd(1.5, 0.5);
```

```cpp
#include <type_traits>
// GOOD (C++11/14/17): only integral types; gcd(1.5, 0.5) is "no matching function"
template <class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
T gcd(T a, T b) { return b == 0 ? a : gcd(b, a % b); }
int g = gcd(12, 18);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.49: Where possible, avoid type-erasure

`std::function` and other type-erased wrappers hide the real type behind an
indirect call (and maybe an allocation). When the operation is known at compile
time, take it as a template parameter. Erase types when you need to store
different callables in one place.

```cpp bad
#include <functional>
#include <vector>
// BAD: every caller passes a lambda, but each call goes through std::function
void transform_all(std::vector<int>& v, const std::function<int(int)>& f) {
    for (int& x : v) x = f(x);
}
```

```cpp
#include <concepts>
#include <functional>
#include <vector>
// GOOD: the callable's own type; the call can inline
template <std::regular_invocable<int> F>
void transform_all(std::vector<int>& v, F f) {
    for (int& x : v) x = std::invoke(f, x);
}
std::vector<std::function<void()>> on_exit;   // fine: different callables in one container
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.60: Minimize a template's context dependencies

A template that uses names from its surroundings depends on what was declared
before it and on what each caller's namespace offers. Prefer the template's
arguments and qualified names; then it means the same thing everywhere.

```cpp compile-fail="not declared|neither visible"
template <class T>
T clamp01(T x) { return limit(x, T{0}, T{1}); }   // BAD: which limit? whatever is around

double limit(double x, double lo, double hi) { return x < lo ? lo : x > hi ? hi : x; }
double c = clamp01(1.5);   // error: limit is declared after the template; ADL can't find it for double
```

```cpp
#include <algorithm>
template <class T>
T clamp01(T x) { return std::clamp(x, T{0}, T{1}); }   // GOOD: says exactly which function
double c = clamp01(1.5);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.61: Do not over-parameterize members (SCARY)

A nested type depends on every parameter of its class template, even the ones
it doesn't use. Two lists that differ only in allocator then have different,
incompatible node types and twice the instantiations. Move the member out, with
only the parameters it needs.

```cpp compile-fail="cannot convert|no matching function"
#include <memory>
template <class T> struct Pool : std::allocator<T> {};
template <class T, class Alloc = std::allocator<T>>
struct List {
    struct Node { T value; Node* next; };   // BAD: depends on Alloc without using it
    Node* head = nullptr;
};
int count(List<int>::Node* n) { int c = 0; for (; n; n = n->next) ++c; return c; }
int n = count(List<int, Pool<int>>{}.head);   // error: a different Node type
```

```cpp
#include <memory>
template <class T> struct Pool : std::allocator<T> {};
template <class T>
struct Node { T value; Node* next; };       // GOOD: depends on T only
template <class T, class Alloc = std::allocator<T>>
struct List {
    using node_type = Node<T>;
    Node<T>* head = nullptr;
};
int count(Node<int>* n) { int c = 0; for (; n; n = n->next) ++c; return c; }
int n = count(List<int, Pool<int>>{}.head);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.62: Place non-dependent class template members in a non-templated base class

Members that don't use the template parameters (constants, enums, helper
types) belong in a plain base class. Then there's one of each, usable without
picking a template argument.

```cpp compile-fail="cannot convert|no matching function"
#include <string>
template <class T>
struct Parser {
    enum class Status { ok, error };   // BAD: a new Status for every T
};
void report(Parser<int>::Status) {}
void f() { report(Parser<std::string>::Status::ok); }   // error: a different enum type
```

```cpp
#include <string>
struct ParserBase {
    enum class Status { ok, error };   // GOOD: one Status for all parsers
};
template <class T>
struct Parser : ParserBase {};
void report(ParserBase::Status) {}
void f() { report(Parser<std::string>::Status::ok); }
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.64: Use specialization to provide alternative implementations of class templates

The primary template gives the general implementation; a specialization swaps
in a different one for a type that needs it, behind the same interface. Users
don't change their code.

```cpp bad
#include <cstring>
#include <string>
#include <vector>
template <class T>
struct Codec {   // BAD: one implementation for every type
    static std::vector<char> encode(const T& v) {
        std::vector<char> out(sizeof v);
        std::memcpy(out.data(), &v, sizeof v);   // for std::string this copies a pointer:
        return out;                              // undefined behaviour, and the text is lost
    }
};
auto bytes = Codec<std::string>::encode("hello");
```

```cpp
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>
template <class T>
struct Codec {                       // general: raw bytes, only where that's valid
    static_assert(std::is_trivially_copyable_v<T>);
    static std::vector<char> encode(const T& v) {
        std::vector<char> out(sizeof v);
        std::memcpy(out.data(), &v, sizeof v);
        return out;
    }
};
template <>
struct Codec<std::string> {          // GOOD: same interface, its own implementation
    static std::vector<char> encode(const std::string& s) { return {s.begin(), s.end()}; }
};
auto bytes = Codec<std::string>::encode("hello");
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.65: Use tag dispatch to provide alternative implementations of a function

Pass a tag object that encodes a property of the type (an iterator category)
and overload on it, so only the matching implementation is instantiated. A
plain `if` on the property compiles both branches for every type. (C++20
concept overloads do the same job, see T.121.)

```cpp compile-fail="no match for 'operator\+='|no viable overloaded '\+='"
#include <iterator>
#include <list>
#include <type_traits>
template <class It>
void skip(It& it, int n) {
    using Cat = typename std::iterator_traits<It>::iterator_category;
    if (std::is_base_of_v<std::random_access_iterator_tag, Cat>) it += n;   // BAD: compiled for every It
    else while (n--) ++it;
}
void f(std::list<int>& l) { auto it = l.begin(); skip(it, 2); }   // error: list iterators have no +=
```

```cpp
#include <iterator>
#include <list>
template <class It>
void skip(It& it, int n, std::random_access_iterator_tag) { it += n; }
template <class It>
void skip(It& it, int n, std::input_iterator_tag) { while (n--) ++it; }

template <class It>
void skip(It& it, int n) {   // GOOD: the tag picks one implementation
    skip(it, n, typename std::iterator_traits<It>::iterator_category{});
}
void f(std::list<int>& l) { auto it = l.begin(); skip(it, 2); }
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.67: Use specialization to provide alternative implementations for irregular types

Some types don't behave like values: `==` on `const char*` compares
addresses, not text. A general template written for regular types gives the
wrong answer for them; specialize it for the irregular type.

```cpp bad
template <class T>
struct Same {
    bool operator()(const T& a, const T& b) const { return a == b; }
};
char x[] = "abc", y[] = "abc";
bool b = Same<const char*>{}(x, y);   // BAD: false; two arrays, two addresses
```

```cpp
#include <cstring>
template <class T>
struct Same {
    bool operator()(const T& a, const T& b) const { return a == b; }
};
template <>
struct Same<const char*> {   // GOOD: compare the text
    bool operator()(const char* a, const char* b) const { return std::strcmp(a, b) == 0; }
};
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.68: Use `{}` rather than `()` within templates to avoid ambiguities

Inside a template, `T x();` declares a function and `T(u)` is a C-style cast
that converts almost anything. Braces always mean "initialize an object" and
reject narrowing.

```cpp compile-fail="cannot initialize return object|could not convert|invalid conversion"
template <class T>
T make_default() {
    T result();       // BAD: declares a function named result
    return result;
}
int i = make_default<int>();
```

```cpp
template <class T>
T make_default() {
    T result{};       // GOOD: a value-initialized object
    return result;
}
template <class T, class U>
T convert(const U& u) { return T{u}; }   // GOOD: convert<int>(2.5) is rejected, not truncated
int i = make_default<int>();
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.69: Inside a template, don't make an unqualified non-member function call unless you intend it to be a customization point

An unqualified call with a dependent argument also searches the argument's
namespace, so any function there with that name can take over. Qualify calls
to your own helpers; leave a call unqualified only where users are meant to
supply their own (like `swap`).

```cpp compile-fail="is ambiguous"
namespace lib {
    template <class T> T normalize(T v) { return v; }
    template <class T> T process(T v) { return normalize(v); }   // BAD: unqualified helper call
}
namespace user {
    struct Angle { double deg; };
    template <class T> T normalize(T v) { return v; }   // happens to share the name
}
auto a = lib::process(user::Angle{370});   // error: lib::normalize or user::normalize?
```

```cpp
#include <utility>
namespace lib {
    namespace detail { template <class T> T normalize(T v) { return v; } }
    template <class T> T process(T v) { return detail::normalize(v); }   // GOOD: qualified

    template <class T> void reorder(T& a, T& b) {
        using std::swap;
        swap(a, b);   // GOOD: deliberately unqualified, a customization point
    }
}
namespace user {
    struct Angle { double deg; };
    template <class T> T normalize(T v) { return v; }
}
auto a = lib::process(user::Angle{370});
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.80: Do not naively templatize a class hierarchy

Virtual functions of a class template are instantiated for every
instantiation, whether they're called or not. That bloats code, and it
demands every operation of every element type, as here where nothing even
calls `sort`.

```cpp compile-fail="no match for 'operator<'|invalid operands to binary expression"
#include <algorithm>
#include <vector>
template <class T>
struct Container {                     // BAD: a templated interface full of virtuals
    std::vector<T> items;
    virtual ~Container() = default;
    virtual void sort() { std::sort(items.begin(), items.end()); }
    virtual std::size_t size() const { return items.size(); }
};
struct Point { int x, y; };            // no <
void f() { Container<Point> c; }       // error: sort() is instantiated anyway
```

```cpp
#include <algorithm>
#include <vector>
template <class T>
struct Container {                     // GOOD: no virtuals; members instantiate only if used
    std::vector<T> items;
    std::size_t size() const { return items.size(); }
};
template <class T>
void sort(Container<T>& c) { std::ranges::sort(c.items); }   // a free algorithm, used on demand
struct Point { int x, y; };
void f() { Container<Point> c; }
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.81: Do not mix hierarchies and arrays

An array of `Derived` converts to a `Base*` silently, and indexing that pointer
steps by `sizeof(Base)`, landing in the middle of objects. Pass a typed
container or a `std::span<Derived>`, which doesn't convert to `std::span<Base>`.

```cpp bad
struct Base { int id = 0; };
struct Derived : Base { double extra = 0; };
void reset(Base* p, int n) { for (int i = 0; i < n; ++i) p[i] = Base{}; }   // steps by sizeof(Base)
Derived items[3];
void f() { reset(items, 3); }   // BAD: compiles; p[1] isn't items[1]: undefined behaviour
```

```cpp
#include <concepts>
#include <span>
struct Base { int id = 0; };
struct Derived : Base { double extra = 0; };
template <std::derived_from<Base> T, std::size_t N>
void reset(std::span<T, N> items) { for (T& x : items) x.id = 0; }   // GOOD: steps by sizeof(T)
Derived items[3];
void f() { reset(std::span{items}); }
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.82: Linearize a hierarchy when virtual functions are undesirable

When behaviour is fixed at compile time, stack it as a chain of class
templates, each deriving from its parameter. Every call is direct and can be
inlined; there's no vtable and no virtual call per event.

```cpp bad
// BAD: layers chained through virtual calls, though the stack never changes at run time
struct Handler { virtual ~Handler() = default; virtual void handle(int) = 0; };
struct Core : Handler { void handle(int) override {} };
struct Counting : Handler {
    Handler& next; long seen = 0;
    explicit Counting(Handler& n) : next(n) {}
    void handle(int e) override { ++seen; next.handle(e); }   // a virtual call per layer
};
```

```cpp
struct Core { void handle(int) {} };

template <class Next>
struct Counting : Next {           // GOOD: each layer derives from the next
    long seen = 0;
    void handle(int e) { ++seen; Next::handle(e); }
};
template <class Next>
struct Filtering : Next {
    void handle(int e) { if (e >= 0) Next::handle(e); }
};
using Pipeline = Filtering<Counting<Core>>;   // one linear hierarchy, no virtuals
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.83: Do not declare a member function template virtual

The language forbids it: a vtable needs a fixed list of functions, and a
template is an open-ended family. Make a non-virtual template that converts
its argument and calls a virtual function with a fixed signature.

```cpp compile-fail="templates may not be 'virtual'|cannot be specified on member function templates"
#include <string>
struct Sink {
    template <class T>
    virtual void write(const T& value) = 0;   // BAD: error: templates can't be virtual
};
```

```cpp
#include <format>
#include <string_view>
class Sink {
public:
    virtual ~Sink() = default;
    template <class T>
    void write(const T& value) { write_text(std::format("{}", value)); }   // GOOD: generic front ...
private:
    virtual void write_text(std::string_view text) = 0;                   // ... virtual core
};
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.84: Use a non-template core implementation to provide an ABI-stable interface

Put the real work (linking nodes, counting) in an ordinary class that can be
compiled once, in a `.cpp`, and keep the template a thin, type-safe layer of
casts over it. Every `List<T>` then shares one copy of the code.

```cpp bad
// BAD: all the linking logic is in the template, so it's compiled again for
// every T, and any change to it means recompiling every user
template <class T>
class List {
    struct Node { Node* next; T value; };
    Node* head = nullptr;
    std::size_t n = 0;
public:
    void push_front(const T& v) { head = new Node{head, v}; ++n; }
    // ...
};
```

```cpp
#include <cstddef>
struct NodeBase { NodeBase* next = nullptr; };
class ListBase {                                    // GOOD: non-template core, could live in a .cpp
protected:
    NodeBase* head_ = nullptr;
    std::size_t size_ = 0;
    void link_front(NodeBase* n) { n->next = head_; head_ = n; ++size_; }
public:
    std::size_t size() const { return size_; }
};

template <class T>
struct Node : NodeBase { T value; explicit Node(const T& v) : value(v) {} };

template <class T>
class List : public ListBase {                      // thin, type-safe layer
public:
    void push_front(const T& v) { link_front(new Node<T>(v)); }
    T& front() { return static_cast<Node<T>*>(head_)->value; }
    ~List() { while (head_) { auto* n = static_cast<Node<T>*>(head_); head_ = n->next; delete n; } }
};
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.100: Use variadic templates when you need a function that takes a variable number of arguments of a variety of types

C varargs (`...` with `va_arg`) lose every argument's type: the callee guesses
and a wrong guess is undefined behaviour. A variadic template keeps each type,
so every argument is handled correctly and checked.

```cpp bad
#include <cstdarg>
double sum(int count, ...) {             // BAD: the types are lost
    va_list args;
    va_start(args, count);
    double s = 0;
    for (int i = 0; i < count; ++i) s += va_arg(args, double);
    va_end(args);
    return s;
}
double d = sum(2, 1, 2);                 // undefined behaviour: ints read as doubles
```

```cpp
#include <sstream>
#include <string>
template <class... Ts>
std::string join(const Ts&... args) {    // GOOD: each argument keeps its type
    std::ostringstream out;
    ((out << args), ...);                 // a fold over the comma operator
    return out.str();
}
auto s = join("x = ", 1, ", y = ", 2.5);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.101: ??? How to pass arguments to a variadic template ???

The official rule is still a placeholder, noting move-only and reference
arguments. Take the pack as forwarding references (`Args&&...`) and pass it on
with `std::forward`, so lvalues stay lvalues and move-only arguments move.

```cpp compile-fail="use of deleted function|implicitly-deleted"
#include <memory>
struct Holder { std::unique_ptr<int> p; explicit Holder(std::unique_ptr<int> q) : p(std::move(q)) {} };
template <class T, class... Args>
T build(Args... args) { return T(args...); }   // BAD: copies; args are lvalues in here
auto h = build<Holder>(std::make_unique<int>(1));   // error: unique_ptr can't be copied
```

```cpp
#include <memory>
#include <utility>
struct Holder { std::unique_ptr<int> p; explicit Holder(std::unique_ptr<int> q) : p(std::move(q)) {} };
template <class T, class... Args>
T build(Args&&... args) { return T(std::forward<Args>(args)...); }   // GOOD: perfect forwarding
auto h = build<Holder>(std::make_unique<int>(1));
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.102: How to process arguments to a variadic template

The official rule is still a placeholder. Prefer a fold expression or a pack
expansion, which handle every element (and the empty pack) in one expression,
over recursion that peels one argument at a time and needs a base case.

```cpp compile-fail="no matching function"
#include <cstddef>
template <class T, class... Rest>
std::size_t total_size(const T& first, const Rest&... rest) {
    return sizeof first + total_size(rest...);   // BAD: recursion with no base case
}
std::size_t n = total_size(1, 2.0);   // error: total_size() with no arguments doesn't exist
```

```cpp
#include <cstddef>
#include <type_traits>
template <class... Ts>
std::size_t total_size(const Ts&...) { return (std::size_t{0} + ... + sizeof(Ts)); }   // GOOD: a fold
template <class... Ts>
constexpr bool all_integral = (std::is_integral_v<Ts> && ...);                        // checks each type
std::size_t n = total_size(1, 2.0);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.103: Don't use variadic templates for homogeneous argument lists

When every argument has the same type, say so: a `std::initializer_list<T>`
or a `std::span<const T>`. A variadic template accepts any mix of types and
creates a new instantiation for each argument count.

```cpp bad
template <class... Ts>
int sum(Ts... xs) { return (0 + ... + xs); }   // BAD: meant for ints, takes anything
int s = sum(1, 2.9, 3);                        // compiles: 2.9 is silently truncated to 2
```

```cpp
#include <initializer_list>
int sum(std::initializer_list<int> xs) {       // GOOD: a list of ints; {1, 2.9} is rejected
    int s = 0;
    for (int x : xs) s += x;
    return s;
}
int s = sum({1, 2, 3});
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.120: Use template metaprogramming only when you really need to

Template metaprograms are hard to read, slow to compile and painful to
debug. Before writing one, check whether a fold expression, a `constexpr`
function or a concept says it directly.

```cpp bad
#include <type_traits>
// BAD: a recursive metaprogram for "is T one of these types?"
template <class T, class... Ts> struct one_of : std::false_type {};
template <class T, class U, class... Ts>
struct one_of<T, U, Ts...> : std::conditional_t<std::is_same_v<T, U>, std::true_type, one_of<T, Ts...>> {};
```

```cpp
#include <concepts>
template <class T, class... Ts>
concept one_of = (std::same_as<T, Ts> || ...);   // GOOD: one line, no recursion
static_assert(one_of<int, char, int> && !one_of<long, char, int>);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.121: Use template metaprogramming primarily to emulate concepts

Overloading on properties of types is the most common reason for TMP, and
before C++20 `enable_if` was the way. It can't say that one condition refines
another, so overlapping conditions are ambiguous. Concepts can, so use them
when you have C++20.

```cpp compile-fail="is ambiguous"
#include <iterator>
#include <type_traits>
#include <vector>
template <class It, std::enable_if_t<std::is_base_of_v<std::forward_iterator_tag,
                     typename std::iterator_traits<It>::iterator_category>, int> = 0>
int step(It) { return 1; }   // BAD: forward iterators ...
template <class It, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag,
                     typename std::iterator_traits<It>::iterator_category>, int> = 0>
int step(It) { return 2; }   // ... and random access: both true for a vector
int s = step(std::vector<int>{}.begin());   // error: ambiguous
```

```cpp
#include <iterator>
#include <vector>
int step(std::forward_iterator auto)       { return 1; }
int step(std::random_access_iterator auto) { return 2; }   // GOOD: refines forward_iterator
int s = step(std::vector<int>{}.begin());                  // 2
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.122: Use templates (usually template aliases) to compute types at compile time

When a type depends on properties of other types, compute it with an alias
template over the standard traits, rather than spelling out a choice for each
type by hand.

```cpp bad
#include <string>
// BAD: the "pass small types by value" decision repeated by hand for each type
void send(int v);
void send(double v);
void send(const std::string& v);
void send(const std::vector<int>& v);
// ... and every new type needs a new decision
```

```cpp
#include <string>
#include <type_traits>
template <class T>   // GOOD: the rule, computed once for every T
using In = std::conditional_t<std::is_trivially_copyable_v<T> && sizeof(T) <= 2 * sizeof(void*),
                              T, const T&>;
template <class T>
void send(In<T> v) { use(v); }
static_assert(std::is_same_v<In<int>, int> && std::is_same_v<In<std::string>, const std::string&>);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.123: Use `constexpr` functions to compute values at compile time

To compute a value, write a function: a `constexpr` function reads like
ordinary code, works at run time too, and usually compiles faster than a
recursive class template.

```cpp bad
// BAD: a value computed by recursive instantiation
template <long B, int E> struct Power { static constexpr long value = B * Power<B, E - 1>::value; };
template <long B> struct Power<B, 0> { static constexpr long value = 1; };
static_assert(Power<2, 10>::value == 1024);
```

```cpp
constexpr long power(long base, int exp) {   // GOOD: an ordinary loop
    long r = 1;
    while (exp-- > 0) r *= base;
    return r;
}
static_assert(power(2, 10) == 1024);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.124: Prefer to use standard-library TMP facilities

`<type_traits>` and `<concepts>` are tested, portable and known to every
reader. A homemade trait is one more thing to learn, and it's easy to miss a
case.

```cpp compile-fail="static assertion failed"
#include <type_traits>
template <class T> struct strip_ref { using type = T; };
template <class T> struct strip_ref<T&> { using type = T; };   // BAD: forgot T&&
static_assert(std::is_same_v<strip_ref<int&&>::type, int>);    // fails: still int&&
```

```cpp
#include <type_traits>
static_assert(std::is_same_v<std::remove_cvref_t<int&&>, int>);         // GOOD
static_assert(std::is_same_v<std::remove_cvref_t<const int&>, int>);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.125: If you need to go beyond the standard-library TMP facilities, use an existing library

*No example: it's advice to use an established third-party library (such as Boost.Mp11) instead of writing one, which our snippets can't include.*

## T.140: If an operation can be reused, give it a name

The same rule as F.10 for templates: an operation used in several places gets
a name, so it's written, tested and fixed once, and the call sites say what
they do.

```cpp bad
#include <algorithm>
#include <string>
#include <vector>
void f(std::vector<std::string>& names, const std::string& s) {
    // BAD: the same comparison written twice; change one and they disagree
    std::ranges::sort(names, [](const std::string& a, const std::string& b) { return a.size() < b.size(); });
    auto it = std::ranges::lower_bound(names, s, [](const std::string& a, const std::string& b) { return a.size() < b.size(); });
}
```

```cpp
#include <algorithm>
#include <string>
#include <vector>
struct ShorterFirst {   // GOOD: named once
    bool operator()(const std::string& a, const std::string& b) const { return a.size() < b.size(); }
};
void f(std::vector<std::string>& names, const std::string& s) {
    std::ranges::sort(names, ShorterFirst{});
    auto it = std::ranges::lower_bound(names, s, ShorterFirst{});
    use(it);
}
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.141: Use an unnamed lambda if you need a simple function object in one place only

The same rule as F.11: a small operation used once reads best where it's
used. A named function object declared elsewhere makes the reader jump away.

```cpp bad
#include <algorithm>
#include <vector>
struct IsNegative { bool operator()(int x) const { return x < 0; } };   // BAD: named, used once, far away
// ... many lines later ...
void f(std::vector<int>& v) { std::erase_if(v, IsNegative{}); }
```

```cpp
#include <vector>
void f(std::vector<int>& v) {
    std::erase_if(v, [](int x) { return x < 0; });   // GOOD: the operation is right here
}
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.142?: Use template variables to simplify notation

A variable template (C++14) names a value that depends on a type, so callers
write `tau<float>` instead of `constants<float>::tau()` or
`trait<T>::value`.

```cpp bad
#include <numbers>
template <class T>
struct Constants {   // BAD: a class and a function call just to name a value
    static constexpr T tau() { return 2 * std::numbers::pi_v<T>; }
};
float t = Constants<float>::tau();
```

```cpp
#include <concepts>
#include <numbers>
template <std::floating_point T>
inline constexpr T tau = 2 * std::numbers::pi_v<T>;   // GOOD: a value per type
float t = tau<float>;
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.143: Don't write unintentionally non-generic code

Use the weakest operation that does the job: `!=` rather than `<` for
iterators, `empty()` rather than `size() == 0`, the base class rather than a
derived one. Otherwise the code rejects types it could have served.

```cpp compile-fail="no match for 'operator<'|invalid operands to binary expression"
#include <list>
template <class It>
int count_all(It first, It last) {
    int n = 0;
    for (auto i = first; i < last; ++i) ++n;   // BAD: < needs random access
    return n;
}
std::list<int> l{1, 2, 3};
int n = count_all(l.begin(), l.end());   // error: list iterators have no <
```

```cpp
#include <list>
template <class It>
int count_all(It first, It last) {
    int n = 0;
    for (auto i = first; i != last; ++i) ++n;  // GOOD: any iterator
    return n;
}
std::list<int> l{1, 2, 3};
int n = count_all(l.begin(), l.end());
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.144: Don't specialize function templates

An explicit specialization of a function template doesn't take part in
overload resolution: the compiler first picks a primary template, and only then
looks at that template's specializations. So a specialization can be silently
ignored. Overload instead.

```cpp bad
template <class T> int which(T)  { return 0; }
template <> int which<int*>(int*) { return 1; }   // BAD: specializes which(T) ...
template <class T> int which(T*) { return 2; }    // ... but this overload is a better match
int* p = nullptr;
int w = which(p);   // 2: the specialization for int* is never called
```

```cpp
template <class T> int which(T)  { return 0; }
template <class T> int which(T*) { return 2; }
int which(int*) { return 1; }   // GOOD: an overload; it wins for int*
int* p = nullptr;
int w = which(p);   // 1
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).

## T.150: Check that a class matches a concept using `static_assert`

If a class is meant to model a concept, assert it right after the class. A
mistake then shows up at the class, in one line, not later in a user's call to
an algorithm.

```cpp compile-fail="no match for call|no matching function for call"
#include <algorithm>
#include <vector>
struct Temperature { double deg; };   // BAD: meant to be regular, but has no ==; nothing says so
bool has(const std::vector<Temperature>& v, Temperature t) {
    return std::ranges::find(v, t) != v.end();   // the mistake surfaces here, in a user's code
}
```

```cpp
#include <compare>
#include <concepts>
struct Temperature {
    double deg;
    auto operator<=>(const Temperature&) const = default;
};
static_assert(std::regular<Temperature>);          // GOOD: checked where the class is defined
static_assert(std::totally_ordered<Temperature>);
```

Tested by [`tests/guidelines/templates_test.cpp`](../../../../tests/guidelines/templates_test.cpp).
