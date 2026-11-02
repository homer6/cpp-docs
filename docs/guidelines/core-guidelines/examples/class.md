# C: Classes and class hierarchies — examples

> Our examples for the [C rules](../class.md): what a class should say about
> its invariant, its default operations, its place in a hierarchy, its
> operators and its unions, with the compiler or a test checking the claim.

## C.1: Organize related data into structures (`struct`s or `class`es)

Values that belong together should travel together. A `struct` names the
group, so a function can't mix up which number goes with which.

```cpp nocompile
void draw_line(int x1, int y1, int x2, int y2);   // BAD: four loose ints; nothing ties x1 to y1
void demo() { draw_line(0, 10, 0, 20); }          // was that (0,10)→(0,20) or (0,0)→(10,20)? It compiles either way
```

```cpp
struct Point { int x; int y; };
void draw_line(Point from, Point to);             // GOOD: the grouping is in the type
void demo() { draw_line({0, 10}, {0, 20}); }
```

## C.2: Use `class` if the class has an invariant; use `struct` if the data members can vary independently

`class` tells the reader "there's a rule these members must keep, and only my
member functions may change them". `struct` says every combination of values
is fine.

```cpp nocompile
struct Date_range {        // BAD: first <= last must hold, but anyone can break it
    int first;
    int last;
};
void shrink(Date_range& r) { r.last = r.first - 1; }   // compiles; the range is now invalid
```

```cpp
#include <stdexcept>
class Date_range {         // GOOD: a class, because first <= last is an invariant
public:
    Date_range(int first, int last) : first_{first}, last_{last} {
        if (first > last) throw std::invalid_argument{"first > last"};
    }
    int first() const { return first_; }
    int last() const { return last_; }
private:
    int first_;
    int last_;
};
struct Rgb { unsigned char r, g, b; };   // GOOD: a struct: any mix of values is a colour
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.3: Represent the distinction between an interface and an implementation using a class

The public part of a class is what users may rely on; the private part is
what the author may change. When users reach into the representation, it can
never change again.

```cpp nocompile
struct Temperature {
    double kelvin;         // BAD: every user reads and writes the representation
};
double to_celsius(const Temperature& t) { return t.kelvin - 273.15; }   // tied to "it's kelvin" forever
```

```cpp
class Temperature {
public:                    // GOOD: the interface
    static Temperature from_celsius(double c) { return Temperature{c + 273.15}; }
    double celsius() const { return kelvin_ - 273.15; }
private:                   // the implementation, free to change
    explicit Temperature(double k) : kelvin_{k} {}
    double kelvin_;
};
```

## C.4: Make a function a member only if it needs direct access to the representation of a class

A member function can touch the private data, so every member is something to
re-check when the representation changes. A function that only needs the
public interface is better as a non-member.

```cpp nocompile
class Account {
public:
    long balance() const { return cents_; }
    bool is_overdrawn() const { return balance() < 0; }   // BAD: a member, but it only uses balance()
    void deposit(long c) { cents_ += c; }
private:
    long cents_ = 0;
};
```

```cpp
class Account {
public:
    long balance() const { return cents_; }
    void deposit(long c) { cents_ += c; }
private:
    long cents_ = 0;
};
bool is_overdrawn(const Account& a) { return a.balance() < 0; }   // GOOD: a non-member helper
```

## C.5: Place helper functions in the same namespace as the class they support

A helper next to its class is visibly part of its interface, and
argument-dependent lookup finds it when it's called unqualified from anywhere.

```cpp compile-fail="not declared|undeclared identifier"
namespace shop { struct Price { long cents; }; }
namespace util { long dollars(shop::Price p) { return p.cents / 100; } }   // BAD: the helper lives elsewhere
namespace app {
long total(shop::Price p) { return dollars(p); }   // error: lookup searches shop (and app), never util
}
```

```cpp
namespace shop {
struct Price { long cents; };
long dollars(Price p) { return p.cents / 100; }   // GOOD: next to Price
}
namespace app {
long total(shop::Price p) { return dollars(p); }   // found by argument-dependent lookup
}
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.7: Don't define a class or enum and declare a variable of its type in the same statement

A type definition that also declares a variable is easy to misread, and a
forgotten `;` after a `}` can silently turn the next name into a variable.

```cpp nocompile
struct Config { int retries; } default_config;      // BAD: a type and a variable in one statement
enum class Mode { fast, safe } mode = Mode::safe;   // BAD: the same with an enum
```

```cpp
struct Config { int retries; };   // GOOD: the type...
Config default_config{};          // ...then the variable
enum class Mode { fast, safe };
Mode mode = Mode::safe;
```

## C.8: Use `class` rather than `struct` if any member is non-public

`struct` reads as "plain data, all public". If something is hidden, say so
with `class` and put the public interface first.

```cpp nocompile
struct Counter {           // BAD: says "struct" but hides its data
    void increment() { ++n_; }
    int value() const { return n_; }
private:
    int n_ = 0;
};
```

```cpp
class Counter {            // GOOD
public:
    void increment() { ++n_; }
    int value() const { return n_; }
private:
    int n_ = 0;
};
```

## C.9: Minimize exposure of members

Every public data member is a promise to every user, and code anywhere can
break the object's rules through it. Keep data private and offer operations.

```cpp nocompile
#include <vector>
class Buffer {
public:
    std::vector<char> data;
    std::size_t used = 0;  // BAD: public, so nothing stops used > data.size()
    void append(char c) { data.push_back(c); ++used; }
};
```

```cpp
#include <vector>
class Buffer {
public:
    void append(char c) { data_.push_back(c); }
    std::size_t size() const { return data_.size(); }
private:
    std::vector<char> data_;   // GOOD: only Buffer can change it
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.10: Prefer concrete types over class hierarchies

A plain value type lives on the stack or inside other objects, needs no
allocation or indirection, and is easy to reason about. Use a hierarchy only
when you need run-time polymorphism.

```cpp nocompile
#include <memory>
struct Shape_base { virtual ~Shape_base() = default; virtual double area() const = 0; };
struct Square : Shape_base {   // BAD: a hierarchy with one type in it: heap, pointer, vtable
    double side;
    explicit Square(double s) : side{s} {}
    double area() const override { return side * side; }
};
std::unique_ptr<Shape_base> make_square(double s) { return std::make_unique<Square>(s); }
```

```cpp
struct Square {                // GOOD: a concrete type, used by value
    double side;
    double area() const { return side * side; }
};
double total_area() { Square a{2}, b{3}; return a.area() + b.area(); }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.11: Make concrete types regular

A regular type behaves like `int`: it can be default-constructed, copied, and
compared, and a copy compares equal to the original. Without `==`, a copyable
type can't be checked or searched for.

```cpp compile-fail="no match for|invalid operands"
struct Point { int x; int y; };                   // BAD: copyable, but no ==
bool same(Point a, Point b) { return a == b; }    // error: no operator==
```

```cpp
#include <concepts>
struct Point {
    int x = 0;
    int y = 0;
    friend bool operator==(const Point&, const Point&) = default;   // GOOD
};
static_assert(std::regular<Point>);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.12: Don't make data members `const` or references in a copyable or movable type

A `const` or reference member deletes the copy and move assignment, so the
type copy-constructs but won't assign, which breaks sorting, containers and
plain `a = b`.

```cpp compile-fail="deleted"
#include <string>
struct Employee {
    const std::string id;    // BAD: const member
    int level;
};
void replace(Employee& e, const Employee& other) { e = other; }   // error: copy assignment is deleted
```

```cpp
#include <string>
class Employee {
public:
    Employee(std::string id, int level) : id_{std::move(id)}, level_{level} {}
    const std::string& id() const { return id_; }   // GOOD: read-only through the interface
    int level() const { return level_; }
private:
    std::string id_;         // not const, so Employee stays assignable
    int level_;
};
void replace(Employee& e, const Employee& other) { e = other; }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.13: If data member `B` uses another data member `A`, declare `A` before `B`

Members are constructed in declaration order and destroyed in reverse, no
matter how the constructor's initializer list is written. A member that uses
another must come after it.

```cpp nocompile
#include <vector>
class Samples {
    std::size_t count_;          // BAD: declared first, so built before data_ exists
    std::vector<int> data_;
public:
    explicit Samples(std::vector<int> v)
        : count_{data_.size()}, data_{std::move(v)} {}   // UB: reads data_ before it's constructed
};
```

