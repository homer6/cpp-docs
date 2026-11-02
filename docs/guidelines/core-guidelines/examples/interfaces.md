# I: Interfaces — examples

> Our examples for the [I rules](../interfaces.md): an interface says
> everything a caller must know in its types, so the compiler can check it.

## I.1: Make interfaces explicit

Everything that changes a function's result should be visible at the call:
its parameters. A hidden input, such as a global "mode" flag, makes the same
call return different results, and a hidden output (a global error code) is
easy to ignore.

```cpp nocompile
#include <cmath>
bool round_up = false;                    // BAD: an invisible input
int to_int(double d) { return round_up ? int(std::ceil(d)) : int(d); }
// to_int(7.2) is 7 or 8, depending on code far away
```

```cpp
#include <cmath>
enum class Rounding { down, up };
int to_int(double d, Rounding r) {        // GOOD: the mode is part of the call
    return r == Rounding::up ? int(std::ceil(d)) : int(std::floor(d));
}
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.2: Avoid non-`const` global variables

A mutable global is shared, undeclared state: any function can change it, so
no function's behaviour can be understood on its own, and two threads that
touch it race. Global constants are fine; state belongs to an object you pass
around.

```cpp nocompile
#include <string>
std::string log_prefix = "app";           // BAD: who else writes it?
void log(const std::string& msg) { use(log_prefix + ": " + msg); }
```

```cpp
#include <string>
struct Logger {                           // GOOD: the state has an owner
    std::string prefix;
    void log(const std::string& msg) const { use(prefix + ": " + msg); }
};
constexpr int max_line = 120;             // a global constant is fine
```

## I.3: Avoid singletons

A singleton is a global in disguise, with the same hidden dependencies and
more code. Create the object where it's needed and pass it in. If what you
want is initialization on first use, a function returning a function-local
`static` does that, thread-safely, without a singleton class.

```cpp nocompile
class Settings {
public:
    static Settings& instance() { static Settings s; return s; }   // BAD: global access
    int volume = 5;
private:
    Settings() = default;
};
void play() { use(Settings::instance().volume); }   // a dependency nobody sees
```

```cpp
struct Settings { int volume = 5; };
void play(const Settings& s) { use(s.volume); }     // GOOD: the dependency is a parameter

struct Table { Table() {} };
const Table& lookup_table() {
    static const Table t;    // built on first call; thread-safe since C++11
    return t;
}
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.4: Make interfaces precisely and strongly typed

Types are checked documentation. An `int` timeout could be seconds or
milliseconds, a `void*` could be anything, and three `bool`s in a row are
easy to mix up. Give each parameter a type that says what it is.

```cpp nocompile
void set_timeout(int timeout);       // BAD: in what unit?
void caller() { set_timeout(2); }    // 2 seconds? 2 milliseconds?
```

```cpp
#include <chrono>
void set_timeout(std::chrono::milliseconds timeout);   // GOOD: the unit is the type
void caller() {
    using namespace std::chrono_literals;
    set_timeout(2s);                 // converted exactly: 2000 ms
    set_timeout(1500ms);
}
```

A bare number no longer compiles, because `duration`'s constructor is
`explicit`:

```cpp compile-fail="could not convert|no viable conversion|cannot convert|no matching function"
#include <chrono>
void set_timeout(std::chrono::milliseconds timeout);
void caller() { set_timeout(2); }    // error: which unit?
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.5: State preconditions (if any)

If a function only works for some argument values, say so where the caller
will look, and check it. Then a misuse is a reported bug instead of silent
garbage.

```cpp nocompile
#include <cmath>
double side_of_square(double area) {   // BAD: what about a negative area?
    return std::sqrt(area);            // returns NaN, and the caller never knows
}
```

```cpp
#include <cassert>
#include <cmath>
// Precondition: area >= 0.
double side_of_square(double area) {
    assert(area >= 0 && "side_of_square: area must not be negative");   // GOOD
    return std::sqrt(area);
}
```

## I.6: Prefer `Expects()` for expressing preconditions

A precondition written as a precondition (GSL's `Expects(cond)`, or C++26's
`pre(cond)` on the declaration) is visibly a requirement on the caller, not
ordinary error handling, and tools can see it. An `if` that quietly returns,
or a check deep in the body, reads like normal control flow.

```cpp nocompile
int area(int height, int width) {
    if (height <= 0 || width <= 0) return 0;   // BAD: a precondition disguised as logic
    return height * width;
}
```

```cpp needs=contracts
int area(int height, int width)
    pre(height > 0 && width > 0)               // GOOD: C++26, part of the declaration
{
    return height * width;
}
```

Before C++26 (and today on GCC 15 and Clang 22, which lack contracts), put
`Expects` or an `assert` at the very top of the body.

## I.7: State postconditions

Say what a function guarantees when it returns: a result in some range, a
buffer cleared, a lock released. A stated postcondition catches a wrong
implementation and tells callers what they can rely on. Guarantees of the
form "this resource is released" are best enforced with RAII.

```cpp nocompile
#include <mutex>
std::mutex m;
int counter = 0;
void increment() {
    m.lock();
    ++counter;            // BAD: nothing says m must be unlocked on exit
}                         // and it isn't
```

```cpp
#include <mutex>
std::mutex m;
int counter = 0;
void increment() {        // postcondition: m is unlocked on exit
    std::scoped_lock lock{m};   // GOOD: RAII makes it true on every path
    ++counter;
}
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.8: Prefer `Ensures()` for expressing postconditions

