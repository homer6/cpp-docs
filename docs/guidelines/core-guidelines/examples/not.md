# NR: Non-Rules and myths — examples

> Our examples for the [NR entries](../not.md): popular coding rules that
> the Guidelines argue against. Each shows code written to obey the myth, then
> the code you get without it.

## NR.1: Don't insist that all declarations should be at the top of a function

Declaring everything at the top made sense when the language required it.
Today it just puts distance between a variable and its first real value, and
leaves variables uninitialized in between. Declare each one where you can
initialize it.

```cpp bad
#include <string>
// BAD: compiles; the variables are declared long before they get a value,
// and name is still empty if someone adds an early use of it.
std::string label(int id, bool admin) {
    std::string name;
    std::string prefix;
    int width;
    prefix = admin ? "admin-" : "user-";
    width = id < 10 ? 1 : 2;
    name = prefix + std::to_string(id);
    return width > 1 ? name : name + "-short";
}
```

```cpp
#include <string>
std::string label(int id, bool admin) {
    const std::string prefix = admin ? "admin-" : "user-";   // GOOD: declared with its value
    const std::string name = prefix + std::to_string(id);
    return id < 10 ? name + "-short" : name;
}
```

## NR.2: Don't insist on having only a single `return`-statement in a function

Forcing one exit point adds result variables and nesting. Returning as soon
as the answer is known, especially for argument checks at the top, is shorter
and easier to follow.

```cpp bad
// BAD: compiles; the single return needs a result variable and nested ifs.
int classify(int x) {
    int result;
    if (x < 0) {
        result = -1;
    } else {
        if (x == 0) result = 0;
        else result = 1;
    }
    return result;
}
```

```cpp
int classify(int x) {
    if (x < 0) return -1;     // GOOD: each case returns as soon as it's known
    if (x == 0) return 0;
    return 1;
}
```

Tested by [`tests/guidelines/not_test.cpp`](../../../../tests/guidelines/not_test.cpp).

## NR.3: Don't avoid exceptions

An error code can be ignored by the caller, and every layer has to pass it
along by hand. An exception can't be ignored, separates the error path from
the normal one, and, with RAII, cleans up on the way out.

```cpp bad
#include <string>
// BAD: compiles; the caller forgot to check the code, so it carries on with port 0.
int parse_port(const std::string& s, int& out) {
    if (s.empty()) return -1;
    out = std::stoi(s);
    return 0;
}
void connect(const std::string& s) {
    int port = 0;
    parse_port(s, port);       // the error is silently dropped
    use(port);
}
```

```cpp
#include <stdexcept>
#include <string>
int parse_port(const std::string& s) {
    if (s.empty()) throw std::invalid_argument("empty port");   // GOOD: can't be ignored
    return std::stoi(s);
}
void connect(const std::string& s) { use(parse_port(s)); }
```

Tested by [`tests/guidelines/not_test.cpp`](../../../../tests/guidelines/not_test.cpp).

## NR.4: Don't insist on placing each class definition in its own source file

*No example: it's about how source files are organized (cohesive files and namespaces rather than one file per class), which a single snippet can't show.*

## NR.5: Don't use two-phase initialization

A constructor followed by a separate `init()` leaves a window where the
object exists but isn't usable, and every member function has to wonder
whether `init()` ran. Let the constructor establish the invariant, and throw if
it can't.

```cpp bad
#include <string>
// BAD: compiles; between construction and init() the object is half-made,
// and nothing stops a caller from skipping init() entirely.
class Connection {
public:
    Connection() = default;
    bool init(const std::string& host) { host_ = host; ready_ = !host.empty(); return ready_; }
    void send(const std::string& msg) { if (ready_) use(host_, msg); }
private:
    std::string host_;
    bool ready_ = false;
};
```

```cpp
#include <stdexcept>
#include <string>
#include <utility>
class Connection {
public:
    explicit Connection(std::string host) : host_{std::move(host)} {   // GOOD: usable once built
        if (host_.empty()) throw std::invalid_argument("no host");
    }
    void send(const std::string& msg) { use(host_, msg); }
private:
    std::string host_;
};
```

Tested by [`tests/guidelines/not_test.cpp`](../../../../tests/guidelines/not_test.cpp).

## NR.6: Don't place all cleanup actions at the end of a function and `goto exit`

The `goto exit` pattern routes every error to one cleanup block at the end.
It's easy to jump past an initialization or forget to release something on one
path. Destructors (RAII) do the cleanup on every exit, including exceptions.

```cpp bad
#include <cstdio>
#include <cstdlib>
// BAD: compiles; every path must reach "done", and a new early return leaks both.
int copy_first_line(const char* path) {
    int rc = -1;
    char* buf = static_cast<char*>(std::malloc(256));
    std::FILE* f = std::fopen(path, "r");
    if (!buf || !f) goto done;
    if (!std::fgets(buf, 256, f)) goto done;
    rc = 0;
done:
    if (f) std::fclose(f);
    std::free(buf);
    return rc;
}
```

```cpp
#include <cstdio>
#include <memory>
#include <string>
struct FileCloser { void operator()(std::FILE* f) const { std::fclose(f); } };

int copy_first_line(const char* path) {
    std::unique_ptr<std::FILE, FileCloser> f{std::fopen(path, "r")};   // GOOD: closed on every exit
    if (!f) return -1;
    std::string buf(256, '\0');
    if (!std::fgets(buf.data(), static_cast<int>(buf.size()), f.get())) return -1;
    return 0;
}
```

Tested by [`tests/guidelines/not_test.cpp`](../../../../tests/guidelines/not_test.cpp).

## NR.7: Don't make data members `protected`

`protected` data can be changed by every derived class, written by anyone,
anywhere, so the base class can't keep its invariant. Keep data `private` and
give derived classes functions that preserve the invariant.

```cpp bad
// BAD: compiles; any derived class can break the "balance >= 0" invariant.
class Account {
protected:
    long balance_ = 0;
};
class Promo : public Account {
public:
    void bonus() { balance_ -= 1000; }   // nothing stops this
};
```

```cpp
class Account {
public:
    long balance() const { return balance_; }
protected:
    void deposit(long amount) { if (amount > 0) balance_ += amount; }   // GOOD: keeps the invariant
private:
    long balance_ = 0;
};
class Promo : public Account {
public:
    void bonus() { deposit(100); }
};
```

A derived class that tries to go around the invariant no longer compiles:

```cpp compile-fail="is private|private member"
class Account {
private:
    long balance_ = 0;
};
class Promo : public Account {
public:
    void cheat() { balance_ -= 1000; }   // error: balance_ is private
};
```

Tested by [`tests/guidelines/not_test.cpp`](../../../../tests/guidelines/not_test.cpp).
