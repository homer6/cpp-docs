# F: Functions — examples

> Our examples for the [F rules](../functions.md): small, well-named functions,
> parameters that say how they're used, and return values that don't dangle.

## F.1: "Package" meaningful operations as carefully named functions

When a piece of code does one recognisable job, pull it out and name it. The
name documents the job, the piece can be reused and tested on its own, and the
surrounding code reads as a list of steps.

```cpp nocompile
#include <iostream>
#include <string>
void report(const std::string& line) {
    // BAD: summing the digits, judging the total and printing are tangled
    // together, the output stream is fixed, and none of it can be reused
    int total = 0;
    for (char c : line)
        if (c >= '0' && c <= '9') total += c - '0';
    if (total > 9) std::cout << "big: " << total << '\n';
    else std::cerr << "small\n";
}
```

```cpp
#include <ostream>
#include <string_view>
int digit_sum(std::string_view s) {
    int total = 0;
    for (char c : s)
        if (c >= '0' && c <= '9') total += c - '0';
    return total;
}
bool is_big(int total) { return total > 9; }

void report(std::ostream& out, std::string_view line) {   // GOOD: reads as two named steps
    const int total = digit_sum(line);
    out << (is_big(total) ? "big: " : "small: ") << total << '\n';
}
```

The BAD version compiles, so it's tagged `nocompile`: the problem is the
structure, which a reviewer has to catch.

## F.2: A function should perform a single logical operation

A function that does one thing is easy to name, test and reuse. One that
parses, transforms and prints can only ever be used for exactly that
combination.

```cpp nocompile
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
void parse_sort_print(const std::string& csv) {   // BAD: three jobs in one
    std::vector<int> v;
    std::istringstream in(csv);
    for (std::string cell; std::getline(in, cell, ',');) v.push_back(std::stoi(cell));
    std::sort(v.begin(), v.end());
    for (int x : v) std::cout << x << ' ';
}
```

```cpp
#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
std::vector<int> parse_csv(const std::string& csv) {   // GOOD: one job each
    std::vector<int> v;
    std::istringstream in(csv);
    for (std::string cell; std::getline(in, cell, ',');) v.push_back(std::stoi(cell));
    return v;
}
std::vector<int> sorted(std::vector<int> v) {
    std::ranges::sort(v);
    return v;
}
void print(std::ostream& out, const std::vector<int>& v) {
    for (int x : v) out << x << ' ';
}
```

## F.3: Keep functions short and simple

Long functions with deep branching hide bugs: it's hard to see that every case
is handled. Split the cases into small functions and keep the top level a
short dispatch.

```cpp nocompile
double shipping(double kg, bool express, bool international, bool member) {
    double cost = 0;                 // BAD: every combination in one body
    if (international) {
        cost = 20 + kg * 4;
        if (express) { cost *= 2; if (member) cost -= 10; }
        else if (member) cost -= 5;
    } else {
        cost = 5 + kg;
        if (express) { cost += 15; if (member) cost -= 15; }
        else if (member) cost = 0;
    }
    return cost < 0 ? 0 : cost;
}
```

```cpp
double domestic(double kg, bool express, bool member) {
    if (!express) return member ? 0 : 5 + kg;
    return member ? 5 + kg : 20 + kg;
}
double international(double kg, bool express, bool member) {
    const double base = 20 + kg * 4;
    if (!express) return member ? base - 5 : base;
    return member ? 2 * base - 10 : 2 * base;
}
double shipping(double kg, bool express, bool intl, bool member) {   // GOOD: a one-line dispatch
    return intl ? international(kg, express, member) : domestic(kg, express, member);
}
```

## F.4: If a function might have to be evaluated at compile time, declare it `constexpr`

Only a `constexpr` (or `consteval`) function can be called in a constant
expression: an array bound, a template argument, a `static_assert`. Without it
the compiler refuses, however simple the body. `constexpr` still lets the same
function run at run time.

```cpp compile-fail="non-constant|non-'constexpr'|not an integral constant expression"
int cube(int x) { return x * x * x; }   // BAD: can't be used at compile time
static_assert(cube(3) == 27);
```