Like I.6, for the result: GSL's `Ensures(cond)` at the end of the body, or
C++26's `post(r: cond)` on the declaration, marks a guarantee as a
guarantee, where tools and readers can find it.

```cpp nocompile
int clamp_percent(int v) {
    int r = v < 0 ? 0 : v > 100 ? 100 : v;
    return r;                  // BAD: "0..100" lives only in the author's head
}
```

```cpp needs=contracts
int clamp_percent(int v)
    post(r: r >= 0 && r <= 100)   // GOOD: C++26, stated on the declaration
{
    return v < 0 ? 0 : v > 100 ? 100 : v;
}
```

## I.9: If an interface is a template, document its parameters using concepts

A constrained template says what it needs from its arguments, and a wrong
argument is rejected at the call with a short message. Unconstrained, the
error appears deep in the body, and a check such as a `requires` expression wrongly
report the call as valid.

```cpp compile-fail="no match for|invalid operands"
#include <vector>
template <class T>
T total(const std::vector<T>& v) {     // BAD: needs +, but doesn't say so
    T sum{};
    for (const T& x : v) sum = sum + x;
    return sum;
}
struct Name { };
Name n = total(std::vector<Name>{});   // error inside total(), not at the call
```

```cpp
#include <concepts>
#include <vector>
template <class T>
concept Addable = std::regular<T> && requires(T a, T b) { { a + b } -> std::convertible_to<T>; };

template <Addable T>                   // GOOD: the requirement is in the interface
T total(const std::vector<T>& v) {
    T sum{};
    for (const T& x : v) sum = sum + x;
    return sum;
}
int n = total(std::vector<int>{1, 2, 3});
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.10: Use exceptions to signal a failure to perform a required task

When a function can't do what it promised, throw. An error code can be
ignored and the program carries on with a bad value; an exception can't be
ignored. Outcomes the caller should always check (a lookup that may find
nothing) are better returned as `std::optional` or `std::expected`.

```cpp nocompile
int parse_port(const char* s);        // BAD: returns -1 on failure
void caller() {
    int port = parse_port("http");    // nobody checks
    use(port);                        // carries on with -1
}
```

```cpp
#include <stdexcept>
#include <string>
int parse_port(const std::string& s) {        // GOOD: throws when it can't
    std::size_t used = 0;
    int port = std::stoi(s, &used);           // throws std::invalid_argument for "http"
    if (used != s.size() || port < 1 || port > 65535)
        throw std::out_of_range("not a port: " + s);
    return port;
}
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.11: Never transfer ownership by a raw pointer (`T*`) or reference (`T&`)

A returned `T*` doesn't say whether the caller must delete it, so it either
leaks or is deleted twice. Return by value (moves make it cheap), or return a
`std::unique_ptr` when you need a pointer. Where an old ABI forces a raw
owning pointer, the GSL's `gsl::owner<T*>` at least marks it.

```cpp nocompile
#include <vector>
std::vector<double>* load_samples() {      // BAD: is the caller supposed to delete it?
    return new std::vector<double>(1000);
}
```

```cpp
#include <memory>
#include <vector>
std::vector<double> load_samples() {       // GOOD: a value; the vector moves out
    return std::vector<double>(1000);
}
struct Plugin { virtual ~Plugin() = default; };
std::unique_ptr<Plugin> load_plugin() {    // GOOD: ownership in the type
    return std::make_unique<Plugin>();
}
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.12: Declare a pointer that must not be null as `not_null`

If null isn't allowed, say so in the type. The GSL's `gsl::not_null<T*>`
does that for pointers. In standard C++, take a reference (which can't be
null) or a type that rejects null, such as `std::string_view`, whose
constructor from `nullptr` is deleted since C++23.

```cpp nocompile
#include <cstddef>
std::size_t length(const char* s) {     // BAD: is length(nullptr) allowed?
    std::size_t n = 0;
    while (s[n]) ++n;                   // undefined behavior if it is null
    return n;
}
```

```cpp
#include <cstddef>
#include <string_view>
std::size_t length(std::string_view s) { return s.size(); }   // GOOD: never null
struct Engine { int rpm = 0; };
void start(Engine& e) { e.rpm = 800; }                         // GOOD: always an object
```

Passing null to those parameters doesn't compile:

```cpp compile-fail="deleted|delete|invalid initialization|no matching function|cannot bind|no viable"
#include <cstddef>
#include <string_view>
std::size_t length(std::string_view s) { return s.size(); }
std::size_t n = length(nullptr);        // error: string_view(nullptr_t) is deleted
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.13: Do not pass an array as a single pointer