```cpp
#include <vector>
class Samples {
    std::vector<int> data_;      // GOOD: declared before the member that uses it
    std::size_t count_;
public:
    explicit Samples(std::vector<int> v)
        : data_{std::move(v)}, count_{data_.size()} {}
    std::size_t count() const { return count_; }
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.20: If you can avoid defining default operations, do

When every member manages itself (`std::string`, `std::vector`, smart
pointers), the compiler's copy, move and destructor are right and fast. This
is the rule of zero.

```cpp nocompile
#include <map>
#include <string>
class Phone_book {
public:
    Phone_book() = default;
    Phone_book(const Phone_book& o) : entries_{o.entries_} {}   // BAD: what the compiler writes anyway,
    Phone_book& operator=(const Phone_book& o) { entries_ = o.entries_; return *this; }
    ~Phone_book() {}                                            // and now there are no moves: they copy
private:
    std::map<std::string, std::string> entries_;
};
```

```cpp
#include <map>
#include <string>
class Phone_book {                          // GOOD: no special members written
public:
    void add(std::string name, std::string number) { entries_[std::move(name)] = std::move(number); }
private:
    std::map<std::string, std::string> entries_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.21: If you define or `=delete` any copy, move, or destructor function, define or `=delete` them all

The five are linked: declaring a destructor or a copy operation stops the
compiler from declaring the moves, and declaring a move deletes the copies.
Write all five (as `= default` or `= delete` where that's what you mean), so
none of them change by accident.

```cpp compile-fail="deleted"
#include <vector>
class Samples {
public:
    Samples() = default;
    Samples(Samples&&) noexcept = default;   // BAD: only the move constructor declared...
private:
    std::vector<double> values_;
};
Samples copy_of(const Samples& s) { return s; }   // error: ...so the copy constructor is deleted
```

```cpp
#include <vector>
class Samples {
public:
    Samples() = default;
    Samples(const Samples&) = default;              // GOOD: all five stated
    Samples& operator=(const Samples&) = default;
    Samples(Samples&&) noexcept = default;
    Samples& operator=(Samples&&) noexcept = default;
    ~Samples() = default;
private:
    std::vector<double> values_;
};
Samples copy_of(const Samples& s) { return s; }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.22: Make default operations consistent

Copy construction and copy assignment must do the same thing, and both must
agree with what the constructor acquires and the destructor releases.

```cpp nocompile
#include <string>
class Text {
public:
    explicit Text(const char* s) : p_{new std::string{s}} {}
    Text(const Text& o) : p_{new std::string{*o.p_}} {}         // deep copy...
    Text& operator=(const Text& o) { p_ = o.p_; return *this; }  // BAD: ...but a shallow assignment: leak, then double delete
    ~Text() { delete p_; }
private:
    std::string* p_;
};
```

```cpp
#include <memory>
#include <string>
class Text {
public:
    explicit Text(const char* s) : p_{std::make_unique<std::string>(s)} {}
    Text(const Text& o) : p_{std::make_unique<std::string>(*o.p_)} {}   // deep copy
    Text& operator=(const Text& o) {                                     // GOOD: deep as well
        Text tmp{o};
        p_.swap(tmp.p_);
        return *this;
    }
    Text(Text&&) noexcept = default;
    Text& operator=(Text&&) noexcept = default;
    ~Text() = default;
    const std::string& str() const { return *p_; }
private:
    std::unique_ptr<std::string> p_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.30: Define a destructor if a class needs an explicit action at object destruction

Write a destructor only when something must happen that the members' own
destructors don't do. An empty one adds nothing, and declaring it costs the
class its implicit moves.

```cpp nocompile
#include <string>
class Name {
public:
    ~Name() {}             // BAD: does nothing the string doesn't, and suppresses the moves
private:
    std::string value_;
};
```

```cpp
#include <functional>
#include <string>
#include <utility>
class On_exit {            // GOOD: its whole job is an action at destruction
public:
    explicit On_exit(std::function<void()> f) : f_{std::move(f)} {}
    On_exit(const On_exit&) = delete;
    On_exit& operator=(const On_exit&) = delete;
    On_exit(On_exit&&) = delete;
    On_exit& operator=(On_exit&&) = delete;
    ~On_exit() { f_(); }
private:
    std::function<void()> f_;
};
struct Name { std::string value; };   // GOOD: no destructor needed
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.31: All resources acquired by a class must be released by the class's destructor

Whatever the constructor (or any member function) acquires, the destructor
releases, including when the object is destroyed by an exception. Holding the
resource in a member that cleans up after itself does it for you.

```cpp nocompile
#include <cstdio>
class Log {
public:
    explicit Log(const char* path) : f_{std::fopen(path, "a")} {}
    void write(const char* s) { std::fputs(s, f_); }
private:
    std::FILE* f_;         // BAD: no destructor, so the file is never closed
};
```

```cpp
#include <cstdio>
#include <memory>
struct File_closer { void operator()(std::FILE* f) const noexcept { std::fclose(f); } };
class Log {
public:
    explicit Log(const char* path) : f_{std::fopen(path, "a")} {}
    void write(const char* s) { if (f_) std::fputs(s, f_.get()); }
private:
    std::unique_ptr<std::FILE, File_closer> f_;   // GOOD: closed when the Log is destroyed
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.32: If a class has a raw pointer (`T*`) or reference (`T&`), consider whether it might be owning

A raw pointer member doesn't say whether the class must delete what it points
to. Decide, then make the answer visible in the type.

```cpp nocompile
struct Node;
class Tree_cursor {
    Node* node_;           // BAD: owner or observer? A reader can't tell, so a leak or a double delete waits
};
```

```cpp
#include <memory>
struct Node { std::unique_ptr<Node> child; Node* parent = nullptr; };
// GOOD: unique_ptr owns the child; the raw pointer only observes the parent
```

## C.33: If a class has an owning pointer member, define a destructor

An owned object must be deleted when its owner dies. With a raw owning
pointer that means a destructor (and then the other four operations, C.21);
a smart pointer does all of it for you.

```cpp nocompile
#include <cstddef>
class Image {
public:
    explicit Image(std::size_t n) : pixels_{new unsigned char[n]} {}
private:
    unsigned char* pixels_;   // BAD: owns the array, but there's no destructor: leaked
};
```

```cpp
#include <cstddef>
#include <memory>
class Image {
public:
    explicit Image(std::size_t n) : pixels_{new unsigned char[n]} {}
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) = delete;
    Image& operator=(Image&&) = delete;
    ~Image() { delete[] pixels_; }               // GOOD: the owner releases it
private:
    unsigned char* pixels_;
};
class Image2 {
    std::unique_ptr<unsigned char[]> pixels_;    // GOOD, simpler: no destructor to write at all
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.35: A base class destructor should be either public and virtual, or protected and non-virtual

Deleting a derived object through a base pointer whose destructor isn't
virtual is undefined behaviour. Either allow it (public and virtual) or forbid
it (protected, so only derived classes can call it).

```cpp nocompile
#include <memory>
#include <string>
struct Sensor {
    virtual double read() const = 0;
    ~Sensor() = default;               // BAD: public and non-virtual
};
struct Thermometer : Sensor {
    std::string unit = "C";
    double read() const override { return 21.5; }
};
void demo() { std::unique_ptr<Sensor> s = std::make_unique<Thermometer>(); }   // UB: deleted through Sensor*
```

```cpp
struct Sensor {                        // GOOD: owned through base pointers, so public and virtual
    virtual double read() const = 0;
    virtual ~Sensor() = default;
};
class Counted {                        // GOOD: a base nobody deletes through, so protected and non-virtual
public:
    int count() const { return n_; }
protected:
    Counted() = default;
    ~Counted() = default;
    void bump() { ++n_; }
private:
    int n_ = 0;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.36: A destructor must not fail

Destructors run during stack unwinding and inside containers that can't cope
with a failure. A destructor that throws while another exception is in
flight calls `std::terminate`. Handle or report the failure and finish.

```cpp nocompile
#include <stdexcept>
class Connection {
public:
    ~Connection() noexcept(false) {
        if (!close()) throw std::runtime_error{"close failed"};   // BAD: during unwinding this is std::terminate
    }
private:
    bool close() { return false; }
};
```

```cpp
class Connection {
public:
    ~Connection() {
        if (!close()) report("close failed");   // GOOD: record the problem, don't throw
    }
private:
    bool close() noexcept { return false; }
    static void report(const char*) noexcept {}
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.37: Make destructors `noexcept`

A destructor is implicitly `noexcept` only while every member's and base's
destructor is. One member with a `noexcept(false)` destructor silently
changes the whole class. Saying `noexcept` keeps the promise explicit.

```cpp nocompile
struct Legacy { ~Legacy() noexcept(false) {} };
class Session {            // BAD: Session's destructor is now noexcept(false) too, without a word
public:
    ~Session() {}
private:
    Legacy legacy_;
};
```

```cpp
struct Legacy { ~Legacy() noexcept(false) {} };
class Session {
public:
    ~Session() noexcept {}   // GOOD: stated; if Legacy ever throws here, the program terminates
private:
    Legacy legacy_;
};
static_assert(std::is_nothrow_destructible_v<Session>);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.40: Define a constructor if a class has an invariant

The constructor is where an invariant is established. Without one, any
initializer the user writes is accepted.

```cpp nocompile
struct Fraction {
    int num;
    int den;               // BAD: must not be 0, but nothing checks
};
Fraction broken{1, 0};     // compiles
```

```cpp
#include <stdexcept>
class Fraction {
public:
    Fraction(int num, int den) : num_{num}, den_{den} {   // GOOD: the invariant holds from the start
        if (den == 0) throw std::invalid_argument{"zero denominator"};
    }
    double value() const { return static_cast<double>(num_) / den_; }
private:
    int num_;
    int den_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.41: A constructor should create a fully initialized object

When the constructor returns, the object must be ready to use. A separate
`init()` that callers must remember is a bug waiting to happen.

```cpp nocompile
#include <vector>
class Histogram {
public:
    Histogram() = default;                             // BAD: not usable yet...
    void init(int bins) { counts_.assign(bins, 0); }   // ...until someone remembers init()
    void add(int bin) { ++counts_[bin]; }              // UB if init() wasn't called
private:
    std::vector<int> counts_;
};
```

```cpp
#include <vector>
class Histogram {
public:
    explicit Histogram(int bins) : counts_(bins, 0) {}   // GOOD: ready when constructed
    void add(int bin) { ++counts_.at(bin); }
    int count(int bin) const { return counts_.at(bin); }
private:
    std::vector<int> counts_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.42: If a constructor cannot construct a valid object, throw an exception

A constructor has no return value to report failure with. Leaving behind a
half-made object with an `is_valid()` flag makes every user check it, and one
of them won't.

```cpp nocompile
class Port {
public:
    explicit Port(int n) : n_{n}, valid_{n > 0 && n < 65536} {}   // BAD: builds an invalid Port
    bool is_valid() const { return valid_; }                       // and trusts every user to ask
    int number() const { return n_; }
private:
    int n_;
    bool valid_;
};
```

```cpp
#include <stdexcept>
class Port {
public:
    explicit Port(int n) : n_{n} {
        if (n <= 0 || n >= 65536) throw std::out_of_range{"port"};   // GOOD: no invalid Port exists
    }
    int number() const { return n_; }
private:
    int n_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.43: Ensure that a copyable class has a default constructor

Arrays, `std::vector<T>(n)`, `resize` and many algorithms need `T{}`. A
copyable value type should have a sensible default value, which makes it
semiregular.

```cpp compile-fail="no matching"
struct Date {
    Date(int y, int m, int d) : y_{y}, m_{m}, d_{d} {}   // BAD: copyable, but no default constructor
    int y_, m_, d_;
};
Date week[7];   // error: no default constructor
```

```cpp
#include <concepts>
struct Date {
    Date() = default;                                    // GOOD: 1970-01-01 by default
    Date(int y, int m, int d) : y_{y}, m_{m}, d_{d} {}
    int y_ = 1970, m_ = 1, d_ = 1;
};
Date week[7];
static_assert(std::semiregular<Date>);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.44: Prefer default constructors to be simple and non-throwing

A default constructor that can't fail makes "reset to empty" safe after an
error and keeps moved-from states cheap. Allocating in it costs time and
memory for every empty object.

```cpp nocompile
#include <memory>
class Int_buffer {
public:
    Int_buffer() : data_{std::make_unique<int[]>(64)} {}   // BAD: an empty buffer allocates, so it can throw
private:
    std::unique_ptr<int[]> data_;
    std::size_t size_ = 0;
};
```

```cpp
#include <memory>
class Int_buffer {
public:
    Int_buffer() noexcept = default;   // GOOD: empty means a null pointer; nothing can fail
private:
    std::unique_ptr<int[]> data_;
    std::size_t size_ = 0;
};
static_assert(std::is_nothrow_default_constructible_v<Int_buffer>);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.45: Don't define a default constructor that only initializes data members; use default member initializers instead

Put the defaults on the members and let the compiler generate the default
constructor; it's shorter and the compiler's version can be more efficient
(and `noexcept`).

```cpp nocompile
class Retry_policy {
public:
    Retry_policy() : attempts_{3}, delay_ms_{100} {}   // BAD: a constructor only to set constants
private:
    int attempts_;
    int delay_ms_;
};
```

```cpp
class Retry_policy {
    int attempts_ = 3;       // GOOD: defaults where the members are declared
    int delay_ms_ = 100;
};
static_assert(std::is_nothrow_default_constructible_v<Retry_policy>);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.46: By default, declare single-argument constructors explicit

A non-`explicit` one-argument constructor is also an implicit conversion, so
a wrong argument type quietly becomes an object. Make it `explicit` unless the
conversion is really wanted.

```cpp nocompile
#include <cstddef>
class Buffer_size {
public:
    Buffer_size(std::size_t bytes) : bytes_{bytes} {}   // BAD: implicit conversion
private:
    std::size_t bytes_;
};
void reserve(Buffer_size);
void demo() { reserve(-1); }   // compiles: -1 silently becomes a huge Buffer_size
```

```cpp
#include <cstddef>
class Buffer_size {
public:
    explicit Buffer_size(std::size_t bytes) : bytes_{bytes} {}   // GOOD
private:
    std::size_t bytes_;
};
void reserve(Buffer_size);
void demo() { reserve(Buffer_size{4096}); }   // the conversion is visible
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.47: Define and initialize data members in the order of member declaration

Members are initialized in declaration order, whatever order the initializer
list uses. Writing the list in a different order misleads the reader, and an
initializer that reads a "later" member reads garbage.

```cpp nocompile
class Rect {
    int area_;
    int width_;
    int height_;
public:
    Rect(int w, int h) : width_{w}, height_{h}, area_{width_ * height_} {}   // BAD: area_ is initialized first,
};                                                                          // from uninitialized width_ and height_
```

```cpp
class Rect {
    int width_;
    int height_;
    int area_;
public:
    Rect(int w, int h) : width_{w}, height_{h}, area_{width_ * height_} {}   // GOOD: same order as declared
    int area() const { return area_; }
};
```

GCC and Clang warn about the BAD order with `-Wreorder` (in `-Wall`), but it
isn't an error, so the block is tagged `nocompile` (wrong but compiles).

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.48: Prefer default member initializers to member initializers in constructors for constant initializers

When several constructors share a constant initial value, write it once on
the member. Then a new constructor can't forget it, and the constructors
only mention what differs.

```cpp nocompile
class Connection_opts {
public:
    Connection_opts() : timeout_ms_{500}, verbose_{false} {}
    explicit Connection_opts(bool verbose) : verbose_{verbose} {}   // BAD: forgot timeout_ms_: uninitialized
private:
    int timeout_ms_;
    bool verbose_;
};
```

```cpp
class Connection_opts {
public:
    Connection_opts() = default;
    explicit Connection_opts(bool verbose) : verbose_{verbose} {}   // GOOD: timeout_ms_ is still 500
    int timeout_ms() const { return timeout_ms_; }
private:
    int timeout_ms_ = 500;
    bool verbose_ = false;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.49: Prefer initialization to assignment in constructors

Assigning in the constructor body first default-constructs each member and
then overwrites it, and a member is unset until its line runs. Initialize in
the member initializer list.

```cpp nocompile
#include <string>
class Person {
public:
    Person(const std::string& name, int age) {
        name_ = name;      // BAD: name_ was default-constructed, now it's assigned
        age_ = age;        // and age_ held garbage until here
    }
private:
    std::string name_;
    int age_;
};
```

```cpp
#include <string>
#include <utility>
class Person {
public:
    Person(std::string name, int age) : name_{std::move(name)}, age_{age} {}   // GOOD: built once
private:
    std::string name_;
    int age_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.50: Use a factory function if you need "virtual behavior" during initialization

During a base constructor the derived part doesn't exist yet, so a virtual
call can't reach it. If setup needs the derived behaviour, construct the
object first, then finish it in a factory that's the only way to create one.

```cpp nocompile
#include <string>
void register_name(const std::string&);
class Plugin {
public:
    Plugin() { register_name(describe()); }   // BAD: always calls Plugin::describe; a pure one would be UB
    virtual ~Plugin() = default;
    virtual std::string describe() const { return "plugin"; }
};
class Spell_check : public Plugin {
public:
    std::string describe() const override { return "spell-check"; }   // never seen by the constructor
};
```

```cpp
#include <memory>
#include <string>
class Plugin {
public:
    template <class T>
    static std::unique_ptr<T> create() {          // GOOD: the only way to make one
        auto p = std::make_unique<T>(Token{});
        p->post_init();                           // the object is complete: the call reaches T
        return p;
    }
    virtual ~Plugin() = default;
    virtual std::string describe() const = 0;
    const std::string& name() const { return name_; }
protected:
    struct Token {};                              // only Plugin can make a Token
    explicit Plugin(Token) {}
private:
    void post_init() { name_ = describe(); }
    std::string name_;
};
class Spell_check : public Plugin {
public:
    explicit Spell_check(Token t) : Plugin{t} {}
    std::string describe() const override { return "spell-check"; }
};
auto checker = Plugin::create<Spell_check>();
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.51: Use delegating constructors to represent common actions for all constructors of a class

When constructors share work (a check, a computation), have the others
delegate to one that does it. Copies drift apart; a delegation can't.

```cpp nocompile
#include <stdexcept>
class Color {
public:
    Color(int r, int g, int b) : r_{r}, g_{g}, b_{b} {
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) throw std::out_of_range{"rgb"};
    }
    explicit Color(int grey) : r_{grey}, g_{grey}, b_{grey} {}   // BAD: a copy of the first, minus the check
private:
    int r_, g_, b_;
};
```

```cpp
#include <stdexcept>
class Color {
public:
    Color(int r, int g, int b) : r_{r}, g_{g}, b_{b} {
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) throw std::out_of_range{"rgb"};
    }
    explicit Color(int grey) : Color{grey, grey, grey} {}   // GOOD: the check runs here too
private:
    int r_, g_, b_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.52: Use inheriting constructors to import constructors into a derived class that does not need further explicit initialization

When a derived class adds behaviour but no data that needs initializing, a
`using Base::Base;` gives it every base constructor. Re-writing them by hand
is tedious and easy to get subtly wrong.

```cpp nocompile
#include <cstddef>
#include <initializer_list>
#include <vector>
class Checked_vector : public std::vector<int> {
public:
    Checked_vector() = default;
    explicit Checked_vector(std::size_t n) : std::vector<int>(n) {}               // BAD: base constructors
    Checked_vector(std::initializer_list<int> il) : std::vector<int>(il) {}       // copied by hand, and some
    int& checked(std::size_t i) { return at(i); }                                 // (count + value, ranges) forgotten
};
```

```cpp
#include <cstddef>
#include <vector>
class Checked_vector : public std::vector<int> {
public:
    using std::vector<int>::vector;               // GOOD: all of them, exactly as the base has them
    int& checked(std::size_t i) { return at(i); }
};
Checked_vector v(3, 7);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.60: Make copy assignment non-`virtual`, take the parameter by `const&`, and return by non-`const&`

`T& operator=(const T&)` is the shape everyone and every library expects: it
reads the source without copying it, and returns the target so that
assignments chain. A `virtual` assignment invites mixing types.

```cpp nocompile
#include <vector>
class Matrix {
public:
    virtual const Matrix& operator=(Matrix other);   // BAD: virtual, copies its argument, returns const&
private:
    std::vector<double> cells_;
};
```

```cpp
#include <vector>
class Matrix {
public:
    Matrix& operator=(const Matrix& other) {          // GOOD
        cells_ = other.cells_;
        return *this;
    }
private:
    std::vector<double> cells_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.61: A copy operation should copy

After `x = y` (or `T x{y}`), `x == y`. With value semantics the two are then
independent: changing one doesn't change the other.

```cpp nocompile
#include <memory>
#include <vector>
class Polygon {
public:
    Polygon() : pts_{std::make_shared<std::vector<int>>()} {}
    Polygon(const Polygon& o) : pts_{o.pts_} {}   // BAD: a "copy" that shares the points
    void add(int p) { pts_->push_back(p); }       // adding to the copy changes the original too
private:
    std::shared_ptr<std::vector<int>> pts_;
};
```

```cpp
#include <vector>
class Polygon {
public:
    void add(int p) { pts_.push_back(p); }
    friend bool operator==(const Polygon&, const Polygon&) = default;
private:
    std::vector<int> pts_;   // GOOD: copying a Polygon copies its points
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.62: Make copy assignment safe for self-assignment

`x = x` must leave `x` unchanged. It happens through aliases (`a[i] = a[j]`
with `i == j`), so a hand-written assignment that frees first and copies
second destroys its own source.

```cpp nocompile
#include <algorithm>
#include <cstddef>
class Blob {
public:
    Blob& operator=(const Blob& o) {
        delete[] data_;                                   // BAD: if &o == this, o.data_ is gone too
        data_ = new char[o.size_];
        std::copy(o.data_, o.data_ + o.size_, data_);     // reads freed memory
        size_ = o.size_;
        return *this;
    }
private:
    char* data_ = nullptr;
    std::size_t size_ = 0;
};
```

```cpp
#include <vector>
class Blob {
public:
    explicit Blob(std::vector<char> bytes) : data_{std::move(bytes)} {}
    // GOOD: the compiler's assignment copies the vector, which handles x = x
private:
    std::vector<char> data_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.63: Make move assignment non-`virtual`, take the parameter by `&&`, and return by non-`const&`

The same shape as copy assignment, for rvalues: `T& operator=(T&&)`, not
virtual, returning `*this`.

```cpp nocompile
#include <vector>
class Buffer {
public:
    virtual const Buffer& operator=(Buffer&& other);   // BAD: virtual, and returns const&
private:
    std::vector<char> bytes_;
};
```

```cpp
#include <utility>
#include <vector>
class Buffer {
public:
    Buffer() = default;
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&& other) noexcept {   // GOOD
        bytes_ = std::move(other.bytes_);
        return *this;
    }
private:
    std::vector<char> bytes_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.64: A move operation should move and leave its source in a valid state

After `y = std::move(x)`, `y` has `x`'s old value and `x` can still be
destroyed, assigned to and used. Ideally it's the default (empty) value.

```cpp nocompile
#include <cstddef>
#include <string>
class Name_list {
public:
    Name_list(Name_list&& o) noexcept : data_{o.data_}, size_{o.size_} {}   // BAD: o still owns the same array
    ~Name_list() { delete[] data_; }                                      // so it's deleted twice
private:
    std::string* data_ = nullptr;
    std::size_t size_ = 0;
};
```

```cpp
#include <cstddef>
#include <string>
#include <utility>
class Name_list {
public:
    Name_list() = default;
    Name_list(Name_list&& o) noexcept
        : data_{std::exchange(o.data_, nullptr)}, size_{std::exchange(o.size_, 0)} {}   // GOOD: o is now empty
    Name_list& operator=(Name_list&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = std::exchange(o.data_, nullptr);
            size_ = std::exchange(o.size_, 0);
        }
        return *this;
    }
    Name_list(const Name_list&) = delete;
    Name_list& operator=(const Name_list&) = delete;
    ~Name_list() { delete[] data_; }
    std::size_t size() const { return size_; }
private:
    std::string* data_ = nullptr;
    std::size_t size_ = 0;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.65: Make move assignment safe for self-assignment

Nobody writes `x = std::move(x)` on purpose, but `std::swap(a, b)` with `a`
and `b` the same object does it. A move assignment that releases first and
steals second loses the value.

```cpp nocompile
class Owner {
public:
    Owner& operator=(Owner&& o) noexcept {
        delete p_;          // BAD: on self-move, this deletes the value we meant to keep
        p_ = o.p_;          // now a dangling pointer...
        o.p_ = nullptr;     // ...and then null: x = std::move(x) loses x
        return *this;
    }
private:
    int* p_ = nullptr;
};
```

```cpp
class Owner {
public:
    Owner& operator=(Owner&& o) noexcept {
        if (this != &o) {   // GOOD: self-move leaves the object as it was
            delete p_;
            p_ = o.p_;
            o.p_ = nullptr;
        }
        return *this;
    }
private:
    int* p_ = nullptr;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.66: Make move operations `noexcept`

`std::vector` moves its elements when it grows only if their move constructor
can't throw; otherwise it copies them to keep its strong guarantee. A move
that isn't `noexcept` quietly turns into a copy.

```cpp nocompile
#include <string>
class Track {
public:
    explicit Track(std::string t) : title_{std::move(t)} {}
    Track(const Track&) = default;
    Track(Track&& o) : title_{std::move(o.title_)} {}   // BAD: not noexcept, so vector<Track> copies when it grows
private:
    std::string title_;
};
```

```cpp
#include <string>
class Track {
public:
    explicit Track(std::string t) : title_{std::move(t)} {}
    Track(const Track&) = default;
    Track(Track&& o) noexcept : title_{std::move(o.title_)} {}   // GOOD
private:
    std::string title_;
};
static_assert(std::is_nothrow_move_constructible_v<Track>);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.67: A polymorphic class should suppress public copy/move

Copying through a base class copies only the base part (slicing), and the
copy then behaves like the base. Delete the copy and move operations of a
polymorphic base (make them protected if the base has data), and use `clone`
(C.130) when deep copies are needed.

```cpp nocompile
struct Shape {
    virtual ~Shape() = default;
    virtual double area() const { return 0; }
};                                                 // BAD: public copy
struct Circle : Shape {
    double r = 1;
    double area() const override { return 3.14159 * r * r; }
};
double area_of(Shape s) { return s.area(); }      // compiles, slices: always 0
```

```cpp
class Shape {
public:
    Shape() = default;
    Shape(const Shape&) = delete;                  // GOOD: no data, so delete them
    Shape& operator=(const Shape&) = delete;
    Shape(Shape&&) = delete;
    Shape& operator=(Shape&&) = delete;
    virtual ~Shape() = default;
    virtual double area() const = 0;
};
double area_of(const Shape& s) { return s.area(); }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.80: Use `=default` if you have to be explicit about using the default semantics

When you must declare a special member (here the destructor is virtual) but
want its usual meaning, `= default` says so and keeps what the compiler's
version gives you, such as triviality and `noexcept`.

```cpp nocompile
struct Pixel {
    Pixel() {}
    Pixel(const Pixel& o) : r{o.r}, g{o.g}, b{o.b} {}   // BAD: hand-written defaults: verbose,
    Pixel& operator=(const Pixel& o) { r = o.r; g = o.g; b = o.b; return *this; }   // easy to get wrong,
    unsigned char r = 0, g = 0, b = 0;                   // and no longer trivially copyable
};
```

```cpp
struct Pixel {
    Pixel() = default;
    Pixel(const Pixel&) = default;                       // GOOD
    Pixel& operator=(const Pixel&) = default;
    unsigned char r = 0, g = 0, b = 0;
};
static_assert(std::is_trivially_copyable_v<Pixel>);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.81: Use `=delete` when you want to disable default behavior (without wanting an alternative)

`= delete` removes an operation for everyone, with a clear diagnostic at the
call. The old trick (private and never defined) still lets members and
friends compile a copy, and fails only when linking.

```cpp nocompile
#include <mutex>
class Lock_guard {
    Lock_guard(const Lock_guard&);              // BAD: private and undefined: a member that copies
    Lock_guard& operator=(const Lock_guard&);   // compiles and fails only at link time
public:
    explicit Lock_guard(std::mutex& m);
};
```

```cpp
#include <mutex>
class Lock_guard {
public:
    explicit Lock_guard(std::mutex& m) : m_{m} { m_.lock(); }
    Lock_guard(const Lock_guard&) = delete;              // GOOD: public and deleted
    Lock_guard& operator=(const Lock_guard&) = delete;
    ~Lock_guard() { m_.unlock(); }
private:
    std::mutex& m_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.82: Don't call virtual functions in constructors and destructors

Inside a constructor or destructor the object is only the class being built
or torn down, so a virtual call runs that class's version, not the derived
one. Calling a pure virtual function that way is undefined behaviour.

```cpp nocompile
class Logger {
public:
    Logger() { open(); }               // BAD: runs Logger::open even when building a File_logger
    virtual ~Logger() { close(); }     // and Logger::close here; if they were pure, UB
    virtual void open() {}
    virtual void close() {}
};
class File_logger : public Logger {
public:
    void open() override {}            // never called by Logger's constructor
};
```

```cpp
class Logger {
public:
    Logger() { open_common(); }        // GOOD: a non-virtual helper: what runs is what you read
    virtual ~Logger() = default;
    virtual void write(const char*) {}
private:
    void open_common() {}
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.83: For value-like types, consider providing a `noexcept` swap function

A cheap, non-throwing `swap` is the basis of copy-and-swap assignment and of
commit-or-rollback code. Give it as a member and as a non-member in the
type's namespace, so `using std::swap; swap(a, b);` finds it.

```cpp nocompile
#include <vector>
class Grid {
public:
    void swap(Grid& o) { cells_.swap(o.cells_); }   // BAD: not noexcept, and no non-member swap:
private:                                         // generic code calling swap(a, b) never sees it
    std::vector<int> cells_;
};
```

```cpp
#include <vector>
class Grid {
public:
    void swap(Grid& o) noexcept { cells_.swap(o.cells_); }          // GOOD
    friend void swap(Grid& a, Grid& b) noexcept { a.swap(b); }     // found by swap(a, b)
private:
    std::vector<int> cells_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.84: A `swap` function must not fail

Algorithms and containers assume `swap` always succeeds. A swap that copies
can run out of memory halfway; swap the handles instead.

```cpp nocompile
#include <vector>
class Table {
public:
    void swap(Table& o) {
        std::vector<int> tmp = rows_;   // BAD: copies, so it can throw std::bad_alloc
        rows_ = o.rows_;
        o.rows_ = tmp;
    }
private:
    std::vector<int> rows_;
};
```

```cpp
#include <vector>
class Table {
public:
    void swap(Table& o) noexcept { rows_.swap(o.rows_); }   // GOOD: exchanges pointers, can't fail
private:
    std::vector<int> rows_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.85: Make `swap` `noexcept`

Since a swap must not fail, say so: `noexcept` lets `std::is_nothrow_swappable`
and the code that checks it rely on it, and terminates if it ever does throw.

```cpp nocompile
#include <string>
struct Tag {
    std::string text;
    friend void swap(Tag& a, Tag& b) { a.text.swap(b.text); }   // BAD: can't fail, but doesn't say so
};
```

```cpp
#include <string>
#include <type_traits>
struct Tag {
    std::string text;
    friend void swap(Tag& a, Tag& b) noexcept { a.text.swap(b.text); }   // GOOD
};
static_assert(std::is_nothrow_swappable_v<Tag>);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.86: Make `==` symmetric with respect to operand types and `noexcept`

`a == b` should work exactly when `b == a` does, and never throw. A member
`operator==` treats its left operand differently (here it can't be `const`);
a non-member (or hidden friend) treats both alike.

```cpp compile-fail="discards qualifiers|invalid operands|no match"
#include <string>
struct Name {
    std::string value;
    bool operator==(const Name& o) { return value == o.value; }   // BAD: member, non-const left side
};
bool same(const Name& a, const Name& b) { return a == b; }   // error: a const left operand isn't accepted
```

```cpp
#include <string>
struct Name {
    std::string value;
    friend bool operator==(const Name& a, const Name& b) noexcept { return a.value == b.value; }   // GOOD
};
bool same(const Name& a, const Name& b) { return a == b; }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.87: Beware of `==` on base classes

An `==` defined on a base compares only the base part, so two different
derived objects can compare equal, and a virtual `==` can't know the other
operand's type. Compare where the whole value is known.

```cpp nocompile
#include <string>
struct Animal {
    std::string name;
    virtual ~Animal() = default;
    virtual bool operator==(const Animal& o) const { return name == o.name; }   // BAD
};
struct Dog : Animal { std::string breed; };
// Dog{"Rex", "collie"} == Dog{"Rex", "poodle"} is true: only the Animal part is compared
```

```cpp
#include <string>
struct Animal {                       // GOOD: no == on the base
    virtual ~Animal() = default;
    virtual std::string name() const = 0;
};
struct Dog final : Animal {
    std::string given_name, breed;
    std::string name() const override { return given_name; }
    friend bool operator==(const Dog& a, const Dog& b) noexcept {   // == where the full value is known
        return a.given_name == b.given_name && a.breed == b.breed;
    }
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.89: Make a `hash` `noexcept`

Unordered containers call the hash on every lookup and don't expect it to
fail; the standard requires it not to throw. Combine the members' standard
hashes and mark it `noexcept`.

```cpp nocompile
#include <functional>
#include <stdexcept>
#include <string>
struct User_id { std::string value; };
namespace std {
template <> struct hash<User_id> {
    std::size_t operator()(const User_id& u) const {   // BAD: throws, and isn't noexcept
        if (u.value.empty()) throw std::invalid_argument{"empty id"};
        return std::hash<std::string>{}(u.value);
    }
};
}
```

```cpp
#include <functional>
#include <string>
struct User_id { std::string value; };
namespace std {
template <> struct hash<User_id> {
    std::size_t operator()(const User_id& u) const noexcept {   // GOOD
        return std::hash<std::string>{}(u.value);
    }
};
}
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.90: Rely on constructors and assignment operators, not `memset` and `memcpy`

Constructors and assignment keep a type's invariants. `memset` and `memcpy`
on a type that isn't trivially copyable are undefined behaviour: they
overwrite pointers, sizes, and vtable pointers behind the type's back.

```cpp nocompile
#include <cstring>
#include <string>
struct Record { std::string name; int score = 0; };
void reset(Record& r) { std::memset(&r, 0, sizeof r); }                              // BAD: wipes the string's internals
void clone(Record& to, const Record& from) { std::memcpy(&to, &from, sizeof to); }   // BAD: two strings, one buffer
```

```cpp
#include <string>
struct Record { std::string name; int score = 0; };
void reset(Record& r) { r = Record{}; }                         // GOOD
void clone(Record& to, const Record& from) { to = from; }       // GOOD
```

GCC warns about the BAD calls (`-Wclass-memaccess`), but they compile, so the
block is `nocompile` (wrong but compiles).

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.100: Follow the STL when defining a container

Use the standard names (`begin`, `end`, `size`, `empty`, `value_type`,
`iterator`…) with their usual meaning, and range-`for`, the algorithms and
the ranges library work with your container for free.

```cpp compile-fail="begin"
#include <vector>
class Bag {                       // BAD: its own names instead of the STL's
public:
    int* first() { return data_.data(); }
    int* past_last() { return data_.data() + data_.size(); }
    int count() const { return static_cast<int>(data_.size()); }
private:
    std::vector<int> data_;
};
int sum(Bag& b) { int s = 0; for (int x : b) s += x; return s; }   // error: no begin()/end()
```

```cpp
#include <cstddef>
#include <vector>
class Bag {
public:
    using value_type = int;                        // GOOD: the names everyone knows
    using iterator = std::vector<int>::iterator;
    using const_iterator = std::vector<int>::const_iterator;
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    std::size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    void push_back(int x) { data_.push_back(x); }
private:
    std::vector<int> data_;
};
int sum(const Bag& b) { int s = 0; for (int x : b) s += x; return s; }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.101: Give a container value semantics

Copying a container should copy its elements: the copy compares equal and is
independent, as with `std::vector`.

```cpp nocompile
#include <cstddef>
class Int_list {
public:
    explicit Int_list(std::size_t n) : data_{new int[n]{}}, size_{n} {}
    ~Int_list() { delete[] data_; }
    // BAD: the implicit copy copies the pointer: two lists share one array, and both delete it
private:
    int* data_;
    std::size_t size_;
};
```

```cpp
#include <cstddef>
#include <vector>
class Int_list {
public:
    explicit Int_list(std::size_t n) : data_(n) {}
    int& operator[](std::size_t i) { return data_[i]; }
    friend bool operator==(const Int_list&, const Int_list&) = default;
private:
    std::vector<int> data_;   // GOOD: a copy copies the elements and compares equal
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.102: Give a container move operations

Moving a container should hand over its buffer, not copy the elements, so
returning one from a function is cheap. A user-declared copy constructor with
no move constructor makes every move a copy.

```cpp nocompile
#include <vector>
class Series {
public:
    Series() = default;
    Series(const Series& o) : data_{o.data_} {}   // BAD: declared copy, no move: returning a Series copies it
private:
    std::vector<double> data_;
};
```

```cpp
#include <vector>
class Series {
public:
    Series() = default;                // GOOD: the implicit moves hand over the vector's buffer
    void add(double x) { data_.push_back(x); }
private:
    std::vector<double> data_;
};
static_assert(std::is_nothrow_move_constructible_v<Series>);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.103: Give a container an initializer list constructor

People expect to write `Bag b{1, 2, 3};` for any container, as they do for
the standard ones.

```cpp compile-fail="no matching"
#include <vector>
class Bag {                           // BAD: no initializer_list constructor
public:
    Bag() = default;
    void push_back(int x) { data_.push_back(x); }
private:
    std::vector<int> data_;
};
Bag primes{2, 3, 5, 7};   // error
```

```cpp
#include <initializer_list>
#include <vector>
class Bag {
public:
    Bag() = default;
    Bag(std::initializer_list<int> il) : data_{il} {}   // GOOD
private:
    std::vector<int> data_;
};
Bag primes{2, 3, 5, 7};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.104: Give a container a default constructor that sets it to empty

A default-constructed container is empty, as `std::vector{}` is. That makes
the type regular and usable in arrays and as a member without setup.

```cpp compile-fail="no matching"
#include <initializer_list>
#include <vector>
class Bag {                           // BAD: no default constructor
public:
    Bag(std::initializer_list<int> il) : data_{il} {}
private:
    std::vector<int> data_;
};
Bag empty_bag;   // error
```

```cpp
#include <initializer_list>
#include <vector>
class Bag {
public:
    Bag() noexcept = default;         // GOOD: an empty Bag
    Bag(std::initializer_list<int> il) : data_{il} {}
    bool empty() const { return data_.empty(); }
private:
    std::vector<int> data_;
};
Bag empty_bag;
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.109: If a resource handle has pointer semantics, provide `*` and `->`

A handle that refers to a shared object should be used like a pointer:
`*h` and `h->member`, as with `std::unique_ptr` and `std::shared_ptr`.

```cpp compile-fail="non-pointer type|not a pointer"
#include <memory>
#include <string>
class Doc_handle {                    // BAD: shares a document, but has no * or ->
public:
    explicit Doc_handle(std::shared_ptr<std::string> p) : p_{std::move(p)} {}
    std::string& get() { return *p_; }
private:
    std::shared_ptr<std::string> p_;
};
std::size_t length(Doc_handle h) { return h->size(); }   // error
```

```cpp
#include <memory>
#include <string>
class Doc_handle {
public:
    explicit Doc_handle(std::shared_ptr<std::string> p) : p_{std::move(p)} {}
    std::string& operator*() const { return *p_; }      // GOOD
    std::string* operator->() const { return p_.get(); }
private:
    std::shared_ptr<std::string> p_;
};
std::size_t length(const Doc_handle& h) { return h->size(); }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.120: Use class hierarchies to represent concepts with inherent hierarchical structure (only)

Inherit only when every derived type really is a kind of the base and can do
everything the base promises. Forcing unrelated things under one base leaves
functions that can't be implemented and throw instead.

```cpp nocompile
#include <stdexcept>
struct Container {                    // BAD: not every container can do all of this
    virtual ~Container() = default;
    virtual int& operator[](int i) = 0;
    virtual void push_front(int x) = 0;
};
struct Stack : Container {
    int& operator[](int) override { throw std::logic_error{"no indexing"}; }      // can't, so it throws
    void push_front(int) override { throw std::logic_error{"no push_front"}; }
};
```

```cpp
struct Shape {                        // GOOD: every shape really has an area and can be drawn
    virtual ~Shape() = default;
    virtual double area() const = 0;
    virtual void draw() const = 0;
};
struct Circle : Shape {
    double r = 1;
    double area() const override { return 3.14159 * r * r; }
    void draw() const override {}
};
```

## C.121: If a base class is used as an interface, make it a pure abstract class

An interface should be only public pure virtual functions and a virtual
destructor: no data and no implementation that ties the users to a
representation.

```cpp nocompile
#include <string>
class Printer {                       // BAD: an interface with data and a non-virtual destructor
public:
    virtual void print(const std::string& s) = 0;
protected:
    std::string name_;
};
class Laser : public Printer {
    std::string queue_;
public:
    void print(const std::string& s) override { queue_ += s; }
};
void demo() { Printer* p = new Laser; delete p; }   // UB: Laser's members are never destroyed
```

```cpp
#include <string>
class Printer {                       // GOOD: pure abstract
public:
    virtual void print(const std::string& s) = 0;
    virtual ~Printer() = default;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.122: Use abstract classes as interfaces when complete separation of interface and implementation is needed

When users must not depend on any implementation detail (across a library or
ABI boundary, or to swap implementations), give them an abstract class and a
factory; the concrete classes can then change without recompiling users.

```cpp nocompile
#include <string>
class Usb_storage {                   // BAD: users include and depend on one concrete class:
public:                               // its size, members and layout are part of their build
    void write(const std::string& data);
private:
    int handle_ = 0;
    char buffer_[512];
};
void save(Usb_storage& dev, const std::string& data) { dev.write(data); }
```

```cpp
#include <memory>
#include <string>
class Storage {                       // GOOD: users see only this
public:
    virtual void write(const std::string& data) = 0;
    virtual ~Storage() = default;
};
std::unique_ptr<Storage> open_usb();  // implemented elsewhere; can change freely
void save(Storage& dev, const std::string& data) { dev.write(data); }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.126: An abstract class typically doesn't need a user-written constructor

An abstract class usually has no data, so there's nothing for a constructor
to set up. Leave it to the compiler.

```cpp nocompile
class Codec {
public:
    Codec() {}                         // BAD: nothing to initialize
    virtual ~Codec() = default;
    virtual int encode(int sample) const = 0;
};
```

```cpp
class Codec {                          // GOOD: no constructor written
public:
    virtual ~Codec() = default;
    virtual int encode(int sample) const = 0;
};
```

## C.127: A class with a virtual function should have a virtual or protected destructor

A class with virtual functions is meant to be used through base pointers, and
eventually something deletes through one. With a public non-virtual
destructor that's undefined behaviour.

```cpp nocompile
#include <vector>
class Task {
public:
    virtual void run() {}
    ~Task() = default;                 // BAD: virtual functions, public non-virtual destructor
};
class Batch : public Task {
    std::vector<int> items_;
public:
    void run() override {}
};
void demo() { Task* t = new Batch; delete t; }   // UB: ~Batch never runs
```

```cpp
class Task {
public:
    virtual void run() {}
    virtual ~Task() = default;         // GOOD
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.128: Virtual functions should specify exactly one of `virtual`, `override`, or `final`

`virtual` introduces a new virtual function, `override` says "this overrides
one", `final` says "and nothing may override me". With `override`, a
signature that drifts from the base is an error instead of a silent new
function.

```cpp nocompile
struct Shape {
    virtual void scale(double factor);
    virtual void draw() const;
};
struct Circle : Shape {
    virtual void scale(float factor);                  // BAD: a new function that hides Shape::scale; no error
    virtual void draw() const override;                // BAD: two keywords; override alone says it all
};
```

```cpp
struct Shape {
    virtual void scale(double factor);                 // GOOD: virtual only on the new function
    virtual void draw() const;
    virtual ~Shape() = default;
};
struct Circle : Shape {
    void scale(double factor) override;                // GOOD: override only
    void draw() const final;                           // GOOD: final only
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.129: When designing a class hierarchy, distinguish between implementation inheritance and interface inheritance

Inheriting an interface separates users from implementations; inheriting an
implementation shares code. Mixing both in one base makes every change to
the shared code or data ripple to every user.

```cpp nocompile
class Shape {                          // BAD: interface and implementation in one base
public:
    Shape(double x, double y) : x_{x}, y_{y} {}
    virtual ~Shape() = default;
    virtual double area() const = 0;
    void move(double dx, double dy) { x_ += dx; y_ += dy; }   // every user now depends on x_ and y_
protected:
    double x_, y_;
};
```

```cpp
class Shape {                          // GOOD: the interface users see
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
    virtual void move(double dx, double dy) = 0;
};
class Positioned {                     // GOOD: shared implementation, a separate base
public:
    void move_by(double dx, double dy) { x_ += dx; y_ += dy; }
private:
    double x_ = 0, y_ = 0;
};
class Circle : public Shape, private Positioned {
public:
    double area() const override { return 3.14159 * r_ * r_; }
    void move(double dx, double dy) override { move_by(dx, dy); }
private:
    double r_ = 1;
};
```

## C.130: For making deep copies of polymorphic classes prefer a virtual `clone` function instead of public copy construction/assignment

Copying through a base slices. A virtual `clone` copies the most-derived
object and returns an owner of the copy.

```cpp nocompile
#include <memory>
struct Shape {
    virtual ~Shape() = default;
    virtual double area() const { return 0; }
};
struct Square : Shape {
    double side = 2;
    double area() const override { return side * side; }
};
std::unique_ptr<Shape> copy(const Shape& s) { return std::make_unique<Shape>(s); }   // BAD: copies only the Shape part
```

```cpp
#include <memory>
class Shape {
public:
    Shape() = default;
    Shape(const Shape&) = delete;
    Shape& operator=(const Shape&) = delete;
    virtual ~Shape() = default;
    virtual std::unique_ptr<Shape> clone() const = 0;   // GOOD
    virtual double area() const = 0;
};
class Square final : public Shape {
public:
    explicit Square(double side) : side_{side} {}
    std::unique_ptr<Shape> clone() const override { return std::make_unique<Square>(side_); }
    double area() const override { return side_ * side_; }
private:
    double side_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.131: Avoid trivial getters and setters

A getter and setter that just read and write a member add nothing over
public data. If any value is allowed, use a `struct`; if not, the functions
should enforce the rule.

```cpp nocompile
class Point {
public:
    int x() const { return x_; }
    void set_x(int x) { x_ = x; }      // BAD: ceremony around a member anyone may change anyway
    int y() const { return y_; }
    void set_y(int y) { y_ = y; }
private:
    int x_ = 0;
    int y_ = 0;
};
```

```cpp
struct Point {                         // GOOD: any x and y are valid, so just say so
    int x = 0;
    int y = 0;
};
```

## C.132: Don't make a function `virtual` without reason

A virtual function costs a vtable pointer in every object and an indirect
call, and it invites overriding. A class that isn't meant as a base doesn't
need any.

```cpp nocompile
class Vec2 {
public:
    virtual double length() const;            // BAD: nobody derives from Vec2; every
    virtual Vec2 operator+(const Vec2&) const;  // object now carries a vtable pointer
private:
    double x_ = 0, y_ = 0;
};
```

```cpp
class Vec2 {
public:
    double length() const;                    // GOOD: plain member functions
    Vec2 operator+(const Vec2&) const;
private:
    double x_ = 0, y_ = 0;
};
static_assert(!std::is_polymorphic_v<Vec2>);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.133: Avoid `protected` data

Protected data can be changed by any derived class, present or future, so no
one can state or keep its invariant. Keep data private and give derived
classes protected functions instead.

```cpp nocompile
class Shape {
public:
    virtual ~Shape() = default;
protected:
    double width_ = 0;                 // BAD: every derived class may set these
    double height_ = 0;                // to anything, and many will
};
```

```cpp
class Shape {
public:
    virtual ~Shape() = default;
protected:
    void resize(double w, double h) {  // GOOD: derived classes go through a checked function
        if (w >= 0 && h >= 0) { width_ = w; height_ = h; }
    }
    double width() const { return width_; }
private:
    double width_ = 0;
    double height_ = 0;
};
```

## C.134: Ensure all non-`const` data members have the same access level

A class either keeps an invariant (all non-`const` data private) or is a
bundle of independent values (all public). Mixing the two leaves the reader
unsure which members the rules cover.

```cpp nocompile
#include <vector>
class Inventory {
public:
    int reorder_level = 10;            // BAD: public...
    void add(int item) { items_.push_back(item); }
private:
    std::vector<int> items_;           // ...next to private: is reorder_level part of the invariant?
};
```

```cpp
#include <vector>
class Inventory {
public:
    explicit Inventory(int reorder_level) : reorder_level_{reorder_level} {}
    void add(int item) { items_.push_back(item); }
    int reorder_level() const { return reorder_level_; }
private:
    int reorder_level_;                // GOOD: all non-const data private
    std::vector<int> items_;
};
struct Margins { int top = 0, right = 0, bottom = 0, left = 0; };   // GOOD: all public
```

## C.135: Use multiple inheritance to represent multiple distinct interfaces

Split a large interface into the separate roles callers need, and let a class
implement several of them. A caller that only reads takes a `Reader&`.

```cpp nocompile
#include <string>
class Stream {                         // BAD: one interface for everything: a read-only
public:                                // source must still implement write and seek
    virtual ~Stream() = default;
    virtual std::string read() = 0;
    virtual void write(const std::string&) = 0;
    virtual void seek(long) = 0;
};
```

```cpp
#include <string>
class Reader { public: virtual ~Reader() = default; virtual std::string read() = 0; };
class Writer { public: virtual ~Writer() = default; virtual void write(const std::string&) = 0; };
class File : public Reader, public Writer {   // GOOD: implements the two roles it has
public:
    std::string read() override { return data_; }
    void write(const std::string& s) override { data_ += s; }
private:
    std::string data_;
};
std::string slurp(Reader& r) { return r.read(); }   // needs only the reading role
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.136: Use multiple inheritance to represent the union of implementation attributes

Small bases that each bring a piece of state and its operations (mixins) can
be combined in one class, instead of copying the same code into every class
that needs it.

```cpp nocompile
#include <string>
class Session {
public:
    void touch(long now) { last_used_ = now; }   // BAD: the same "last used" code is copied
    long last_used() const { return last_used_; }
private:
    long last_used_ = 0;
    std::string user_;
};
class Upload {
public:
    void touch(long now) { last_used_ = now; }   // ...into every class that needs it
    long last_used() const { return last_used_; }
private:
    long last_used_ = 0;
};
```

```cpp
#include <string>
class Timestamped {
public:
    void touch(long now) { last_used_ = now; }
    long last_used() const { return last_used_; }
protected:
    ~Timestamped() = default;
private:
    long last_used_ = 0;
};
class Named {
public:
    const std::string& name() const { return name_; }
    void rename(std::string n) { name_ = std::move(n); }
protected:
    ~Named() = default;
private:
    std::string name_;
};
class Session : public Timestamped, public Named {};   // GOOD: the union of both attributes
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.137: Use `virtual` bases to avoid overly general base classes

When several branches of a hierarchy share an implementation base, make it a
`virtual` base so there's one copy of it, instead of pushing its data up
into the root of the whole hierarchy.

```cpp compile-fail="ambiguous|multiple base"
struct Utility { int uses = 0; };
struct Reader : Utility { void read() { ++uses; } };
struct Writer : Utility { void write() { ++uses; } };
struct File : Reader, Writer {};      // BAD: two Utility parts, two counters
int uses_of(File& f) { return f.uses; }   // error: which uses?
```

```cpp
struct Utility { int uses = 0; };
struct Reader : virtual Utility { void read() { ++uses; } };   // GOOD: one shared Utility
struct Writer : virtual Utility { void write() { ++uses; } };
struct File : Reader, Writer {};
int uses_of(File& f) { return f.uses; }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.138: Create an overload set for a derived class and its bases with `using`

A function declared in a derived class hides every base function with the
same name, not only the one with the same parameters. `using Base::name;`
brings the base overloads back.

```cpp compile-fail="cannot convert|no viable conversion"
#include <string>
struct Printer {
    void print(int) {}
    void print(const std::string&) {}
};
struct Fancy_printer : Printer {
    void print(int) {}                     // BAD: hides Printer::print(const std::string&)
};
void demo(Fancy_printer& p) { p.print(std::string{"hello"}); }   // error
```

```cpp
#include <string>
struct Printer {
    void print(int) {}
    void print(const std::string&) {}
};
struct Fancy_printer : Printer {
    using Printer::print;                  // GOOD: the base overloads stay visible
    void print(int) {}
};
void demo(Fancy_printer& p) { p.print(std::string{"hello"}); }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.139: Use `final` on classes sparingly

Marking a class in a hierarchy `final` forbids every future extension, and
the speed it's said to buy is rarely measured. Use it only for a real reason.

```cpp compile-fail="final"
struct Shape { virtual ~Shape() = default; virtual double area() const = 0; };
struct Circle final : Shape {              // BAD: capped "for speed", unmeasured
    double r = 1;
    double area() const override { return 3.14159 * r * r; }
};
struct Labelled_circle : Circle {};        // error: a reasonable extension is now impossible
```

```cpp
struct Shape { virtual ~Shape() = default; virtual double area() const = 0; };
struct Circle : Shape {                    // GOOD: left open
    double r = 1;
    double area() const override { return 3.14159 * r * r; }
};
struct Labelled_circle : Circle { const char* label = ""; };
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.140: Do not provide different default arguments for a virtual function and an overrider

Default arguments are chosen from the static type, but the body comes from the
dynamic type. If the base and the overrider disagree, the same call gives a
different argument depending on how you reach it.

```cpp nocompile
struct Logger {
    virtual ~Logger() = default;
    virtual void log(const char* msg, int level = 1);
};
struct Debug_logger : Logger {
    void log(const char* msg, int level = 3) override;   // BAD: called through Logger&, level is 1
};
```

```cpp
struct Logger {
    virtual ~Logger() = default;
    void log(const char* msg) { do_log(msg, 1); }        // GOOD: the default lives in one non-virtual place
    void log(const char* msg, int level) { do_log(msg, level); }
private:
    virtual void do_log(const char* msg, int level) = 0;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.145: Access polymorphic objects through pointers and references

Passing or storing a polymorphic object by value copies only the base part,
and calls then go to the base's functions. Use a reference, a pointer or a
smart pointer.

```cpp nocompile
#include <vector>
struct Shape { virtual ~Shape() = default; virtual double area() const { return 0; } };
struct Square : Shape { double side = 2; double area() const override { return side * side; } };
double area_of(Shape s) { return s.area(); }   // BAD: sliced: always 0
std::vector<Shape> shapes;                     // BAD: every element is just a Shape
```

```cpp
#include <memory>
#include <vector>
struct Shape { virtual ~Shape() = default; virtual double area() const = 0; };
double area_of(const Shape& s) { return s.area(); }   // GOOD
std::vector<std::unique_ptr<Shape>> shapes;           // GOOD
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.146: Use `dynamic_cast` where class hierarchy navigation is unavoidable

`dynamic_cast` checks at run time that the object really is of the target
type. A `static_cast` downcast trusts you, and is undefined behaviour when
you're wrong.

```cpp nocompile
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape { double r = 1; };
struct Square : Shape { double side = 1; };
double radius(Shape& s) { return static_cast<Circle&>(s).r; }   // BAD: UB if s is a Square
```

```cpp
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape { double r = 1; };
double radius_or_zero(Shape& s) {
    if (auto* c = dynamic_cast<Circle*>(&s)) return c->r;      // GOOD: checked
    return 0;
}
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.147: Use `dynamic_cast` to a reference type when failure to find the required class is considered an error

A reference can't be null, so `dynamic_cast<T&>` throws `std::bad_cast` when
the object isn't a `T`. Use it when "not a `T`" is a bug, not a case to
handle.

```cpp nocompile
struct Message { virtual ~Message() = default; };
struct Login : Message { const char* user = ""; };
const char* user_of(Message& m) {
    return dynamic_cast<Login*>(&m)->user;   // BAD: "must be a Login", but a null pointer is dereferenced if not: UB
}
```

```cpp
struct Message { virtual ~Message() = default; };
struct Login : Message { const char* user = ""; };
const char* user_of(Message& m) {
    return dynamic_cast<Login&>(m).user;     // GOOD: throws std::bad_cast if it isn't a Login
}
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.148: Use `dynamic_cast` to a pointer type when failure to find the required class is considered a valid alternative

When "it might not be a `T`" is a normal case, cast to a pointer and test it
for null. An exception isn't the tool for ordinary control flow.

```cpp nocompile
#include <typeinfo>
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape { double r = 1; };
double radius_or_zero(Shape& s) {
    try { return dynamic_cast<Circle&>(s).r; }   // BAD: an exception for an expected "no"
    catch (const std::bad_cast&) { return 0; }
}
```

```cpp
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape { double r = 1; };
double radius_or_zero(Shape& s) {
    if (auto* c = dynamic_cast<Circle*>(&s)) return c->r;   // GOOD: null means "not a Circle"
    return 0;
}
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.149: Use `unique_ptr` or `shared_ptr` to avoid forgetting to `delete` objects created using `new`

A raw `new` needs a matching `delete` on every path, including exceptions and
early returns. A smart pointer deletes on all of them.

```cpp nocompile
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape {};
void draw(Shape&);
void demo() {
    Shape* s = new Circle;   // BAD: if draw throws, or someone adds a return, it leaks
    draw(*s);
    delete s;
}
```

```cpp
#include <memory>
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape {};
void draw(Shape&);
void demo() {
    std::unique_ptr<Shape> s = std::make_unique<Circle>();   // GOOD: deleted on every path
    draw(*s);
}
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.150: Use `make_unique()` to construct objects owned by `unique_ptr`s

`std::make_unique<T>(args)` names the type once and never leaves a naked
`new` in the code.

```cpp nocompile
#include <memory>
#include <string>
std::unique_ptr<std::string> greeting{new std::string{"hello"}};   // BAD: type written twice, a naked new
```

```cpp
#include <memory>
#include <string>
auto greeting = std::make_unique<std::string>("hello");            // GOOD
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.151: Use `make_shared()` to construct objects owned by `shared_ptr`s

`std::make_shared` allocates the object and its reference counts together,
once, instead of in two allocations, and avoids the naked `new`.

```cpp nocompile
#include <memory>
#include <string>
std::shared_ptr<std::string> name{new std::string{"Ada"}};   // BAD: two allocations, a naked new
```

```cpp
#include <memory>
#include <string>
auto name = std::make_shared<std::string>("Ada");            // GOOD: one allocation
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.152: Never assign a pointer to an array of derived class objects to a pointer to its base

An array of `Derived` decays to `Derived*`, which converts to `Base*`
silently. Indexing that pointer steps by `sizeof(Base)`, landing in the
middle of objects.

```cpp nocompile
struct Base { int id = 0; };
struct Derived : Base { int extra = 0; };
int sum_ids(const Base* p, int n) { int s = 0; for (int i = 0; i < n; ++i) s += p[i].id; return s; }
Derived items[4];
int total = sum_ids(items, 4);   // BAD: compiles; p[1] isn't items[1]: UB
```

```cpp
#include <span>
struct Base { int id = 0; };
struct Derived : Base { int extra = 0; };
int sum_ids(std::span<const Derived> items) {   // GOOD: the element type is the real one
    int s = 0;
    for (const auto& d : items) s += d.id;
    return s;
}
Derived items[4];
int total = sum_ids(items);
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.153: Prefer virtual function to casting

A virtual call always reaches the most-derived override. A chain of casts
checks for the types someone thought of, and silently does the wrong thing
for a class added later.

```cpp nocompile
struct Shape { virtual ~Shape() = default; };
struct Circle : Shape { double r = 1; };
struct Square : Shape { double side = 1; };
double area(const Shape& s) {
    if (auto* c = dynamic_cast<const Circle*>(&s)) return 3.14159 * c->r * c->r;   // BAD: a type switch
    if (auto* q = dynamic_cast<const Square*>(&s)) return q->side * q->side;
    return 0;                                                  // a new Triangle silently gets 0
}
```

```cpp
struct Shape { virtual ~Shape() = default; virtual double area() const = 0; };
struct Circle : Shape { double r = 1; double area() const override { return 3.14159 * r * r; } };
struct Square : Shape { double side = 1; double area() const override { return side * side; } };
double area(const Shape& s) { return s.area(); }   // GOOD: each class answers for itself
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.160: Define operators primarily to mimic conventional usage

An operator should do what the symbol means for built-in types: `+` adds and
doesn't change its operands, `==` is equality and a copy compares equal.

```cpp nocompile
struct Money {
    long cents = 0;
    Money operator+(const Money& o) { cents += o.cents; return *this; }   // BAD: + changes its left operand
};
```

```cpp
struct Money {
    long cents = 0;
    friend Money operator+(Money a, Money b) { return Money{a.cents + b.cents}; }   // GOOD
    friend bool operator==(const Money&, const Money&) = default;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.161: Use non-member functions for symmetric operators

A member operator converts only its right operand, so `x + 1` works and
`1 + x` doesn't. A non-member (here a hidden friend) treats both sides alike.

```cpp compile-fail="no match for|invalid operands"
struct Meters {
    Meters(double v) : value{v} {}                 // converts from double on purpose
    Meters operator+(const Meters& o) const { return value + o.value; }   // BAD: member
    double value;
};
Meters a = Meters{2} + 1.5;                         // fine
Meters b = 1.5 + Meters{2};                         // error: no conversion for the left operand
```

```cpp
struct Meters {
    Meters(double v) : value{v} {}
    friend Meters operator+(const Meters& a, const Meters& b) { return a.value + b.value; }   // GOOD
    double value;
};
Meters a = Meters{2} + 1.5;
Meters b = 1.5 + Meters{2};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.162: Overload operations that are roughly equivalent

The same operation on different types should have the same name. Encoding
the type in the name adds noise and stops generic code from calling it.

```cpp nocompile
#include <string>
std::string int_to_text(int);                   // BAD: the type is in the name,
std::string double_to_text(double);             // so a template can't call "to_text" on T
```

```cpp
#include <string>
std::string to_text(int v) { return std::to_string(v); }          // GOOD: one name
std::string to_text(double v) { return std::to_string(v); }
template <class T> std::string label(const T& v) { return "[" + to_text(v) + "]"; }
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.163: Overload only for operations that are roughly equivalent

Two different operations shouldn't share a name: overload resolution then
picks the meaning from the argument's type, and a small change in a call
changes what it does.

```cpp nocompile
#include <algorithm>
#include <cstddef>
#include <vector>
void remove(std::vector<int>& v, int value) { std::erase(v, value); }                         // removes by value
void remove(std::vector<int>& v, std::size_t index) { v.erase(v.begin() + static_cast<std::ptrdiff_t>(index)); }   // BAD: by position
// remove(v, 2) erases every 2; remove(v, i) with a size_t i erases one element
```

```cpp
#include <algorithm>
#include <cstddef>
#include <vector>
void erase_value(std::vector<int>& v, int value) { std::erase(v, value); }   // GOOD: different names
void erase_at(std::vector<int>& v, std::size_t index) { v.erase(v.begin() + static_cast<std::ptrdiff_t>(index)); }
```

## C.164: Avoid implicit conversion operators

An implicit conversion operator fires in places nobody intended, such as
turning a temporary string into a pointer that dangles. Make it `explicit`,
or give it a name.

```cpp nocompile
#include <string>
class Path {
public:
    explicit Path(std::string s) : s_{std::move(s)} {}
    operator const char*() const { return s_.c_str(); }   // BAD: implicit
private:
    std::string s_;
};
Path make_path();
const char* p = make_path();   // compiles; the temporary Path dies here, so p dangles
```

```cpp
#include <string>
class Path {
public:
    explicit Path(std::string s) : s_{std::move(s)} {}
    const char* c_str() const { return s_.c_str(); }       // GOOD: named
    explicit operator bool() const { return !s_.empty(); } // GOOD: explicit; still works in if (path)
private:
    std::string s_;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.165: Use `using` for customization points

To call a customizable function such as `swap`, bring the generic version
into scope with `using std::swap;` and call it unqualified. Then a type's own
`swap` is found by argument-dependent lookup, and the generic one is the
fallback.

```cpp nocompile
#include <utility>
template <class T>
void exchange_values(T& a, T& b) {
    std::swap(a, b);           // BAD: always the generic swap, never the type's own one
}
```

```cpp
#include <utility>
template <class T>
void exchange_values(T& a, T& b) {
    using std::swap;           // GOOD: generic fallback...
    swap(a, b);                // ...but a better swap in T's namespace wins
}
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.166: Overload unary `&` only as part of a system of smart pointers and references

Code everywhere assumes `&x` is the address of `x`. Overloading it breaks
that for every user and every template; generic code then has to use
`std::addressof`.

```cpp nocompile
struct Tracked {
    int value = 0;
    int operator&() const { return value; }   // BAD: &t is no longer an address
};
```

```cpp
#include <memory>
struct Tracked { int value = 0; };            // GOOD: & means what it always means
template <class T> T* where(T& obj) { return std::addressof(obj); }   // generic code: safe even if & is overloaded
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.167: Use an operator for an operation with its conventional meaning

When an operation has a conventional operator (`<<` for output, `==` for
equality, `[]` for indexing), use it: then the type works with the code that
relies on the convention.

```cpp nocompile
#include <iostream>
struct Point { int x, y; };
void print_point(std::ostream& os, const Point& p) { os << p.x << ',' << p.y; }   // BAD: can't be chained or used generically
```

```cpp
#include <ostream>
struct Point { int x, y; };
std::ostream& operator<<(std::ostream& os, const Point& p) { return os << p.x << ',' << p.y; }   // GOOD
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.168: Define overloaded operators in the namespace of their operands

Argument-dependent lookup looks for operators in the operands' namespaces.
An operator defined anywhere else is found only where its namespace happens
to be visible, and can mean different things in different places.

```cpp compile-fail="no match for|invalid operands"
namespace geo { struct Point { int x, y; }; }
namespace ops { bool operator==(geo::Point a, geo::Point b) { return a.x == b.x && a.y == b.y; } }   // BAD
namespace app {
bool same(geo::Point a, geo::Point b) { return a == b; }   // error: ops isn't searched
}
```

```cpp
namespace geo {
struct Point { int x, y; };
bool operator==(Point a, Point b) { return a.x == b.x && a.y == b.y; }   // GOOD: next to Point
}
namespace app {
bool same(geo::Point a, geo::Point b) { return a == b; }   // found by argument-dependent lookup
}
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.170: If you feel like overloading a lambda, use a generic lambda

A lambda is a variable, so two lambdas can't share a name. A generic lambda
(`auto` parameter) gives one name that works for many types.

```cpp compile-fail="conflicting declaration|redefinition"
#include <string>
auto describe = [](int v) { return std::to_string(v); };
auto describe = [](const std::string& s) { return s; };   // BAD: error: a second variable named describe
```

```cpp
#include <string>
#include <type_traits>
auto describe = [](const auto& v) -> std::string {         // GOOD: one generic lambda
    if constexpr (std::is_arithmetic_v<std::remove_cvref_t<decltype(v)>>) return std::to_string(v);
    else return v;
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.180: Use `union`s to save memory

A union lets members that are never used at the same time share storage: it's
as big as its largest member, not the sum. Keep it wrapped with something
that knows which member is live (C.181).

```cpp nocompile
struct Token {
    int kind;              // 0 = number, 1 = symbol
    double number;         // BAD: both always take space, though only one is used
    char symbol;
};
```

```cpp
struct Token {
    enum class Kind { number, symbol } kind;
    union {                // GOOD: number and symbol share storage
        double number;
        char symbol;
    };
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.181: Avoid "naked" `union`s

A union on its own doesn't record which member holds a value, so reading the
wrong one is a silent type error. Use `std::variant`, which tracks it and
checks every access.

```cpp nocompile
union Value {
    int i;
    double d;
};
double read() {
    Value v;
    v.i = 42;
    return v.d;            // BAD: reads a member that isn't active: UB, no warning
}
```

```cpp
#include <variant>
double read() {
    std::variant<int, double> v = 42;   // GOOD: knows it holds an int
    if (auto* d = std::get_if<double>(&v)) return *d;
    return std::get<int>(v);            // std::get<double>(v) would throw std::bad_variant_access
}
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.182: Use anonymous `union`s to implement tagged unions

When you write your own tagged union, put the tag and an anonymous union
inside a class, keep both private, and let the member functions keep them in
step. Users can't then set one without the other.

```cpp nocompile
struct Value {
    enum Kind { integer, real } kind;
    union { int i; double d; } as;    // BAD: public tag and union: nothing keeps them in step
};
void demo() { Value v; v.kind = Value::real; v.as.i = 3; }   // claims real, holds an int
```

```cpp
#include <stdexcept>
class Value {
public:
    explicit Value(int i) : kind_{Kind::integer}, i_{i} {}
    explicit Value(double d) : kind_{Kind::real}, d_{d} {}
    int as_int() const {
        if (kind_ != Kind::integer) throw std::logic_error{"not an int"};
        return i_;
    }
    double as_real() const {
        if (kind_ != Kind::real) throw std::logic_error{"not a real"};
        return d_;
    }
private:
    enum class Kind { integer, real } kind_;
    union { int i_; double d_; };      // GOOD: anonymous and private; only the class touches it
};
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).

## C.183: Don't use a `union` for type punning

Writing one member of a union and reading another is undefined behaviour in
C++, and it's invisible at the read. To see an object's bits, use
`std::bit_cast` (C++20), which is defined and checked.

```cpp nocompile
#include <cstdint>
std::uint32_t bits_of(float f) {
    union { float f; std::uint32_t u; } pun;
    pun.f = f;
    return pun.u;          // BAD: reads the inactive member: UB
}
```

```cpp
#include <bit>
#include <cstdint>
std::uint32_t bits_of(float f) {
    return std::bit_cast<std::uint32_t>(f);   // GOOD: defined, and checks the sizes match
}
```

Tested by [`tests/guidelines/class_test.cpp`](../../../../tests/guidelines/class_test.cpp).