```cpp
constexpr int cube(int x) { return x * x * x; }   // GOOD
static_assert(cube(3) == 27);                     // compile time
int run_time(int n) { return cube(n); }           // and run time
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.5: If a function is very small and time-critical, declare it `inline`

A tiny function called in a hot loop is cheapest when the compiler can see its
body at the call site. Defining it in the header (`inline`, or inside the class
body, which is implicitly `inline`) makes that possible without link-time
optimization. Keep large functions and stable ABIs out of line, and measure.

```cpp nocompile
// point.h
struct Point {
    double x, y;
    double norm2() const;          // BAD: one multiply-add, called millions of times...
};
// point.cpp
double Point::norm2() const { return x * x + y * y; }   // ...but every call from another file is a real call
```

```cpp
// point.h
struct Point {
    double x, y;
    double norm2() const { return x * x + y * y; }   // GOOD: defined in the class, so inline
};
inline double dot(Point a, Point b) { return a.x * b.x + a.y * b.y; }   // GOOD: inline free function
```

The BAD version is valid C++; it's tagged `nocompile` because it shows two
files. `constexpr` functions are implicitly `inline` too.

## F.6: If your function must not throw, declare it `noexcept`

`noexcept` is a promise that callers and the library can check. `std::vector`
moves its elements when it grows only if the move constructor is `noexcept`;
otherwise it copies them to keep its strong guarantee. Destructors, `swap` and
move operations should be `noexcept`.

```cpp nocompile
#include <string>
#include <utility>
struct Track {
    std::string title;
    Track() = default;
    Track(const Track&) = default;
    Track(Track&& other) : title(std::move(other.title)) {}   // BAD: can't throw, but doesn't say so,
                                                              // so a growing vector<Track> copies every element
};
```

```cpp
#include <string>
#include <utility>
struct Track {
    std::string title;
    Track() = default;
    Track(const Track&) = default;
    Track(Track&& other) noexcept : title(std::move(other.title)) {}   // GOOD
};
static_assert(std::is_nothrow_move_constructible_v<Track>);
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.7: For general use, take `T*` or `T&` arguments rather than smart pointers

A function that only uses an object shouldn't care how its caller owns it.
Taking a smart pointer ties it to one ownership scheme and rejects objects on
the stack, in containers or owned by a different kind of smart pointer.

```cpp compile-fail="invalid initialization|no matching function"
#include <memory>
struct Gauge { int value = 0; };
int level(const std::shared_ptr<Gauge>& g) { return g->value; }   // BAD: only reads the Gauge
Gauge local;
int v = level(local);   // error: a plain Gauge isn't a shared_ptr
```

```cpp
#include <memory>
struct Gauge { int value = 0; };
int level(const Gauge& g) { return g.value; }   // GOOD: any Gauge will do
Gauge local;
auto shared = std::make_shared<Gauge>();
auto unique = std::make_unique<Gauge>();
int a = level(local), b = level(*shared), c = level(*unique);
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.8: Prefer pure functions

A pure function's result depends only on its arguments and it changes nothing
else. It's easy to test, safe to call from several threads, can be cached, and
can often be `constexpr`.

```cpp nocompile
double tax_rate = 0.2;
int calls = 0;
double with_tax(double price) {
    ++calls;                         // BAD: hidden side effect
    return price * (1 + tax_rate);   // BAD: result depends on a global
}
```

```cpp
constexpr double with_tax(double price, double rate) {   // GOOD: depends only on its arguments
    return price * (1 + rate);
}
static_assert(with_tax(100.0, 0.25) == 125.0);
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.9: Unused parameters should be unnamed

A parameter the body never reads is clearer without a name: readers don't look
for its use, and `-Wunused-parameter` stays quiet. When it's unused only on
some paths, mark it `[[maybe_unused]]`.

```cpp nocompile
struct Event { int code; };
void on_event(const Event& e, int priority) {   // BAD: priority is never read (-Wunused-parameter warns)
    use(e.code);
}
```