A pointer plus a count relies on the caller getting the count right, and a
derived-class array silently converts to a base pointer, so indexing walks
with the wrong element size. A `std::span` keeps the count, and it refuses
that conversion.

```cpp nocompile
struct Shape { virtual ~Shape() = default; int id = 0; };
struct Circle : Shape { double r = 1; };
void draw(Shape* shapes, int n) {
    for (int i = 0; i < n; ++i) use(shapes[i].id);   // BAD: steps sizeof(Shape) through Circles
}
Circle circles[3];
void caller() { draw(circles, 3); }                  // compiles: undefined behavior
```

```cpp
#include <span>
struct Shape { virtual ~Shape() = default; int id = 0; };
struct Circle : Shape { double r = 1; };
void draw(std::span<const Circle> circles) {          // GOOD: the count and element type travel together
    for (const Circle& c : circles) use(c.id);
}
Circle circles[3];
void caller() { draw(circles); }                      // size deduced
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.22: Avoid complex initialization of global objects

Globals in different source files are initialized in an unspecified order, so
a global whose initializer reads another file's global may read it before it's
built (the "static initialization order fiasco"). Initialize globals with
constants (`constexpr`, `constinit`), or build them on first use.

```cpp nocompile
// config.cpp
#include <string>
std::string read_config();
std::string config_path = read_config();       // dynamic initialization

// logger.cpp
extern std::string config_path;
std::string log_file = config_path + ".log";   // BAD: config_path may not be built yet
```

```cpp
#include <string>
#include <string_view>
constexpr std::string_view config_path = "/etc/app.conf";   // GOOD: a constant
constinit int retry_limit = 3;                              // GOOD: no run-time initialization
std::string read_config();
const std::string& config() {                               // GOOD: built on first use
    static const std::string c = read_config();
    return c;
}
```

`constinit` (C++20) makes the compiler reject an initializer that would run at
start-up:

```cpp compile-fail="constant initializer|constant expression|constinit"
int compute();
constinit int retry_limit = compute();   // error: not a constant initializer
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.23: Keep the number of function arguments low

A long parameter list is easy to call wrong and usually means an abstraction
is missing or the function does too much. Group parameters that belong
together into a type (a point, a style, a range); fewer than four parameters
is a good target.

```cpp nocompile
#include <cstdint>
void draw_text(int x, int y, int size, bool bold, bool italic,
               std::uint32_t color, const char* text);    // BAD: seven positional arguments
void caller() { draw_text(10, 20, 12, false, true, 0xff0000, "hi"); }
```

```cpp
#include <cstdint>
#include <string_view>
struct Point { int x = 0; int y = 0; };
struct TextStyle { int size = 12; bool bold = false; bool italic = false; std::uint32_t color = 0; };
void draw_text(Point at, const TextStyle& style, std::string_view text);   // GOOD: three
void caller() { draw_text({10, 20}, {.size = 12, .italic = true, .color = 0xff0000}, "hi"); }
```

## I.24: Avoid adjacent parameters that can be invoked by the same arguments in either order with different meaning

Two neighbouring parameters of the same type can be swapped without a
compiler error. Make them different types: `const` on the source, a named
type per role, or a struct whose fields are named at the call.

```cpp nocompile
#include <cstddef>
void copy_bytes(char* from, char* to, std::size_t n);   // BAD: swap from and to, it still compiles
```

```cpp
#include <span>
void copy_bytes(std::span<const char> from, std::span<char> to);   // GOOD: the source is const
struct Size { int width; int height; };
void resize(Size s);                                  // called as resize({.width = 4, .height = 3})
```

Swapping the arguments is now an error, because a `const` source can't
become the writable destination:

