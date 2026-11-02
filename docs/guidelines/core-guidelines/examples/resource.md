# R: Resource management — examples

> Our examples for the [R rules](../resource.md): every resource has an owner
> object that releases it, on every path, exceptions included.

## R.1: Manage resources automatically using resource handles and RAII (Resource Acquisition Is Initialization)

Wrap anything that must be released (memory, files, locks, sockets) in an
object whose constructor acquires it and whose destructor releases it. Then
every exit from the scope (a `return`, a `break`, an exception) releases it
exactly once, without anyone having to remember.

```cpp bad
#include <cstdio>
#include <mutex>
std::mutex m;
bool log_line(const char* path, const char* text) {
    std::FILE* f = std::fopen(path, "a");
    m.lock();
    if (std::fputs(text, f) < 0) return false;   // BAD: f stays open, m stays locked
    m.unlock();
    std::fclose(f);
    return true;
}
```

```cpp
#include <cstdio>
#include <memory>
#include <mutex>
struct FileCloser {
    void operator()(std::FILE* f) const noexcept { std::fclose(f); }
};
using File = std::unique_ptr<std::FILE, FileCloser>;   // the file's owner
std::mutex m;
bool log_line(const char* path, const char* text) {
    File f{std::fopen(path, "a")};
    if (!f) return false;
    std::lock_guard lock{m};                    // GOOD: owners release on every path
    return std::fputs(text, f.get()) >= 0;
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.2: In interfaces, use raw pointers to denote individual objects (only)

A `T*` parameter should point at one object (or be null). A pointer plus a
count says nothing the compiler can check; a range belongs in a container or a
`std::span`, which carries its size.

```cpp bad
void zero(int* values, int n) {           // BAD: is n right? nothing checks it
    for (int i = 0; i < n; ++i) values[i] = 0;
}
int buffer[4];
void caller() { zero(buffer, 8); }        // compiles, writes past the end
```

```cpp
#include <span>
void zero(std::span<int> values) {        // GOOD: the size travels with the data
    for (int& v : values) v = 0;
}
struct Counter { int hits = 0; };
void bump(Counter* c) { if (c) ++c->hits; }   // GOOD: one object, maybe null
int buffer[4];
void caller() { zero(buffer); }           // size deduced: 4
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.3: A raw pointer (a `T*`) is non-owning

Treat every `T*` as borrowed: whoever holds it doesn't delete it. Owning
pointers are `std::unique_ptr` or `std::shared_ptr` (or a value returned by
value), so ownership is visible in the type. The GSL's `gsl::owner<T*>` is
only an annotation for legacy code that must keep a raw owning pointer.

```cpp bad
struct Gadget { int id; };
Gadget* make_gadget(int id) { return new Gadget{id}; }   // BAD: who deletes it?
void caller() {
    Gadget* g = make_gadget(1);
    use(g);                      // leaks unless every path remembers delete
}
```