```cpp
struct Event { int code; };
void on_event(const Event& e, int /*priority*/) {   // GOOD: the interface needs it, the body doesn't
    use(e.code);
}
template <bool Log>
void on_event_logged(const Event& e, [[maybe_unused]] int priority) {
    if constexpr (Log) use(priority);   // used only when Log is true
    use(e.code);
}
```

## F.10: If an operation can be reused, give it a name

When the same non-trivial lambda shows up twice, it's a function waiting to be
named. A name documents what it does, and a fix lands in one place.

```cpp nocompile
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
bool has_user(const std::vector<std::string>& names, const std::string& who) {
    // BAD: a case-insensitive compare written inline; the next caller will copy it
    return std::ranges::any_of(names, [&](const std::string& n) {
        return std::ranges::equal(n, who, [](unsigned char a, unsigned char b) {
            return std::tolower(a) == std::tolower(b);
        });
    });
}
```

```cpp
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
bool equals_ignore_case(const std::string& a, const std::string& b) {   // GOOD: named and reusable
    return std::ranges::equal(a, b, [](unsigned char x, unsigned char y) {
        return std::tolower(x) == std::tolower(y);
    });
}
bool has_user(const std::vector<std::string>& names, const std::string& who) {
    return std::ranges::any_of(names, [&](const std::string& n) { return equals_ignore_case(n, who); });
}
```

## F.11: Use an unnamed lambda if you need a simple function object in one place only

For a short predicate used once, an unnamed lambda at the call site is the
clearest: the logic sits where it's used, with no class or function to look up
elsewhere.

```cpp nocompile
#include <algorithm>
#include <vector>
struct IsNegative {                    // BAD: a whole class for a one-off test,
    bool operator()(int x) const { return x < 0; }   // defined away from its only use
};
long negatives(const std::vector<int>& v) { return std::ranges::count_if(v, IsNegative{}); }
```

```cpp
#include <algorithm>
#include <vector>
long negatives(const std::vector<int>& v) {
    return std::ranges::count_if(v, [](int x) { return x < 0; });   // GOOD
}
```

## F.15: Prefer simple and conventional ways of passing information

Use the usual conventions (value or `const&` for inputs, `&` for in-out, the
return value for outputs) so readers know what a signature means at a glance.
Clever schemes surprise readers and rarely pay off; measure before using one.

```cpp nocompile
#include <cstddef>
#include <string>
// BAD: an untyped buffer for input, an rvalue reference "for speed" that
// forces callers to std::move or copy by hand, and a status code instead of a result
int summarize(const void* data, std::size_t bytes, std::string&& label, double* out_mean);
```

```cpp
#include <span>
#include <string>
struct Summary { double mean; std::string label; };
Summary summarize(std::span<const double> data, const std::string& label);   // GOOD: conventional
```

## F.16: For "in" parameters, pass cheaply-copied types by value and others by reference to `const`

Small types (a few machine words: `int`, `double`, pointers, `std::string_view`,
`std::span`) are fastest by value. Anything that may be expensive to copy
(`std::string`, containers) goes by `const&`, which still accepts temporaries.

```cpp nocompile
#include <string>
#include <vector>
std::size_t words(std::vector<std::string> lines);   // BAD: copies every string on each call
int twice(const int& x);                             // BAD: an indirection to read an int
```