```cpp compile-fail="no matching function|could not convert|no viable conversion|cannot convert"
#include <span>
void copy_bytes(std::span<const char> from, std::span<char> to);
void caller(std::span<const char> in, std::span<char> out) {
    copy_bytes(out, in);                              // error: in is const
}
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.25: Prefer empty abstract classes as interfaces to class hierarchies

An interface base with no data (only pure virtual functions and a virtual
destructor) is stable: changing an implementation never changes it, and no
derived class is forced to carry members it doesn't need.

```cpp nocompile
#include <string>
#include <vector>
struct Point { int x, y; };
class Shape {                          // BAD: an interface loaded with data
public:
    virtual void draw() const;
    Point center() const { return c_; }
protected:
    Point c_;                          // every shape must store a center
    std::vector<Point> outline_;       // and an outline, whether it has one or not
    std::string color_;
};
```

```cpp
struct Point { int x, y; };
class Shape {                          // GOOD: a pure interface
public:
    virtual Point center() const = 0;
    virtual void draw() const = 0;
    virtual ~Shape() = default;
};
class Circle : public Shape {
    Point c_;
    int r_;
public:
    Circle(Point c, int r) : c_{c}, r_{r} {}
    Point center() const override { return c_; }
    void draw() const override { use(r_); }
};
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.26: If you want a cross-compiler ABI, use a C-style subset

Different compilers (and standard libraries) lay out classes, mangle names
and throw exceptions differently, so a library used across toolchains should
export `extern "C"` functions over plain data: built-in types, pointers and
standard-layout structs. Inside, it can use all of C++.

```cpp nocompile
#include <string>
#include <vector>
// BAD as a binary interface: std::string and std::vector differ between
// libstdc++, libc++ and the MS STL, and exceptions don't cross compilers.
__declspec(dllexport) std::vector<std::string> split(const std::string& text);
```

```cpp
#include <cstddef>
extern "C" {
struct split_result {                  // GOOD: plain, standard-layout data
    std::size_t count;
    int error;                         // an error code instead of an exception
};
split_result lib_split(const char* text, std::size_t length,
                       const char** out_parts, std::size_t capacity);
}
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.27: For stable library ABI, consider the Pimpl idiom

Private members are part of a class's layout, so changing them forces every
user to recompile, and a library's ABI breaks. Moving them behind a pointer to
an implementation class ("Pimpl") keeps the public class's size and layout
fixed, at the cost of an allocation and an indirection.

```cpp nocompile
// parser.h
#include <map>
#include <string>
class Parser {
public:
    int parse(const std::string& text);
private:
    std::map<std::string, int> symbols_;   // BAD for a stable ABI: change this and
    int depth_ = 0;                        // every user must be rebuilt
};
```

```cpp
// parser.h
#include <memory>
#include <string>
class Parser {
public:
    Parser();
    ~Parser();                             // defined where Impl is complete
    Parser(Parser&&) noexcept;
    Parser& operator=(Parser&&) noexcept;
    int parse(const std::string& text);
private:
    struct Impl;                           // GOOD: the layout is one pointer
    std::unique_ptr<Impl> impl_;
};

// parser.cpp
#include <map>
struct Parser::Impl {
    std::map<std::string, int> symbols;
    int depth = 0;
};
Parser::Parser() : impl_{std::make_unique<Impl>()} {}
Parser::~Parser() = default;
Parser::Parser(Parser&&) noexcept = default;
Parser& Parser::operator=(Parser&&) noexcept = default;
int Parser::parse(const std::string& text) { return impl_->symbols[text]++; }
```

The destructor and moves are declared in the header but defaulted in the
`.cpp`, where `Impl` is complete; defaulting them in the header fails to
compile, because `unique_ptr` can't delete an incomplete type.

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).

## I.30: Encapsulate rule violations

Sometimes an unsafe technique is needed: here, an input stream that is
sometimes owned and sometimes borrowed. Hide it inside one class with a
simple interface, so the tricky ownership logic is written and reviewed once
instead of at every call site.

```cpp nocompile
#include <fstream>
#include <iostream>
void run(const char* path) {
    bool owned = path != nullptr;
    std::istream* in = owned ? new std::ifstream{path} : &std::cin;   // BAD: at the call site
    use(*in);
    if (owned) delete in;                                             // every caller must remember
}
```

```cpp
#include <fstream>
#include <iostream>
#include <memory>
class Input {                            // GOOD: the special case lives here only
    std::unique_ptr<std::istream> owned_;
    std::istream* in_;
public:
    Input() : in_{&std::cin} {}          // borrows std::cin
    explicit Input(const char* path)
        : owned_{std::make_unique<std::ifstream>(path)}, in_{owned_.get()} {}
    std::istream& get() { return *in_; }
};
void run(const char* path) {
    Input in = path ? Input{path} : Input{};
    use(in.get());
}
```

Tested by [`tests/guidelines/interfaces_test.cpp`](../../../../tests/guidelines/interfaces_test.cpp).