```cpp
#include <memory>
struct Gadget { int id; };
Gadget make_gadget(int id) { return Gadget{id}; }                 // GOOD: a value
std::unique_ptr<Gadget> make_owned_gadget(int id) {              // GOOD: an owner
    return std::make_unique<Gadget>(id);
}
void inspect(const Gadget* g) { use(g); }                         // borrows only
void caller() {
    auto owned = make_owned_gadget(1);
    inspect(owned.get());
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.4: A raw reference (a `T&`) is non-owning

A reference refers to an object someone else owns; it never deletes it.
Binding a reference to a fresh `new` makes an owner that doesn't look like
one.

```cpp bad
void f() {
    int& r = *new int{7};    // BAD: an owning reference
    use(r);
    delete &r;               // easy to forget, and wrong if r was ever rebound
}
```

```cpp
#include <string>
void shout(std::string& s) { s += '!'; }   // GOOD: borrows the caller's string
void f() {
    std::string s = "hi";
    shout(s);
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.5: Prefer scoped objects, don't heap-allocate unnecessarily

A local object, a member or a global costs no extra allocation, and its
destructor runs when its scope ends. Use the heap only when the object must
outlive the scope, is too big for the stack, or its size is only known at run
time.

```cpp bad
struct Gadget { explicit Gadget(int); };
void f(int n) {
    auto* g = new Gadget{n};     // BAD: an allocation for nothing
    use(*g);                     // if this throws, g leaks
    delete g;
}
```

```cpp
struct Gadget { explicit Gadget(int) {} };
void f(int n) {
    Gadget g{n};                 // GOOD: destroyed at the end of the scope, always
    use(g);
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.6: Avoid non-`const` global variables

The same rule as I.2: a mutable global is a hidden input and output of every
function that touches it, and a data race waiting for a second thread. Pass
the state in, or make it a member of an object.

```cpp bad
int open_connections = 0;                   // BAD: anyone can change it
void connect() { ++open_connections; }
```

```cpp
class Pool {
    int open_ = 0;                          // GOOD: owned by an object
public:
    void connect() { ++open_; }
    int open() const { return open_; }
};
constexpr int max_connections = 16;         // constants are fine
```

## R.10: Avoid `malloc()` and `free()`

`malloc` returns raw bytes: no constructor runs, so an object with a
`std::string` member isn't a valid object yet, and `free` runs no destructor.
Mixing them with `new`/`delete` is undefined behavior. Use `std::make_unique`,
a container, or a local.

```cpp bad
#include <cstdlib>
#include <string>
struct Record { int id; std::string name; };
void f() {
    auto* r = static_cast<Record*>(std::malloc(sizeof(Record)));
    r->name = "x";      // BAD: undefined behavior: no string was ever constructed
    std::free(r);       // and no destructor runs
}
```

```cpp
#include <memory>
#include <string>
struct Record { int id; std::string name; };
void f() {
    auto r = std::make_unique<Record>();   // GOOD: constructed, destroyed, freed
    r->name = "x";
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.11: Avoid calling `new` and `delete` explicitly

Every naked `new` needs a matching `delete` on every path, and nothing checks
that you wrote it. Let a factory (`std::make_unique`, `std::make_shared`) or a
container own the object instead; `delete` then lives in one place, the
owner's destructor.

```cpp bad
#include <stdexcept>
struct Shape { virtual ~Shape() = default; };
void draw() {
    Shape* s = new Shape;
    if (!s) return;
    use(*s);                                   // BAD: a throw here leaks s
    delete s;
}
```

```cpp
#include <memory>
struct Shape { virtual ~Shape() = default; };
void draw() {
    auto s = std::make_unique<Shape>();         // GOOD: no new, no delete
    use(*s);
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.12: Immediately give the result of an explicit resource allocation to a manager object

When you must call an acquire function yourself (`fopen`, a C library's
`create`), hand the result to its owner in the same statement. Anything that
runs in between, even an allocation that can throw, can leak it.

```cpp bad
#include <cstdio>
#include <vector>
void read_header(const char* path) {
    std::FILE* f = std::fopen(path, "rb");
    std::vector<char> buf(4096);              // BAD: if this throws, f leaks
    std::fread(buf.data(), 1, buf.size(), f);
    std::fclose(f);
}
```

```cpp
#include <cstdio>
#include <memory>
#include <vector>
struct FileCloser {
    void operator()(std::FILE* f) const noexcept { std::fclose(f); }
};
void read_header(const char* path) {
    std::unique_ptr<std::FILE, FileCloser> f{std::fopen(path, "rb")};   // GOOD: owned at once
    if (!f) return;
    std::vector<char> buf(4096);
    std::fread(buf.data(), 1, buf.size(), f.get());
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.13: Perform at most one explicit resource allocation in a single expression statement

Two `new`s in one expression can leak: the first object is allocated, the
second allocation or constructor throws, and the first raw pointer hasn't
reached its owner yet. Since C++17 each function argument is evaluated
completely before the next, so `f(std::unique_ptr<T>(new T), …)` no longer
interleaves, but raw pointers handed to one constructor still leak. The
factories avoid the question.

```cpp bad
#include <memory>
#include <utility>
struct Engine { };
struct Wheel { Wheel(); };           // the constructor may throw
using Parts = std::pair<std::unique_ptr<Engine>, std::unique_ptr<Wheel>>;
Parts build() {
    return Parts{new Engine, new Wheel};   // BAD: if Wheel() throws, the Engine leaks
}
```

```cpp
#include <memory>
#include <utility>
struct Engine { };
struct Wheel { Wheel(); };
using Parts = std::pair<std::unique_ptr<Engine>, std::unique_ptr<Wheel>>;
Parts build() {
    return Parts{std::make_unique<Engine>(), std::make_unique<Wheel>()};   // GOOD
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.14: Avoid `[]` parameters, prefer `span`

A parameter declared `int a[]` (or `int a[10]`) is really `int*`: the size is
gone, `sizeof(a)` is the size of a pointer, and any array length is accepted.
`std::span` keeps the size.

```cpp bad
#include <cstddef>
std::size_t count(int a[10]) {
    return sizeof(a) / sizeof(a[0]);   // BAD: sizeof a pointer, not 10 ints
}
```

```cpp
#include <cstddef>
#include <span>
std::size_t count(std::span<const int> a) {
    return a.size();                   // GOOD: the real size
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.15: Always overload matched allocation/deallocation pairs

A class that defines its own `operator new` must define the matching
`operator delete` (and the array forms if it uses them); otherwise memory from
one allocator goes back to another. If the memory must never be freed,
declare the deallocation function `= delete` rather than leaving it out.

```cpp bad
#include <cstddef>
void* pool_alloc(std::size_t);
struct Message {
    static void* operator new(std::size_t n) { return pool_alloc(n); }
    // BAD: no operator delete, so `delete msg` hands pool memory to the global heap
};
```

```cpp
#include <cstddef>
void* pool_alloc(std::size_t);
void pool_free(void*) noexcept;
struct Message {
    static void* operator new(std::size_t n) { return pool_alloc(n); }
    static void operator delete(void* p) noexcept { pool_free(p); }   // GOOD: a pair
};
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.20: Use `unique_ptr` or `shared_ptr` to represent ownership

When a pointer owns its object, make it a smart pointer: the object is then
destroyed when its last owner goes away, on every path.

```cpp bad
struct Texture { };
void load() {
    Texture* t{new Texture};          // BAD: owns, but nothing will delete it
    use(t);
}                                     // leak
```

```cpp
#include <memory>
struct Texture { };
void load() {
    auto t = std::make_unique<Texture>();        // GOOD: sole owner
    auto shared = std::make_shared<Texture>();   // GOOD: shared owners
    use(t, shared);
}                                                 // both freed here
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.21: Prefer `unique_ptr` over `shared_ptr` unless you need to share ownership

`unique_ptr` is the size of a raw pointer, has no reference count to update,
and you know exactly where the object dies. Reach for `shared_ptr` only when
several owners really share the object's lifetime. A `unique_ptr` converts to
a `shared_ptr` later if that need appears.

```cpp bad
#include <memory>
struct Parser { };
void parse() {
    std::shared_ptr<Parser> p = std::make_shared<Parser>();   // BAD: never shared
    use(*p);                                  // a control block and atomic counts for nothing
}
```

```cpp
#include <memory>
struct Parser { };
void parse() {
    auto p = std::make_unique<Parser>();      // GOOD: one owner, no count
    use(*p);
}
```

A `unique_ptr` can't be copied by accident: a second owner has to be made on
purpose (`std::move`, or a `shared_ptr`).

```cpp compile-fail="deleted|delete"
#include <memory>
struct Parser { };
void parse() {
    auto p = std::make_unique<Parser>();
    auto q = p;                               // error: unique_ptr's copy constructor is deleted
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.22: Use `make_shared()` to make `shared_ptr`s

`std::make_shared<T>(args)` names `T` once and allocates the object and its
reference counts together, in one allocation instead of two. It also never
leaves a raw pointer lying around.

```cpp bad
#include <memory>
struct Session { explicit Session(int); };
std::shared_ptr<Session> s{new Session{42}};   // BAD: compiles, but two allocations and T named twice
```

```cpp
#include <memory>
struct Session { explicit Session(int) {} };
auto s = std::make_shared<Session>(42);        // GOOD: one allocation
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.23: Use `make_unique()` to make `unique_ptr`s

`std::make_unique<T>(args)` (C++14) says the type once and keeps `new` out of
the code, so a search for `new` finds only the code that really manages
memory.

```cpp bad
#include <memory>
struct Buffer { explicit Buffer(int); };
std::unique_ptr<Buffer> b{new Buffer{64}};   // BAD: compiles, but repetitive and a naked new
```

```cpp
#include <memory>
struct Buffer { explicit Buffer(int) {} };
auto b = std::make_unique<Buffer>(64);       // GOOD
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.24: Use `std::weak_ptr` to break cycles of `shared_ptr`s

Two objects that hold `shared_ptr`s to each other keep each other's count
above zero forever, so neither is destroyed. Make one direction (usually the
back pointer to a parent or owner) a `std::weak_ptr`, and `lock()` it when
you need it.

```cpp bad
#include <memory>
struct Child;
struct Parent { std::shared_ptr<Child> child; };
struct Child { std::shared_ptr<Parent> parent; };   // BAD: a cycle
void f() {
    auto p = std::make_shared<Parent>();
    p->child = std::make_shared<Child>();
    p->child->parent = p;
}                                                   // neither is ever destroyed: a leak
```

```cpp
#include <memory>
struct Child;
struct Parent { std::shared_ptr<Child> child; };
struct Child { std::weak_ptr<Parent> parent; };     // GOOD: doesn't own its parent
void f() {
    auto p = std::make_shared<Parent>();
    p->child = std::make_shared<Child>();
    p->child->parent = p;
    if (auto parent = p->child->parent.lock()) use(*parent);
}                                                   // both destroyed here
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.30: Take smart pointers as parameters only to explicitly express lifetime semantics

(The same advice as F.7.) A function that only uses an object should take
`T&` or `T*`. A smart-pointer parameter says "I take part in this object's
lifetime", and it rejects every caller whose object is owned differently.

```cpp compile-fail="no matching function|could not convert|cannot convert|no viable conversion|invalid initialization"
#include <memory>
struct Shape { double area() const { return 1.0; } };
double area_of(const std::unique_ptr<Shape>& s) { return s->area(); }   // BAD: only reads
void caller() {
    Shape local;
    area_of(local);                   // error: a local isn't owned by a unique_ptr
}
```

```cpp
#include <memory>
struct Shape { double area() const { return 1.0; } };
double area_of(const Shape& s) { return s.area(); }   // GOOD: any Shape will do
void caller() {
    Shape local;
    auto owned = std::make_unique<Shape>();
    auto shared = std::make_shared<Shape>();
    area_of(local);
    area_of(*owned);
    area_of(*shared);
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.31: If you have non-`std` smart pointers, follow the basic pattern from `std`

A home-grown handle should behave like the standard ones: unique ownership
means no copying, moving transfers it, `*` and `->` reach the object, and the
destructor releases it. Code reviewers and analysis tools then read it the
same way.

```cpp bad
template <class T>
struct Handle {
    T* p;
    ~Handle() { delete p; }
    T* get() const { return p; }
};
void f() {
    Handle<int> a{new int{1}};
    Handle<int> b = a;                 // BAD: copies the pointer: a double delete
}
```

```cpp
#include <utility>
template <class T>
class Handle {                         // GOOD: shaped like unique_ptr
    T* p_ = nullptr;
public:
    Handle() = default;
    explicit Handle(T* p) noexcept : p_{p} {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&& o) noexcept : p_{std::exchange(o.p_, nullptr)} {}
    Handle& operator=(Handle&& o) noexcept {
        if (this != &o) { delete p_; p_ = std::exchange(o.p_, nullptr); }
        return *this;
    }
    ~Handle() { delete p_; }
    T& operator*() const { return *p_; }
    T* operator->() const { return p_; }
    explicit operator bool() const noexcept { return p_ != nullptr; }
};
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.32: Take a `unique_ptr<widget>` parameter to express that a function assumes ownership of a `widget`

A by-value `std::unique_ptr<T>` parameter is a sink: the caller must hand
ownership over with `std::move`, and the type documents and enforces it.

```cpp bad
struct Job { };
void enqueue(Job* job);              // BAD: does it keep job? delete it later?
void caller() {
    auto* j = new Job;
    enqueue(j);
    delete j;                        // double delete if enqueue took ownership
}
```

```cpp
#include <memory>
#include <utility>
#include <vector>
struct Job { };
std::vector<std::unique_ptr<Job>> queue;
void enqueue(std::unique_ptr<Job> job) { queue.push_back(std::move(job)); }   // GOOD: a sink
void caller() {
    auto j = std::make_unique<Job>();
    enqueue(std::move(j));           // j is now empty; the queue owns the Job
}
```

Forgetting `std::move` is caught, because passing would copy:

```cpp compile-fail="deleted|delete"
#include <memory>
struct Job { };
void enqueue(std::unique_ptr<Job> job);
void caller() {
    auto j = std::make_unique<Job>();
    enqueue(j);                      // error: unique_ptr can't be copied
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.33: Take a `unique_ptr<widget>&` parameter to express that a function reseats the `widget`

A non-`const` `std::unique_ptr<T>&` parameter means "I may replace the object
you own". If the function only uses the object, take `T&` (or `T*`) instead,
so callers aren't misled and any `T` can be passed.

```cpp bad
#include <memory>
struct Config { int level = 0; };
void print(std::unique_ptr<Config>& c) { use(c->level); }   // BAD: never reseats
```

```cpp
#include <memory>
struct Config { int level = 0; };
void reload(std::unique_ptr<Config>& c) {                   // GOOD: replaces it
    c = std::make_unique<Config>(Config{2});
}
void print(const Config& c) { use(c.level); }               // GOOD: only reads
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.34: Take a `shared_ptr<widget>` parameter to express shared ownership

A by-value `std::shared_ptr<T>` parameter says the function (or the object
it builds) becomes one more owner. Move it into place to avoid an extra count
update.

```cpp bad
#include <memory>
struct Texture { };
struct Sprite {
    std::shared_ptr<Texture> tex;
    explicit Sprite(Texture* t) : tex{t} {}   // BAD: a second, separate owner of *t
};
void f() {
    auto t = std::make_shared<Texture>();
    Sprite s{t.get()};                        // two control blocks: a double delete
}
```

```cpp
#include <memory>
#include <utility>
struct Texture { };
struct Sprite {
    std::shared_ptr<Texture> tex;
    explicit Sprite(std::shared_ptr<Texture> t) : tex{std::move(t)} {}   // GOOD: shares
};
void f() {
    auto t = std::make_shared<Texture>();
    Sprite s{t};                              // use_count is now 2
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.35: Take a `shared_ptr<widget>&` parameter to express that a function might reseat the shared pointer

When a function may point the caller's `shared_ptr` at a different object,
take it by non-`const` reference. By value, the function would only change its
own copy.

```cpp bad
#include <memory>
struct Theme { int id; };
void switch_theme(std::shared_ptr<Theme> t) {      // BAD: reseats a copy
    t = std::make_shared<Theme>(Theme{2});          // the caller never sees it
}
```

```cpp
#include <memory>
struct Theme { int id; };
void switch_theme(std::shared_ptr<Theme>& t) {     // GOOD: reseats the caller's pointer
    t = std::make_shared<Theme>(Theme{2});
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.36: Take a `const shared_ptr<widget>&` parameter to express that it might retain a reference count to the object ???

A `const std::shared_ptr<T>&` parameter is for a function that *might* keep
a copy: it pays for a count update only on the path that copies. A function
that always keeps it takes it by value (R.34); one that never does takes `T&`.

```cpp bad
#include <memory>
#include <vector>
struct Event { bool important; };
std::vector<std::shared_ptr<Event>> kept;
void maybe_keep(std::shared_ptr<Event> e) {         // BAD: copies on every call
    if (e->important) kept.push_back(e);
}
```

```cpp
#include <memory>
#include <vector>
struct Event { bool important; };
std::vector<std::shared_ptr<Event>> kept;
void maybe_keep(const std::shared_ptr<Event>& e) {  // GOOD: copies only when it keeps
    if (e->important) kept.push_back(e);
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).

## R.37: Do not pass a pointer or reference obtained from an aliased smart pointer

If you pass `*p` from a `shared_ptr` that other code can reach (a global, a
member, something shared), the callee may reset that smart pointer and destroy
the object while you still hold the reference. Take a local copy first: it
keeps the object alive for the whole call.

```cpp bad
#include <memory>
struct Doc { int pages = 1; };
std::shared_ptr<Doc> current = std::make_shared<Doc>();
void close_current() { current.reset(); }
void print(const Doc& d) {
    close_current();       // destroys the Doc if current was its only owner
    use(d.pages);          // dangling
}
void caller() { print(*current); }   // BAD: *current comes from an aliased pointer
```

```cpp
#include <memory>
struct Doc { int pages = 1; };
std::shared_ptr<Doc> current = std::make_shared<Doc>();
void close_current() { current.reset(); }
void print(const Doc& d) {
    close_current();
    use(d.pages);          // still alive: caller's pin owns it
}
void caller() {
    auto pin = current;    // GOOD: a local owner for the call
    print(*pin);
}
```

Tested by [`tests/guidelines/resource_test.cpp`](../../../../tests/guidelines/resource_test.cpp).