```cpp
#include <string>
#include <vector>
std::size_t words(const std::vector<std::string>& lines);   // GOOD: no copy
int twice(int x);                                           // GOOD: cheap to copy
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.17: For "in-out" parameters, pass by reference to non-`const`

When a function reads an argument and changes it for the caller, take it by
`T&`. The signature then says "this will be modified"; by value, the change is
silently lost.

```cpp nocompile
#include <string>
void normalize(std::string path) {      // BAD: changes a copy; the caller's string is untouched
    if (!path.empty() && path.back() == '/') path.pop_back();
}
```

```cpp
#include <string>
void normalize(std::string& path) {     // GOOD: in-out, and the signature says so
    if (!path.empty() && path.back() == '/') path.pop_back();
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.18: For "will-move-from" parameters, pass by `X&&` and `std::move` the parameter

A function that takes over an argument's contents should take `X&&` and move
from it. An lvalue then won't bind without an explicit `std::move` at the call
site, so giving up the object is visible. `std::move` on a `const&` doesn't
move; it quietly copies.

```cpp nocompile
#include <utility>
#include <vector>
struct Inbox {
    std::vector<int> items;
    void take(const std::vector<int>& v) { items = std::move(v); }   // BAD: const, so this copies
};
```

```cpp
#include <utility>
#include <vector>
struct Inbox {
    std::vector<int> items;
    void take(std::vector<int>&& v) { items = std::move(v); }   // GOOD: really moves
};
void fill(Inbox& box) {
    std::vector<int> batch{1, 2, 3};
    box.take(std::move(batch));   // the caller says it gives batch up
}
```

Move-only, cheap-to-move types such as `std::unique_ptr` can simply be taken
by value.

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.19: For "forward" parameters, pass by `TP&&` and only `std::forward` the parameter

A function that only passes an argument on should take a forwarding reference
(`T&&` with `T` deduced) and `std::forward` it, once, so an rvalue stays an
rvalue and is moved, not copied.

```cpp nocompile
#include <string>
#include <vector>
template <class T>
void push(std::vector<std::string>& v, T&& s) {
    v.push_back(s);                   // BAD: s is an lvalue here, so rvalues get copied
}
```

```cpp
#include <string>
#include <utility>
#include <vector>
template <class T>
void push(std::vector<std::string>& v, T&& s) {
    v.push_back(std::forward<T>(s));  // GOOD: moves rvalues, copies lvalues
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.20: For "out" output values, prefer return values to output parameters

A return value says plainly what the function produces. An output parameter
could be in-out or out-only, needs a pre-made object, and makes the call
awkward. Returning a local is cheap: it's elided or moved, never copied.

```cpp nocompile
#include <vector>
void evens(const std::vector<int>& in, std::vector<int>& out) {   // BAD: is out cleared first? appended to?
    for (int x : in)
        if (x % 2 == 0) out.push_back(x);
}
```

```cpp
#include <vector>
std::vector<int> evens(const std::vector<int>& in) {   // GOOD
    std::vector<int> out;
    for (int x : in)
        if (x % 2 == 0) out.push_back(x);
    return out;   // elided or moved
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.21: To return multiple "out" values, prefer returning a struct

Several results come back best as a named `struct`; its member names document
them, and structured bindings unpack them at the call site. Use `pair` or
`tuple` only when the values have no better names.

```cpp nocompile
#include <string>
bool parse_port(const std::string& s, int& port, std::string& error);   // BAD: two out-parameters
```

```cpp
#include <charconv>
#include <string>
#include <string_view>
struct PortResult { int port = 0; std::string error; };
PortResult parse_port(std::string_view s) {   // GOOD
    int port = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), port);
    if (ec != std::errc{} || ptr != s.data() + s.size()) return {0, "not a number"};
    return {port, ""};
}
void connect_to(std::string_view arg) {
    if (auto [port, error] = parse_port(arg); error.empty()) use(port);
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.60: Prefer `T*` over `T&` when "no argument" is a valid option

A reference always refers to an object; there's no null reference. When
"nothing" is a valid input, take a pointer (or `std::optional`) and check it.
When it isn't, a reference saves the check.

```cpp compile-fail="invalid initialization|no matching function"
#include <string>
struct Style { int size = 12; };
std::string render(const std::string& text, const Style& s) { return text; }   // BAD: the style is optional...
std::string r = render("hi", nullptr);   // ...but callers can't say "no style"
```

```cpp
#include <string>
struct Style { int size = 12; };
std::string render(const std::string& text, const Style* s) {   // GOOD: nullptr means "default style"
    const int size = s ? s->size : 12;
    return text + "@" + std::to_string(size);
}
std::string r = render("hi", nullptr);
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.22: Use `T*` or `owner<T*>` to designate a single object

A raw pointer in a signature should mean "one object, not owned by me". Use
other types for the other meanings: `std::span` for a sequence, a string type
for text, and `owner<T*>` (the Guidelines Support Library's annotation; better,
a smart pointer) for a pointer that must be deleted.

```cpp nocompile
void reset(int* values, int n, char* name, int* scratch) {
    values[n - 1] = 0;   // BAD: is values really n elements long?
    use(name);           // BAD: zero-terminated? may it be null?
    delete scratch;      // BAD: does this function own scratch?
}
```

```cpp
#include <span>
#include <string_view>
template <class T> using owner = T;   // like gsl::owner: documents ownership, adds nothing at run time

void reset(std::span<int> values, std::string_view name, owner<int*> scratch) {   // GOOD
    if (!values.empty()) values.back() = 0;
    use(name);
    delete scratch;
}
```

## F.23: Use a `not_null<T>` to indicate that "null" is not a valid value

A plain pointer leaves both sides guessing who checks for null. A
`not_null<T*>` parameter puts that duty on the caller and lets the callee skip
the check. The GSL provides `gsl::not_null`; a minimal version rejects a
literal `nullptr` at compile time and costs nothing at run time.

```cpp nocompile
struct Record { int size; };
int length(const Record* r) { return r->size; }   // BAD: may r be null? crashes if it is
int n = length(nullptr);                          // compiles
```

```cpp
#include <cassert>
#include <cstddef>
template <class P>
class not_null {
    P p_;
public:
    not_null(P p) : p_(p) { assert(p_ != nullptr); }
    not_null(std::nullptr_t) = delete;   // a literal nullptr doesn't compile
    P get() const { return p_; }
    decltype(auto) operator->() const { return p_; }
};

struct Record { int size; };
int length(not_null<const Record*> r) { return r->size; }   // GOOD: the caller guarantees non-null
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.24: Use a `span<T>` or a `span_p<T>` to designate a half-open sequence

A pointer plus a count is two arguments that can disagree. `std::span` carries
both together, is built from arrays and containers automatically, and is as
cheap to pass as the pair. (`span_p`, a span ending at a sentinel, is a GSL
idea with no standard type.)

```cpp nocompile
int sum(const int* p, int n) {   // BAD: nothing ties n to the real size of p
    int total = 0;
    for (int i = 0; i < n; ++i) total += p[i];
    return total;
}
int a[3] = {1, 2, 3};
int s = sum(a, 4);               // reads past the end; compiles fine
```

```cpp
#include <span>
#include <vector>
int sum(std::span<const int> values) {   // GOOD: the size travels with the data
    int total = 0;
    for (int v : values) total += v;
    return total;
}
int a[3] = {1, 2, 3};
std::vector<int> v{4, 5};
int s = sum(a) + sum(v);
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.25: Use a `zstring` or a `not_null<zstring>` to designate a C-style string

A `const char*` might be one character, a buffer or a zero-terminated string.
Name the zero-terminated case (the GSL calls it `czstring`, or `zstring` when
mutable) so readers know a terminator is required. If you don't need the
terminator, take `std::string_view`, which knows its length.

```cpp nocompile
#include <cstddef>
std::size_t spaces(const char* s);   // BAD: zero-terminated? may s be null?
```

```cpp
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string_view>
using czstring = const char*;   // like gsl::czstring: a zero-terminated string

std::size_t spaces(czstring s) {   // GOOD: says a terminator is required
    return s ? static_cast<std::size_t>(std::count(s, s + std::strlen(s), ' ')) : 0;
}
std::size_t spaces(std::string_view s) {   // GOOD: no terminator needed
    return static_cast<std::size_t>(std::ranges::count(s, ' '));
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.26: Use a `unique_ptr<T>` to transfer ownership where a pointer is needed

When a function hands an object to its caller through a pointer (a factory
returning a derived class, say), return `std::unique_ptr`. The caller owns it,
can't forget to delete it, and the transfer costs no more than a raw pointer.

```cpp nocompile
struct Shape { virtual ~Shape() = default; virtual double area() const = 0; };
struct Square : Shape { double s = 1; double area() const override { return s * s; } };
Shape* make_shape() { return new Square; }   // BAD: who deletes it? one early return leaks it
```

```cpp
#include <memory>
struct Shape { virtual ~Shape() = default; virtual double area() const = 0; };
struct Square : Shape { double s = 1; double area() const override { return s * s; } };
std::unique_ptr<Shape> make_shape() { return std::make_unique<Square>(); }   // GOOD
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.27: Use a `shared_ptr<T>` to share ownership

When several parts of a program own an object and none knows which finishes
last, `std::shared_ptr` deletes it when the last owner lets go. Prefer
`unique_ptr` when there's only ever one owner at a time: the shared count costs
atomic operations.

```cpp nocompile
struct Image { int w, h; };
struct Viewer { Image* img; ~Viewer() { delete img; } };   // BAD: two viewers sharing one image
void show() {                                              // both delete it: double free
    Image* img = new Image{640, 480};
    Viewer a{img}, b{img};
}
```

```cpp
#include <memory>
struct Image { int w, h; };
struct Viewer { std::shared_ptr<const Image> img; };   // GOOD: the last viewer frees it
void show() {
    auto img = std::make_shared<const Image>(640, 480);
    Viewer a{img}, b{img};
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.42: Return a `T*` to indicate a position (only)

A returned raw pointer should mean "here it is" (or `nullptr`, "not found"),
pointing into something the caller already has. It should never mean "you own
this now"; return a smart pointer for that.

```cpp nocompile
#include <cstddef>
int* make_buffer(std::size_t n) { return new int[n]{}; }   // BAD: a raw pointer that must be delete[]d
```

```cpp
#include <string>
struct Node { std::string name; Node* left = nullptr; Node* right = nullptr; };
const Node* find(const Node* t, const std::string& name) {   // GOOD: a position in the caller's tree
    if (!t || t->name == name) return t;
    if (const Node* p = find(t->left, name)) return p;
    return find(t->right, name);
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.43: Never (directly or indirectly) return a pointer or a reference to a local object

A local dies when the function returns, so a pointer or reference to it
dangles. The compiler catches some cases: returning a reference to a
temporary is ill-formed since C++26 (P2748), and a local returned through a
non-`const` `T&` is an rvalue since C++23 (P2266), so it won't bind. Returning
a local's address, or a `const T&` to it, only gets a warning, and leaks
through lambdas, out-parameters or returned objects get nothing. Return by
value.

```cpp compile-fail="returning reference to (local )?temporary"
#include <string>
const std::string& greeting() {
    return "hello";   // BAD: a reference to a temporary std::string that dies here
}
```

```cpp
#include <string>
std::string greeting() {
    return "hello";   // GOOD: returned by value
}
```

## F.44: Return a `T&` when copy is undesirable and "returning no object" isn't needed

When a function gives access to an object that always exists (a member, an
element), return a reference: no copy, no null check, and changes go to the
real object. Returning by value hands out a copy instead.

```cpp compile-fail="rvalue as lvalue|not assignable"
struct Wheel { int psi = 30; };
struct Car {
    Wheel wheels[4];
    Wheel wheel(int i) const { return wheels[i]; }   // BAD: a copy
};
void inflate(Car& c) { c.wheel(0).psi = 35; }        // error: assigning to a member of a temporary
```

```cpp
struct Wheel { int psi = 30; };
struct Car {
    Wheel wheels[4];
    Wheel& wheel(int i) { return wheels[i]; }                 // GOOD: always refers to a wheel
    const Wheel& wheel(int i) const { return wheels[i]; }
};
void inflate(Car& c) { c.wheel(0).psi = 35; }
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.45: Don't return a `T&&`

An rvalue reference return almost always refers to a temporary that dies at
the end of the full expression, leaving the caller with a dangling reference.
For a pass-through wrapper, return `auto` (a value). `std::move` and
`std::forward` are the exceptions: they are casts used within one expression.

```cpp compile-fail="returning reference to (local )?temporary"
#include <string>
template <class F>
auto&& logged(F f) {
    return f();   // BAD: f() is a temporary; the returned reference dangles
}
auto&& r = logged([] { return std::string("ok"); });
```

```cpp
#include <string>
template <class F>
auto logged(F f) {
    return f();   // GOOD: returned by value
}
auto r = logged([] { return std::string("ok"); });
```

GCC and Clang reject the BAD block in C++26 mode because of P2748 (binding a
returned reference to a temporary is ill-formed); earlier modes only warn.

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.46: `int` is the return type for `main()`

`main` returns `int`; `void main()` isn't C++, however many compilers once
accepted it. Falling off the end of `main` returns 0, so no `return` is
needed.

```cpp compile-fail="must return 'int'"
void main() {}   // BAD: not C++
```

```cpp
int main() {}   // GOOD: returns 0
```

## F.47: Return `T&` from assignment operators

`operator=` should return `*this` as a non-`const` `T&`, like the built-in and
standard types. Anything else breaks code that chains assignments or uses the
result.

```cpp compile-fail="no match for 'operator='|no viable overloaded '='"
struct Meter {
    int v = 0;
    void operator=(const Meter& o) { v = o.v; }   // BAD: returns nothing
};
void reset_all(Meter& a, Meter& b, const Meter& zero) { a = b = zero; }   // error
```

```cpp
struct Meter {
    int v = 0;
    Meter& operator=(const Meter& o) { v = o.v; return *this; }   // GOOD
};
void reset_all(Meter& a, Meter& b, const Meter& zero) { a = b = zero; }
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.48: Don't `return std::move(local)`

Returning a local by name already moves it if the copy isn't elided. Wrapping
it in `std::move` turns the return into a move from a reference, which blocks
the elision (NRVO) and forces a move that needn't happen.

```cpp nocompile
#include <string>
#include <utility>
std::string build() {
    std::string s = "result";
    return std::move(s);   // BAD: always a move; GCC warns with -Wpessimizing-move
}
```

```cpp
#include <string>
std::string build() {
    std::string s = "result";
    return s;              // GOOD: elided, or moved at worst
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.49: Don't return `const T`

A `const` return value can't be moved from, so assigning it to an existing
variable copies instead of moving. The old reason for it, stopping assignment
to a temporary, isn't worth that cost.

```cpp nocompile
#include <vector>
const std::vector<int> load();   // BAD: v = load() copies every element
void refresh(std::vector<int>& v) { v = load(); }
```

```cpp
#include <vector>
std::vector<int> load();          // GOOD: v = load() moves
void refresh(std::vector<int>& v) { v = load(); }
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.50: Use a lambda when a function won't do (to capture local variables, or to write a local function)

A function can't be defined inside another function or see its locals; a
lambda can do both. When neither is needed, or you need overloads, a plain
function is simpler.

```cpp compile-fail="function.definition is not allowed here"
int count_above(const int* v, int n, int limit) {
    bool above(int x) { return x > limit; }   // BAD: no local functions in C++
    int c = 0;
    for (int i = 0; i < n; ++i) c += above(v[i]);
    return c;
}
```

```cpp
#include <algorithm>
#include <vector>
long count_above(const std::vector<int>& v, int limit) {
    auto above = [limit](int x) { return x > limit; };   // GOOD: local, and captures limit
    return std::ranges::count_if(v, above);
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.51: Where there is a choice, prefer default arguments over overloading

When the variants differ only in whether a trailing argument is given, a
default argument keeps one implementation. Overloads are separate bodies that
can drift apart.

```cpp nocompile
#include <string>
std::string pad(const std::string& s, std::size_t width, char fill) {
    return s.size() >= width ? s : std::string(width - s.size(), fill) + s;
}
std::string pad(const std::string& s, std::size_t width) {   // BAD: a second body that has
    return std::string(width - s.size(), ' ') + s;           // already drifted (no size check)
}
```

```cpp
#include <string>
std::string pad(const std::string& s, std::size_t width, char fill = ' ') {   // GOOD: one body
    return s.size() >= width ? s : std::string(width - s.size(), fill) + s;
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.52: Prefer capturing by reference in lambdas that will be used locally, including passed to algorithms

A lambda that runs before the enclosing function returns (an algorithm's
predicate, a local helper) can capture by reference: nothing is copied, and
the lambda can update the caller's variables.

```cpp nocompile
#include <algorithm>
#include <string>
#include <vector>
void tally(const std::vector<std::string>& words, std::vector<std::string> banned) {
    int hits = 0;
    std::ranges::for_each(words, [banned, hits](const std::string& w) mutable {   // BAD: copies banned
        if (std::ranges::find(banned, w) != banned.end()) ++hits;                 // and counts into a copy
    });
    use(hits);   // still 0
}
```

```cpp
#include <algorithm>
#include <string>
#include <vector>
void tally(const std::vector<std::string>& words, const std::vector<std::string>& banned) {
    int hits = 0;
    std::ranges::for_each(words, [&](const std::string& w) {   // GOOD: used here and now
        if (std::ranges::find(banned, w) != banned.end()) ++hits;
    });
    use(hits);
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.53: Avoid capturing by reference in lambdas that will be used non-locally, including returned, stored on the heap, or passed to another thread

A lambda that outlives the current scope (returned, stored, run on another
thread) must not hold references to locals: they'll be gone when it runs.
Capture by value or move the data in.

```cpp nocompile
#include <functional>
#include <string>
std::function<std::string()> make_greeter(std::string name) {
    return [&] { return "hi " + name; };   // BAD: name is destroyed when make_greeter returns
}
```

```cpp
#include <functional>
#include <string>
#include <utility>
std::function<std::string()> make_greeter(std::string name) {
    return [name = std::move(name)] { return "hi " + name; };   // GOOD: the lambda owns a copy
}
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.54: When writing a lambda that captures `this` or any class data member, don't use `[=]` default capture

In a member function, `[=]` looks like it copies everything, but members are
reached through `this`, which is the only thing copied: the lambda sees later
changes and dangles if the object dies. C++20 deprecated that implicit capture.
Write `[this]` when you mean it, or `[*this]` for a real copy.

```cpp nocompile
struct Counter {
    int count = 0;
    auto reader() {
        return [=] { return count; };   // BAD: reads this->count live; deprecated since C++20
    }
};
```

```cpp
struct Counter {
    int count = 0;
    auto live_reader() { return [this] { return count; }; }        // GOOD: says it uses this
    auto snapshot_reader() { return [*this] { return count; }; }   // GOOD: copies the object
};
```

The BAD block compiles with a deprecation warning, so it's tagged `nocompile`.

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.55: Don't use `va_arg` arguments

A C variadic function reads its arguments by trusting that the caller passed
the types it expects; pass a `double` where an `int` is read and the behaviour
is undefined. A variadic template checks every argument's type.

```cpp nocompile
#include <cstdarg>
int sum(int count, ...) {
    va_list args;
    va_start(args, count);
    int total = 0;
    for (int i = 0; i < count; ++i) total += va_arg(args, int);   // BAD: assumes ints
    va_end(args);
    return total;
}
int s = sum(2, 1.5, 2.5);   // undefined behaviour: doubles read as ints
```

```cpp
template <class... Ts>
constexpr auto sum(Ts... xs) { return (xs + ... + 0); }   // GOOD: typed, and no count to get wrong
static_assert(sum(1, 2, 3) == 6);
static_assert(sum(1.5, 2.5) == 4.0);
```

Tested by [`tests/guidelines/functions_test.cpp`](../../../../tests/guidelines/functions_test.cpp).

## F.56: Avoid unnecessary condition nesting

Handle the exceptional cases first with early returns (guard clauses) and keep
the main work at the outer level. Merge conditions instead of nesting them,
and drop an `else` after a `return`.

```cpp nocompile
struct Order { bool paid; bool in_stock; int items; };
int ship(const Order& o) {
    if (o.paid) {                    // BAD: the real work is three levels deep
        if (o.in_stock) {
            if (o.items > 0) {
                return o.items;
            } else {
                return 0;
            }
        }
    }
    return -1;
}
```

```cpp
struct Order { bool paid; bool in_stock; int items; };
int ship(const Order& o) {
    if (!o.paid || !o.in_stock) return -1;   // GOOD: guard first
    if (o.items <= 0) return 0;
    return o.items;
}
```
