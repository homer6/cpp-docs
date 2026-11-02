# E: Error handling — examples

> Our examples for the [E rules](../errors.md): report failures with
> exceptions (or, where you can't, one systematic alternative), and let RAII
> clean up on every path.

## E.1: Develop an error-handling strategy early in a design

*No example: a design decision (exceptions, error codes or `std::expected`, and where failures are handled), made before the code; the other E rules show its pieces.*

## E.2: Throw an exception to signal that a function can't perform its assigned task

When a function can't do its job, throwing tells the caller in a way it can't
ignore, and the return type stays the plain result. A special "error" return
value is easy to miss and steals a value from the result's range.

```cpp bad
#include <string>
int port_from(const std::string& text) {
    if (text.empty()) return -1;   // BAD: -1 is easy to forget to check,
    return std::stoi(text);        // and the caller may pass it on as a port
}
```

```cpp
#include <stdexcept>
#include <string>
int port_from(const std::string& text) {
    if (text.empty()) throw std::invalid_argument{"port_from: empty text"};   // GOOD
    return std::stoi(text);
}
```

The BAD version compiles: nothing forces the caller to test for `-1`.

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.3: Use exceptions for error handling only

An exception is for a failure, not for ordinary control flow. Using `throw`
to return a found value is harder to read and much slower than a `return`,
because implementations make the non-throwing path fast.

```cpp bad
#include <cstddef>
#include <vector>
std::ptrdiff_t index_of(const std::vector<int>& v, int x) {
    try {
        for (std::size_t i = 0; i < v.size(); ++i)
            if (v[i] == x) throw i;   // BAD: "found" isn't an error
    } catch (std::size_t i) {
        return static_cast<std::ptrdiff_t>(i);
    }
    return -1;
}
```

```cpp
#include <algorithm>
#include <optional>
#include <vector>
std::optional<std::size_t> index_of(const std::vector<int>& v, int x) {
    auto it = std::ranges::find(v, x);   // GOOD: not finding x is a normal result
    if (it == v.end()) return std::nullopt;
    return static_cast<std::size_t>(it - v.begin());
}
```

The BAD version compiles and even works; it's wrong as a design, so it's
tagged `bad` (compiles, but is wrong).

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.4: Design your error-handling strategy around invariants

Each class states what must hold for its objects (the invariant). Errors are
handled so that every object still alive afterwards still satisfies it: an
operation either finishes or leaves the object as it was.

```cpp bad
#include <string>
#include <vector>
class Roster {
    std::vector<std::string> names_;
    std::size_t count_ = 0;   // invariant: count_ == names_.size()
public:
    void add(const std::string& n) {
        ++count_;                 // BAD: if push_back throws, count_ is wrong
        names_.push_back(n);
    }
};
```

```cpp
#include <string>
#include <vector>
class Roster {
    std::vector<std::string> names_;   // the size is the count: no second copy to keep in step
public:
    void add(const std::string& n) { names_.push_back(n); }   // GOOD: strong guarantee from vector
    std::size_t count() const { return names_.size(); }
};
```

The BAD version compiles; it breaks its invariant only when `push_back`
throws (out of memory, or a throwing copy).

## E.5: Let a constructor establish an invariant, and throw if it cannot

A constructor that can't make a valid object should throw, so no half-built
object ever exists. A separate `init()` call leaves a window where the object
is unusable and every member function has to check.

```cpp bad
class Percent {
    int value_ = 0;
public:
    bool init(int v) {            // BAD: until init succeeds, the object is invalid
        if (v < 0 || v > 100) return false;
        value_ = v;
        return true;
    }
    int value() const { return value_; }
};
```

```cpp
#include <stdexcept>
class Percent {
    int value_;
public:
    explicit Percent(int v) : value_{v} {
        if (v < 0 || v > 100) throw std::out_of_range{"Percent: not in 0..100"};   // GOOD
    }
    int value() const { return value_; }   // can rely on 0 <= value_ <= 100
};
```

The BAD version compiles; a caller who forgets `init()` or ignores its
result uses an object with no invariant.

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.6: Use RAII to prevent leaks

Give every resource an owner whose destructor releases it. The destructor
runs when the scope is left by any route, a `return`, a `throw` here or a
throw from a called function, so nothing leaks and no path needs its own
cleanup.

```cpp bad
#include <cstdio>
void copy_header(const char* path) {
    std::FILE* f = std::fopen(path, "r");
    if (!f) return;
    process(f);          // BAD: if process throws, fclose never runs
    std::fclose(f);
}
```

```cpp
#include <cstdio>
#include <memory>
struct FileCloser { void operator()(std::FILE* f) const noexcept { std::fclose(f); } };
using File = std::unique_ptr<std::FILE, FileCloser>;
void copy_header(const char* path) {
    File f{std::fopen(path, "r")};   // GOOD: closed on every exit path
    if (!f) return;
    process(f.get());
}
```

The BAD version compiles; the leak happens only when `process` throws.

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.7: State your preconditions

Say what a function needs from its arguments, in code where you can, so
callers know what's required and a violation is caught where it happens
instead of corrupting something later. C++26 contracts (`pre`) state it in
the declaration; before them, check it at the top of the function
(`assert`, or throw for a checked interface).

```cpp bad
double average(const std::vector<double>& v) {
    // BAD: nothing says v must not be empty; with an empty v this divides by zero
    double sum = 0;
    for (double x : v) sum += x;
    return sum / static_cast<double>(v.size());
}
```

```cpp needs=contracts
#include <vector>
double average(const std::vector<double>& v)
    pre(!v.empty())   // GOOD: the requirement is part of the declaration
{
    double sum = 0;
    for (double x : v) sum += x;
    return sum / static_cast<double>(v.size());
}
```

The GOOD block uses C++26 contracts (checked with GCC 16; GCC 15 and Clang 22
don't have them, hence `needs=contracts`). The BAD version compiles; it
returns NaN for an empty vector.

## E.8: State your postconditions

Say what a function guarantees about its result, so callers can rely on it
and a broken implementation is caught at the function that broke it. C++26
contracts write it as `post`; before them, check it before returning or in a
test.

```cpp bad
int clamp_percent(int v) {
    // BAD: callers assume 0..100, but nothing says so or checks it
    return v < 0 ? 0 : v > 100 ? 100 : v;
}
```

```cpp needs=contracts
int clamp_percent(int v)
    post(r: r >= 0 && r <= 100)   // GOOD: the promise is in the declaration
{
    return v < 0 ? 0 : v > 100 ? 100 : v;
}
```

C++26 contracts again (checked with GCC 16). The BAD version is correct
today, but the promise lives only in the caller's head.

## E.12: Use `noexcept` when exiting a function because of a `throw` is impossible or unacceptable

Mark a function `noexcept` when it can't throw, or when a throw would be a
bug you'd rather terminate on. Callers and the library use it: `std::vector`
moves its elements on reallocation only when the move constructor is
`noexcept`, and otherwise copies them.

```cpp compile-fail="static assertion failed"
#include <string>
#include <type_traits>
struct Track {
    std::string title;
    Track() = default;
    Track(const Track&) = default;
    Track(Track&& o) : title{std::move(o.title)} {}   // BAD: can't throw, but doesn't say so
};
static_assert(std::is_nothrow_move_constructible_v<Track>);   // so vector<Track> copies on growth
```

```cpp
#include <string>
#include <type_traits>
struct Track {
    std::string title;
    Track() = default;
    Track(const Track&) = default;
    Track(Track&& o) noexcept : title{std::move(o.title)} {}   // GOOD
};
static_assert(std::is_nothrow_move_constructible_v<Track>);
```

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.13: Never throw while being the direct owner of an object

If a function holds a resource through a raw pointer or handle when it
throws, the resource leaks: the code that would release it never runs. Hand
the resource to an owning object before anything can throw.

```cpp bad
#include <stdexcept>
void build(int n) {
    int* buffer = new int[100];
    if (n < 0) throw std::invalid_argument{"negative"};   // BAD: buffer leaks
    use(buffer, n);
    delete[] buffer;
}
```

```cpp
#include <stdexcept>
#include <vector>
void build(int n) {
    std::vector<int> buffer(100);   // GOOD: freed by its destructor, even on the throw
    if (n < 0) throw std::invalid_argument{"negative"};
    use(buffer, n);
}
```

The BAD version compiles; it leaks 400 bytes every time `n` is negative.

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.14: Use purpose-designed user-defined types as exceptions (not built-in types)

Throw a type made for the error. Its name says what went wrong, it can carry
details, and a handler can catch exactly it. An `int` or a string literal
carries no meaning, and could clash with anyone else's `int`.

```cpp bad
void load(int id) {
    if (id < 0) throw 42;                 // BAD: what does 42 mean, and who catches int?
    if (id == 0) throw "no such record";  // BAD: a const char*, caught by nobody expecting std::exception
}
```

```cpp
#include <stdexcept>
#include <string>
struct RecordNotFound : std::runtime_error {   // GOOD: named, carries the id, is a std::exception
    int id;
    explicit RecordNotFound(int i)
        : std::runtime_error{"record not found: " + std::to_string(i)}, id{i} {}
};
void load(int id) {
    if (id <= 0) throw RecordNotFound{id};
}
```

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.15: Throw by value, catch exceptions from a hierarchy by reference

Throw an object, not a pointer to one (who would delete it?). Catch by
`const&`: catching a base class by value copies only the base part, so the
handler loses the derived type and its overrides (slicing). To rethrow, write
`throw;`, which rethrows the original object.

```cpp bad
#include <stdexcept>
void handle() {
    try {
        throw new std::out_of_range{"index"};    // BAD: a pointer; nothing deletes it
    } catch (std::logic_error e) {               // BAD: by value; a thrown out_of_range would be sliced
        throw e;                                 // BAD: throws a sliced copy, not the original
    }
}
```

```cpp
#include <stdexcept>
void handle() {
    try {
        throw std::out_of_range{"index"};        // GOOD: by value
    } catch (const std::logic_error& e) {        // GOOD: by const&, keeps the dynamic type
        use(e.what());
        throw;                                   // GOOD: rethrows the original out_of_range
    }
}
```

The BAD version compiles (GCC's `-Wcatch-value` warns about the catch); the
pointer leaks and isn't caught by the `catch` at all.

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.16: Destructors, deallocation, `swap`, and exception type copy/move construction must never fail

Destructors run during unwinding, and the library assumes they, deallocation
and `swap` never throw. A destructor is `noexcept` by default, so a `throw`
that escapes one calls `std::terminate`. Report a failure some other way
(log it, or offer an explicit `close()` that can throw).

```cpp bad
#include <stdexcept>
struct Connection {
    bool flush();
    ~Connection() {
        if (!flush()) throw std::runtime_error{"flush failed"};   // BAD: destructor is noexcept: terminates
    }
};
```

```cpp
#include <type_traits>
struct Connection {
    bool flush();
    void close() { if (!flush()) { /* report, or throw from here: callers can catch it */ } }
    ~Connection() { flush(); }   // GOOD: best effort, never throws
};
static_assert(std::is_nothrow_destructible_v<Connection>);
```

The BAD version compiles (GCC and Clang warn that the `throw` will call
`std::terminate`).

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.17: Don't try to catch every exception in every function

Catch an exception only where something useful can be done about it. A
function that can't recover should let it pass: RAII cleans up on the way,
and a handler higher up deals with it.

```cpp bad
#include <string>
std::string read_config(const std::string& path);
std::string settings(const std::string& path) {
    try {
        return read_config(path);
    } catch (...) {
        throw;   // BAD: catches only to rethrow; noise
    }
}
```

```cpp
#include <string>
std::string read_config(const std::string& path);
std::string settings(const std::string& path) {
    return read_config(path);   // GOOD: the caller that can recover catches
}
```

The BAD version compiles and behaves the same; it's clutter that hides the
real handlers.

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.18: Minimize the use of explicit `try`/`catch`

A `try`/`catch` written to free a resource is a sign the resource has no
owner. Use an owning object instead; the code gets shorter and handles every
exception, not just the ones you listed.

```cpp bad
#include <stdexcept>
void run_job() {
    Widget* w = new Widget;
    try {
        w->run();
    } catch (const std::exception&) {   // BAD: manual cleanup; a non-std exception still leaks w
        delete w;
        throw;
    }
    delete w;
}
```

```cpp
#include <memory>
void run_job() {
    auto w = std::make_unique<Widget>();   // GOOD: no try/catch needed
    w->run();
}
```

The BAD version compiles; it's verbose and still leaks on an exception that
isn't a `std::exception`.

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.19: Use a `final_action` object to express cleanup if no suitable resource handle is available

When a cleanup step has no owning type (a C API, a one-off flag), wrap it in
a small object whose destructor runs the step: `gsl::finally` from the GSL,
or a few lines of your own as below. It still runs on every exit path. A real
resource type is better when one exists.

```cpp bad
void with_lock_flag(bool& busy) {
    busy = true;
    process();
    busy = false;   // BAD: skipped if process throws, and on any early return
}
```

```cpp
#include <utility>
template <class F>
class final_action {
    F f_;
public:
    explicit final_action(F f) : f_{std::move(f)} {}
    final_action(const final_action&) = delete;
    final_action& operator=(const final_action&) = delete;
    ~final_action() { f_(); }
};

void with_lock_flag(bool& busy) {
    busy = true;
    final_action reset{[&] { busy = false; }};   // GOOD: runs however we leave
    process();
}
```

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.25: If you can't throw exceptions, simulate RAII for resource management

Without exceptions, keep RAII anyway: the destructor still releases the
resource, and the object records whether construction succeeded. Make the
check hard to forget, with `[[nodiscard]]` on what reports it.

```cpp bad
#include <cstdio>
int save(const char* path) {
    std::FILE* f = std::fopen(path, "w");
    if (!f) return -1;
    if (std::fputs("data", f) < 0) return -2;   // BAD: forgets to close f on this path
    std::fclose(f);
    return 0;
}
```

```cpp
#include <cstdio>
class OutFile {
    std::FILE* f_;
public:
    explicit OutFile(const char* path) noexcept : f_{std::fopen(path, "w")} {}
    OutFile(const OutFile&) = delete;
    OutFile& operator=(const OutFile&) = delete;
    ~OutFile() { if (f_) std::fclose(f_); }       // still releases on every path
    [[nodiscard]] bool valid() const noexcept { return f_ != nullptr; }
    [[nodiscard]] bool write(const char* s) noexcept { return std::fputs(s, f_) >= 0; }
};

[[nodiscard]] int save(const char* path) noexcept {
    OutFile f{path};
    if (!f.valid()) return -1;            // GOOD: construction failure is checked
    if (!f.write("data")) return -2;      // and the destructor closes the file
    return 0;
}
```

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.26: If you can't throw exceptions, consider failing fast

If an error can't be handled where it's found and can't be reported
systematically either, stop the program at once (`std::abort`, after logging)
rather than continue in a corrupt state. A supervisor or the next level up
can restart it.

```cpp bad
#include <cstdlib>
int* table(std::size_t n) {
    auto p = static_cast<int*>(std::malloc(n * sizeof(int)));
    return p;   // BAD: a null p is returned silently and dereferenced somewhere else
}
```

```cpp
#include <cstdio>
#include <cstdlib>
int* table(std::size_t n) {
    auto p = static_cast<int*>(std::malloc(n * sizeof(int)));
    if (!p) {
        std::fputs("table: out of memory\n", stderr);
        std::abort();   // GOOD: stop here, with a reason, before the damage spreads
    }
    return p;
}
```

The BAD version compiles; it fails later and far from the cause.

## E.27: If you can't throw exceptions, use error codes systematically

If errors are values, use one shape everywhere: every fallible function
returns the result and the error together, and every caller checks. C++23's
`std::expected<T, E>` is that shape; `[[nodiscard]]` on a plain error code is
the minimum.

```cpp bad
int parse_port(const char* s, int& out);   // BAD: returns 0 on success? errno? out set on failure?
void connect(const char* s) {
    int port;
    parse_port(s, port);   // the status is ignored, port may be garbage
    use(port);
}
```

```cpp
#include <charconv>
#include <expected>
#include <string_view>
#include <system_error>
std::expected<int, std::errc> parse_port(std::string_view s) {
    int v = 0;
    auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{}) return std::unexpected{ec};
    if (end != s.data() + s.size() || v < 1 || v > 65535) return std::unexpected{std::errc::invalid_argument};
    return v;   // GOOD: one type carries the value or the error
}
void connect(std::string_view s) {
    if (auto port = parse_port(s)) use(*port);
    else use(std::make_error_code(port.error()).message());
}
```

`std::expected` is C++23 (GCC 12, Clang 16 with libc++, MSVC 19.33).

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.28: Avoid error handling based on global state (e.g. `errno`)

A global error variable is easy to forget to check, is overwritten by the
next call, and is shared between callers. Return the error with the result
instead. The standard library's newer functions do: `std::from_chars`
returns its error, where `std::strtol` sets `errno`.

```cpp bad
#include <cerrno>
#include <cstdlib>
long to_long(const char* s) {
    errno = 0;
    long v = std::strtol(s, nullptr, 10);
    return v;   // BAD: the caller must remember to look at errno, before anything else touches it
}
```

```cpp
#include <charconv>
#include <optional>
#include <string_view>
std::optional<long> to_long(std::string_view s) {
    long v = 0;
    auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), v);   // GOOD: the error is returned
    if (ec != std::errc{} || end != s.data() + s.size()) return std::nullopt;
    return v;
}
```

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.30: Don't use exception specifications

Dynamic exception specifications (`throw(X, Y)`) were removed in C++17; the
compiler now rejects them. The only specification left is `noexcept`: a
function either may throw anything or promises not to throw.

```cpp compile-fail="dynamic exception specification"
#include <stdexcept>
int checked(int v) throw(std::out_of_range) {   // BAD: not C++17 or later
    if (v < 0) throw std::out_of_range{"negative"};
    return v;
}
```

```cpp
#include <stdexcept>
int checked(int v) {                 // GOOD: may throw; document what
    if (v < 0) throw std::out_of_range{"negative"};
    return v;
}
int twice(int v) noexcept { return 2 * v; }   // GOOD: can't throw
```

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).

## E.31: Properly order your `catch`-clauses

Handlers are tried in order and the first that matches wins, so put the most
derived types first and `catch (...)` last. A base handler before a derived
one hides it (compilers warn); a `catch (...)` that isn't last is an error.

```cpp compile-fail="must be the last handler|catch-all handler must come last"
#include <stdexcept>
void run() {
    try {
        process();
    } catch (...) {                        // BAD: catches everything...
    } catch (const std::exception& e) {    // ...so this can never run
        use(e.what());
    }
}
```

```cpp
#include <stdexcept>
void run() {
    try {
        process();
    } catch (const std::out_of_range& e) {   // GOOD: most derived first
        use(e.what());
    } catch (const std::exception& e) {      // then its bases
        use(e.what());
    } catch (...) {                          // catch-all last
    }
}
```

Tested by [`tests/guidelines/errors_test.cpp`](../../../../tests/guidelines/errors_test.cpp).
