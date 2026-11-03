# CP: Concurrency and parallelism — examples

> Our examples for the [CP rules](../concurrency.md): share less, synchronize
> what you share with RAII and the standard library's tools, and let joins,
> futures and atomics do the work instead of hand-rolled protocols.

## CP.1: Assume that your code will run as part of a multi-threaded program

Code written for one thread ends up called from many, often years later.
Hidden shared state, such as a function-local `static` that caches a result,
becomes a data race the day a second thread calls the function. Keep the state
in an object the caller owns (or make it `thread_local`).

```cpp bad
#include <string>
// BAD: compiles, but the two statics are shared by every thread that calls this:
// two callers at once race on them (undefined behaviour).
std::string to_label(int code) {
    static int last_code = -1;
    static std::string last_label;
    if (code != last_code) {
        last_code = code;
        last_label = "code-" + std::to_string(code);
    }
    return last_label;
}
```

```cpp
#include <string>
// GOOD: the cache belongs to whoever creates it: one per thread, or one guarded by a lock.
class LabelCache {
public:
    const std::string& get(int code) {
        if (code != last_code_) {
            last_code_ = code;
            last_label_ = "code-" + std::to_string(code);
        }
        return last_label_;
    }
private:
    int last_code_ = -1;
    std::string last_label_;
};

std::string to_label(int code) {
    thread_local LabelCache cache;   // or a member of the caller's object
    return cache.get(code);
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.2: Avoid data races

Two threads touching the same object at once, at least one of them writing,
with nothing ordering the accesses, is a data race: undefined behaviour, not
just a wrong number. Make the shared object atomic, guard it with a mutex, or
stop sharing it.

```cpp bad
// BAD: compiles; ++ on a plain long is a read, an add and a write. Request handlers
// on several threads race on it: updates get lost, and the behaviour is undefined.
long requests_served = 0;
void on_request() { ++requests_served; }
```

```cpp
#include <atomic>
// GOOD: fetch_add is one indivisible step, so no update is lost.
std::atomic<long> requests_served{0};
void on_request() { requests_served.fetch_add(1, std::memory_order_relaxed); }
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.3: Minimize explicit sharing of writable data

Data nobody writes can be read by any number of threads without a lock. Give
tasks read-only (`const`) input and let each produce its own result, rather
than having them all write into one shared object.

```cpp bad
#include <thread>
#include <vector>
// BAD: compiles; both threads push_back into the same vector with no lock: a data race.
std::vector<int> evens_and_odds(const std::vector<int>& in) {
    std::vector<int> out;
    std::thread a([&] { for (int x : in) if (x % 2 == 0) out.push_back(x); });
    std::thread b([&] { for (int x : in) if (x % 2 != 0) out.push_back(x); });
    a.join();
    b.join();
    return out;
}
```

```cpp
#include <future>
#include <vector>
// GOOD: the input is shared read-only; each task builds and returns its own vector.
std::vector<int> evens_and_odds(const std::vector<int>& in) {
    auto pick = [&in](int rem) {
        std::vector<int> part;
        for (int x : in) if (x % 2 == rem) part.push_back(x);
        return part;
    };
    auto evens = std::async(std::launch::async, pick, 0);
    auto odds  = std::async(std::launch::async, pick, 1);
    std::vector<int> out = evens.get();
    std::vector<int> rest = odds.get();
    out.insert(out.end(), rest.begin(), rest.end());
    return out;
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.4: Think in terms of tasks, rather than threads

Say what you want done ("compute this, give me the answer") and let a task
facility worry about threads. A raw thread makes you invent a channel for the
result and remember to join; a task returns its result, or its exception,
through a future.

```cpp bad
#include <thread>
// BAD: compiles, but the result travels through a variable the caller must not touch
// until join(), and an exception thrown by parse() would call std::terminate.
int parse(const char* text);
int parse_in_background(const char* text) {
    int result = 0;
    std::thread worker([&] { result = parse(text); });
    // ... other work, being careful not to read result yet ...
    worker.join();
    return result;
}
```

```cpp
#include <future>
// GOOD: the task hands back its value (or rethrows its exception) through get().
int parse(const char* text);
int parse_in_background(const char* text) {
    std::future<int> result = std::async(std::launch::async, parse, text);
    // ... other work ...
    return result.get();
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.8: Don't try to use `volatile` for synchronization

In C++, `volatile` only stops the compiler from removing or merging accesses.
It doesn't make an operation atomic and doesn't order memory between threads,
so a `volatile` flag or counter shared by threads is still a data race. Use
`std::atomic` (or a mutex).

```cpp bad
// BAD: compiles, but volatile is not synchronization: the read of `ready` races with
// the write, and nothing makes `payload` visible to the reader when it sees true.
volatile bool ready = false;
int payload = 0;
void producer() { payload = 42; ready = true; }
int consumer() {
    while (!ready) {}
    return payload;
}
```

```cpp
#include <atomic>
// GOOD: the atomic store/load pair orders the accesses, so the reader sees 42.
std::atomic<bool> ready{false};
int payload = 0;
void producer() { payload = 42; ready.store(true); }
int consumer() {
    while (!ready.load()) {}
    return payload;
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.9: Whenever feasible use tools to validate your concurrent code

*No example: the rule is about running tools on the code (ThreadSanitizer, Clang's `-Wthread-safety` annotations, static analysers), not about how the code is written.*

See [tooling](../../tooling.md).

## CP.20: Use RAII, never plain `lock()`/`unlock()`

A matching `unlock()` is easy to miss on an early `return` or when something
throws, and then every other thread waiting on the mutex hangs. A lock object
(`std::scoped_lock`, `std::lock_guard`, `std::unique_lock`) unlocks in its
destructor, on every path out of the scope.

```cpp bad
#include <mutex>
#include <vector>
std::mutex m;
std::vector<int> items;
// BAD: compiles, but the early return (or a throwing push_back) leaves m locked forever.
bool add(int x) {
    m.lock();
    if (x < 0) return false;
    items.push_back(x);
    m.unlock();
    return true;
}
```

```cpp
#include <mutex>
#include <vector>
std::mutex m;
std::vector<int> items;
// GOOD: the guard unlocks on every way out: return, throw or the end of the scope.
bool add(int x) {
    std::scoped_lock lock(m);
    if (x < 0) return false;
    items.push_back(x);
    return true;
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.21: Use `std::lock()` or `std::scoped_lock` to acquire multiple `mutex`es

When two threads take the same two mutexes in opposite orders, each can end
up holding one and waiting for the other: a deadlock. `std::scoped_lock` (or
`std::lock`) takes them all with a deadlock-avoidance algorithm, so the order
you list them in no longer matters.

```cpp bad
#include <mutex>
struct Account { std::mutex m; long balance = 0; };
// BAD: compiles; transfer(a, b) and transfer(b, a) running at once lock the two
// mutexes in opposite orders and can deadlock.
void transfer(Account& from, Account& to, long amount) {
    std::lock_guard<std::mutex> l1(from.m);
    std::lock_guard<std::mutex> l2(to.m);
    from.balance -= amount;
    to.balance += amount;
}
```

```cpp
#include <mutex>
struct Account { std::mutex m; long balance = 0; };
// GOOD: scoped_lock acquires both without deadlock, whatever order callers pass them in.
void transfer(Account& from, Account& to, long amount) {
    std::scoped_lock lock(from.m, to.m);
    from.balance -= amount;
    to.balance += amount;
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.22: Never call unknown code while holding a lock (e.g., a callback)

A callback can do anything, including calling back into your object and
asking for the lock you already hold (a deadlock with `std::mutex`), or
blocking for a long time while everyone else waits. Copy what you need under
the lock, release it, then call out.

```cpp bad
#include <functional>
#include <mutex>
#include <vector>
// BAD: compiles, but a listener that calls subscribe() or size() locks m again: deadlock.
class Publisher {
public:
    void subscribe(std::function<void(int)> f) { std::scoped_lock l(m_); subs_.push_back(std::move(f)); }
    std::size_t size() { std::scoped_lock l(m_); return subs_.size(); }
    void publish(int v) {
        std::scoped_lock l(m_);
        for (auto& f : subs_) f(v);   // unknown code runs with m_ held
    }
private:
    std::mutex m_;
    std::vector<std::function<void(int)>> subs_;
};
```

```cpp
#include <functional>
#include <mutex>
#include <vector>
// GOOD: take a copy of the listeners under the lock, then call them with no lock held.
class Publisher {
public:
    void subscribe(std::function<void(int)> f) { std::scoped_lock l(m_); subs_.push_back(std::move(f)); }
    std::size_t size() { std::scoped_lock l(m_); return subs_.size(); }
    void publish(int v) {
        std::vector<std::function<void(int)>> snapshot;
        {
            std::scoped_lock l(m_);
            snapshot = subs_;
        }
        for (auto& f : snapshot) f(v);
    }
private:
    std::mutex m_;
    std::vector<std::function<void(int)>> subs_;
};
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.23: Think of a joining `thread` as a scoped container

A thread that is joined before its scope ends is like a function call: it may
use pointers and references to anything that outlives it in that scope.
Locals are destroyed in reverse order of declaration, so declare the data
*before* the `std::jthread` that uses it; then the thread is joined first.

```cpp bad
#include <thread>
#include <vector>
// BAD: compiles, but `out` is declared after the thread, so it is destroyed first,
// while the thread may still be writing to it (it is joined only afterwards).
void work() {
    std::jthread t;
    std::vector<int> out;
    t = std::jthread([&out] { out.push_back(1); });
}
```

```cpp
#include <thread>
#include <vector>
// GOOD: `out` outlives the thread: the jthread is destroyed (joined) first.
std::vector<int> work() {
    std::vector<int> out;
    {
        std::jthread t([&out] { out.push_back(1); });
    }   // joined here
    return out;
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.24: Think of a `thread` as a global container

If a thread might be detached, assume it outlives the function that started
it: it may only use objects with static storage or objects it owns. A pointer
or reference to a local is a dangling pointer waiting to happen.

```cpp bad
#include <string>
#include <thread>
void log_line(const std::string* s);
// BAD: compiles, but the detached thread can run after `msg` (a local) is destroyed.
void report() {
    std::string msg = "done";
    std::thread(log_line, &msg).detach();
}
```

```cpp
#include <memory>
#include <string>
#include <thread>
void log_line(const std::string* s);
// GOOD (when you must detach, see CP.26): the thread owns what it uses.
void report() {
    auto msg = std::make_shared<const std::string>("done");
    std::thread([msg] { log_line(msg.get()); }).detach();
}
```

## CP.25: Prefer `gsl::joining_thread` over `std::thread`

A `std::thread` that is still joinable when it's destroyed calls
`std::terminate`, so every path out of its scope has to remember `join()`.
A joining thread joins in its destructor. The standard one is C++20's
`std::jthread`, which also offers a stop token; `gsl::joining_thread` from the
GSL is the older equivalent.

```cpp bad
#include <thread>
void tick();
// BAD: compiles; if anything between the two lines throws (or returns), t is
// destroyed while joinable and std::terminate is called.
void run() {
    std::thread t(tick);
    // ... more work that might throw ...
    t.join();
}
```

```cpp
#include <thread>
void tick();
// GOOD: jthread joins in its destructor on every path out of run().
void run() {
    std::jthread t(tick);
    // ... more work that might throw ...
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.26: Don't `detach()` a thread

A detached thread can't be joined, asked to stop or checked on, and it may
still be running while static objects are destroyed at exit. Give a
long-running thread an owner instead: a `std::jthread` member of an object
whose destructor asks it to stop and joins it.

```cpp bad
#include <thread>
void heartbeat_once();
// BAD: compiles, but nobody can stop this thread, tell whether it is still alive,
// or wait for it before the program exits.
void start_heartbeat() {
    std::thread([] { for (;;) heartbeat_once(); }).detach();
}
```

```cpp
#include <stop_token>
#include <thread>
void heartbeat_once();
// GOOD: the Heartbeat object owns the thread; destroying it requests stop and joins.
class Heartbeat {
public:
    Heartbeat() : t_([](std::stop_token st) { while (!st.stop_requested()) heartbeat_once(); }) {}
private:
    std::jthread t_;
};
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.31: Pass small amounts of data between threads by value, rather than by reference or pointer

A copy belongs to the thread that received it, so there is nothing to lock
and nothing to dangle. `std::thread`, `std::jthread` and `std::async` copy
their arguments for exactly this reason, and refuse to bind them to a
non-`const` reference: sharing has to be spelled out with `std::ref`.

```cpp compile-fail="invocable"
#include <thread>
#include <vector>
void fill(std::vector<int>& out);
void start() {
    std::vector<int> results;
    std::jthread t(fill, results);   // BAD: error: the thread gets a copy, which can't bind to vector&
}
```

```cpp
#include <future>
#include <string>
std::string shout(std::string s) { for (char& c : s) c = static_cast<char>(c & ~0x20); return s; }
// GOOD: the argument is copied into the task; the result comes back by value.
std::string start(const std::string& word) {
    auto f = std::async(std::launch::async, shout, word);
    return f.get();
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.32: To share ownership between unrelated `thread`s use `shared_ptr`

When two threads that don't know about each other's lifetimes both use a heap
object, neither can safely delete it. Give each a `std::shared_ptr` copy: the
reference count is updated atomically, and the last owner to let go deletes
the object.

```cpp bad
#include <thread>
struct Config { int level = 1; };
void apply(const Config& c);
// BAD: compiles, but which thread deletes cfg? This one may delete it while the
// other is still reading it.
void start() {
    Config* cfg = new Config;
    std::thread([cfg] { apply(*cfg); }).detach();
    std::thread([cfg] { apply(*cfg); delete cfg; }).detach();
}
```

```cpp
#include <memory>
#include <thread>
struct Config { int level = 1; };
void apply(const Config& c);
// GOOD: each thread holds a shared_ptr; the Config is deleted when the last one ends.
void start() {
    auto cfg = std::make_shared<const Config>();
    std::jthread a([cfg] { apply(*cfg); });
    std::jthread b([cfg] { apply(*cfg); });
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.40: Minimize context switching

Every time a thread blocks and another runs, the CPU saves and restores state
and loses warm caches. Handing work over one tiny item at a time, waking the
consumer for each, spends more on switching than on the work; hand it over in
batches.

```cpp bad
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>
std::mutex m;
std::condition_variable cv;
std::deque<int> queue;
// BAD: compiles, and is correct, but every item locks, wakes the consumer and
// probably costs a context switch.
void produce(const std::vector<int>& items) {
    for (int x : items) {
        { std::scoped_lock l(m); queue.push_back(x); }
        cv.notify_one();
    }
}
```

```cpp
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>
std::mutex m;
std::condition_variable cv;
std::deque<int> queue;
// GOOD: one lock and one wake-up for the whole batch.
void produce(const std::vector<int>& items) {
    {
        std::scoped_lock l(m);
        queue.insert(queue.end(), items.begin(), items.end());
    }
    cv.notify_one();
}
```

## CP.41: Minimize thread creation and destruction

Starting a thread costs an OS call, a stack and scheduling, often far more
than a small piece of work. Start a few workers once and give them many
items (or use a thread pool) instead of a thread per item.

```cpp bad
#include <thread>
#include <vector>
void handle(int request);
// BAD: compiles, but creates and destroys one thread per request.
void serve(const std::vector<int>& requests) {
    for (int r : requests) std::jthread(handle, r);
}
```

```cpp
#include <atomic>
#include <thread>
#include <vector>
void handle(int request);
// GOOD: a fixed set of workers share the requests through an atomic index.
void serve(const std::vector<int>& requests) {
    std::atomic<std::size_t> next{0};
    auto worker = [&] {
        for (std::size_t i = 0; (i = next.fetch_add(1)) < requests.size();) handle(requests[i]);
    };
    std::vector<std::jthread> pool;
    for (unsigned n = 0; n < 4; ++n) pool.emplace_back(worker);
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.42: Don't `wait` without a condition

A condition variable doesn't remember notifications: one sent before you
wait is lost, and `wait` may also return for no reason (a spurious wake-up).
Always wait for a condition on shared state, with the overload that takes a
predicate.

```cpp bad
#include <condition_variable>
#include <mutex>
std::mutex m;
std::condition_variable cv;
bool ready = false;
// BAD: compiles; if notify_one() ran before this wait, it blocks forever, and a
// spurious wake-up returns while ready is still false.
void wait_for_ready() {
    std::unique_lock lock(m);
    cv.wait(lock);
}
```

```cpp
#include <condition_variable>
#include <mutex>
std::mutex m;
std::condition_variable cv;
bool ready = false;
// GOOD: returns at once if ready is already true, and re-checks after every wake-up.
void wait_for_ready() {
    std::unique_lock lock(m);
    cv.wait(lock, [] { return ready; });
}
void set_ready() {
    { std::scoped_lock l(m); ready = true; }
    cv.notify_one();
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.43: Minimize time spent in a critical section

While one thread holds a lock, every other thread that wants it waits. Do the
work that doesn't touch shared data (formatting, I/O, heavy computation)
outside the lock, and wrap only the shared access in a block with its own
guard.

```cpp bad
#include <mutex>
#include <string>
#include <vector>
std::mutex m;
std::vector<std::string> log_lines;
std::string format_entry(int id);   // slow, touches no shared data
// BAD: compiles, but the slow formatting runs with the lock held.
void record(int id) {
    std::scoped_lock lock(m);
    std::string line = format_entry(id);
    log_lines.push_back(std::move(line));
}
```

```cpp
#include <mutex>
#include <string>
#include <vector>
std::mutex m;
std::vector<std::string> log_lines;
std::string format_entry(int id);
// GOOD: format first, then lock only for the push_back.
void record(int id) {
    std::string line = format_entry(id);
    {
        std::scoped_lock lock(m);
        log_lines.push_back(std::move(line));
    }
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.44: Remember to name your `lock_guard`s and `unique_lock`s

A lock object with no name is a temporary: it locks and unlocks within the
same statement, and what follows runs unprotected. Worse, `T(x);` with
parentheses declares a variable named `x`. Here the compiler catches it,
because `m` is already a parameter:

```cpp compile-fail="shadows a parameter|redefinition of"
#include <mutex>
void bump(std::mutex& m, int& counter) {
    std::unique_lock<std::mutex>(m);   // BAD: declares a new `m`, doesn't lock the parameter
    ++counter;
}
```

The temporary form compiles, and protects nothing. Recent standard libraries
mark the lock constructors `[[nodiscard]]`, so GCC 15 with libstdc++ and
Clang 22 with libc++ at least warn about it with `-Wall`:

```cpp bad
#include <mutex>
std::mutex m;
int counter = 0;
void bump() {
    std::lock_guard<std::mutex>{m};   // BAD: locked and unlocked right here
    ++counter;                        // runs with no lock held: a data race
}
```

```cpp
#include <mutex>
std::mutex m;
int counter = 0;
void bump() {
    std::lock_guard<std::mutex> lock{m};   // GOOD: held until the end of the function
    ++counter;
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.50: Define a `mutex` together with the data it guards. Use `synchronized_value<T>` where possible

A mutex declared far from the data it protects invites the wrong lock or no
lock at all. Keep them together, and better, make the data reachable only
through the lock. `synchronized_value<T>` (proposed in P0290, not yet
standard) does that; a small wrapper with a `with()` function gives the same
guarantee today.

```cpp bad
#include <map>
#include <mutex>
#include <string>
std::mutex stats_mutex;                  // which data does this protect? Nothing says.
std::map<std::string, int> hits;
// BAD: compiles, but nothing stops a function from touching `hits` without the lock.
void count(const std::string& page) { ++hits[page]; }
```

```cpp
#include <map>
#include <mutex>
#include <string>
#include <utility>
// GOOD: the data is private and only reachable while its own mutex is held.
template <class T>
class Synchronized {
public:
    template <class F>
    decltype(auto) with(F&& f) {
        std::scoped_lock lock(m_);
        return std::forward<F>(f)(value_);
    }
private:
    std::mutex m_;
    T value_{};
};

Synchronized<std::map<std::string, int>> hits;
void count(const std::string& page) {
    hits.with([&](auto& h) { ++h[page]; });
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.51: Do not use capturing lambdas that are coroutines

A coroutine lambda's captures live in the closure object, not in the
coroutine frame. Once the closure is gone (often right after the call), the
suspended coroutine reads freed memory when it resumes. Pass the values as
parameters instead: parameters are copied into the frame.

```cpp bad
#include <coroutine>
#include <exception>
#include <string>
#include <utility>
struct Lazy {   // a minimal coroutine type: starts suspended, the caller resumes it
    struct promise_type {
        Lazy get_return_object() { return Lazy{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    explicit Lazy(std::coroutine_handle<promise_type> h) : h_(h) {}
    Lazy(Lazy&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~Lazy() { if (h_) h_.destroy(); }
    void resume() { h_.resume(); }
    std::coroutine_handle<promise_type> h_;
};
void print(const std::string&);

Lazy make_task(std::string name) {
    auto body = [name]() -> Lazy {   // BAD: `name` lives in the closure `body`...
        co_await std::suspend_always{};
        print(name);                 // ...which was destroyed when make_task returned
    };
    return body();
}
```

```cpp
#include <coroutine>
#include <exception>
#include <string>
#include <utility>
struct Lazy {
    struct promise_type {
        Lazy get_return_object() { return Lazy{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    explicit Lazy(std::coroutine_handle<promise_type> h) : h_(h) {}
    Lazy(Lazy&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~Lazy() { if (h_) h_.destroy(); }
    void resume() { h_.resume(); }
    std::coroutine_handle<promise_type> h_;
};
void print(const std::string&);

Lazy make_task(std::string name) {
    auto body = [](std::string n) -> Lazy {   // GOOD: no captures; n is copied into the frame
        co_await std::suspend_always{};
        print(n);
    };
    return body(std::move(name));
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.52: Do not hold locks or other synchronization primitives across suspension points

A coroutine that suspends with a lock held keeps it for as long as it stays
suspended, which may be forever, and it may be resumed on another thread,
which then unlocks a mutex it doesn't own (undefined behaviour). Release the
lock before every `co_await`.

```cpp bad
#include <coroutine>
#include <exception>
#include <mutex>
#include <utility>
struct Lazy {
    struct promise_type {
        Lazy get_return_object() { return Lazy{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    explicit Lazy(std::coroutine_handle<promise_type> h) : h_(h) {}
    Lazy(Lazy&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~Lazy() { if (h_) h_.destroy(); }
    void resume() { h_.resume(); }
    std::coroutine_handle<promise_type> h_;
};
std::mutex m;
int shared_total = 0;

Lazy add_then_wait(int x) {
    std::scoped_lock lock(m);
    shared_total += x;
    co_await std::suspend_always{};   // BAD: suspended with m still locked
}
```

```cpp
#include <coroutine>
#include <exception>
#include <mutex>
#include <utility>
struct Lazy {
    struct promise_type {
        Lazy get_return_object() { return Lazy{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    explicit Lazy(std::coroutine_handle<promise_type> h) : h_(h) {}
    Lazy(Lazy&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~Lazy() { if (h_) h_.destroy(); }
    void resume() { h_.resume(); }
    std::coroutine_handle<promise_type> h_;
};
std::mutex m;
int shared_total = 0;

Lazy add_then_wait(int x) {
    {
        std::scoped_lock lock(m);
        shared_total += x;
    }                                 // GOOD: unlocked before suspending
    co_await std::suspend_always{};
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.53: Parameters to coroutines should not be passed by reference

A coroutine returns to its caller at the first suspension, and the caller's
arguments (temporaries especially) may be gone by the time it resumes. A
reference parameter then dangles; a by-value parameter is a copy in the
coroutine frame.

```cpp bad
#include <coroutine>
#include <exception>
#include <string>
#include <utility>
struct Lazy {
    struct promise_type {
        Lazy get_return_object() { return Lazy{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    explicit Lazy(std::coroutine_handle<promise_type> h) : h_(h) {}
    Lazy(Lazy&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~Lazy() { if (h_) h_.destroy(); }
    void resume() { h_.resume(); }
    std::coroutine_handle<promise_type> h_;
};
void print(const std::string&);

Lazy greet(const std::string& name) {   // BAD: a reference into the caller
    co_await std::suspend_always{};
    print(name);
}
void run() {
    Lazy t = greet(std::string("Ada"));  // the temporary dies at the end of this line
    t.resume();                          // the coroutine reads a dangling reference
}
```

```cpp
#include <coroutine>
#include <exception>
#include <string>
#include <utility>
struct Lazy {
    struct promise_type {
        Lazy get_return_object() { return Lazy{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    explicit Lazy(std::coroutine_handle<promise_type> h) : h_(h) {}
    Lazy(Lazy&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~Lazy() { if (h_) h_.destroy(); }
    void resume() { h_.resume(); }
    std::coroutine_handle<promise_type> h_;
};
void print(const std::string&);

Lazy greet(std::string name) {          // GOOD: the frame keeps its own copy
    co_await std::suspend_always{};
    print(name);
}
void run() {
    Lazy t = greet(std::string("Ada"));
    t.resume();
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.60: Use a `future` to return a value from a concurrent task

A future is the return channel of a concurrent call: `get()` waits for the
task and gives you its value, or rethrows the exception it ended with. No
shared variable, no flag and no lock to get right.

```cpp bad
#include <atomic>
#include <thread>
long checksum(const char* path);
// BAD: compiles, but it hand-builds a return channel: a shared result, a flag the
// caller must poll, and an exception from checksum() calls std::terminate.
long result = 0;
std::atomic<bool> done{false};
void start(const char* path) {
    std::thread([path] { result = checksum(path); done = true; }).detach();
}
```

```cpp
#include <future>
long checksum(const char* path);
// GOOD: get() returns the value or rethrows the task's exception.
std::future<long> start(const char* path) {
    return std::async(std::launch::async, checksum, path);
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.61: Use `async()` to spawn concurrent tasks

`std::async` starts the task and hands back its future, with no thread,
promise or packaged task to manage. Pass `std::launch::async` when the task
must really run concurrently; without it the library may defer it until
`get()`.

```cpp bad
#include <future>
#include <thread>
int score(int);
// BAD: compiles, but it manages a promise and a raw detached thread by hand; if
// score() throws, nothing calls set_exception and get() never returns.
std::future<int> start(int x) {
    std::promise<int> p;
    std::future<int> f = p.get_future();
    std::thread([p = std::move(p), x]() mutable { p.set_value(score(x)); }).detach();
    return f;
}
```

```cpp
#include <future>
int score(int);
// GOOD: one call; the value or the exception comes back through the future.
std::future<int> start(int x) {
    return std::async(std::launch::async, score, x);
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.100: Don't use lock-free programming unless you absolutely have to

Lock-free data structures are very hard to get right (the ABA problem,
memory reclamation, memory orders) and the bugs rarely show up in tests.
Unless a measurement shows the lock is the bottleneck, a mutex around a
standard container is correct and usually fast enough.

```cpp bad
#include <atomic>
struct Node { int value; Node* next; };
std::atomic<Node*> head{nullptr};
// BAD: compiles, but pop() is broken: another thread can pop and delete `old`
// between the load and the compare_exchange (use after free), and if a node
// with the same address is pushed back meanwhile, the CAS succeeds wrongly (ABA).
bool pop(int& out) {
    Node* old = head.load();
    while (old && !head.compare_exchange_weak(old, old->next)) {}
    if (!old) return false;
    out = old->value;
    delete old;
    return true;
}
```

```cpp
#include <mutex>
#include <optional>
#include <vector>
// GOOD: a mutex-protected stack: obviously correct, and fast enough until measured otherwise.
class Stack {
public:
    void push(int v) { std::scoped_lock l(m_); items_.push_back(v); }
    std::optional<int> pop() {
        std::scoped_lock l(m_);
        if (items_.empty()) return std::nullopt;
        int v = items_.back();
        items_.pop_back();
        return v;
    }
private:
    std::mutex m_;
    std::vector<int> items_;
};
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.101: Distrust your hardware/compiler combination

Code with weak memory orders can pass every test on one machine and fail on
another: x86 keeps most stores in order, ARM doesn't, and optimizers reorder
too. Use the orders the C++ memory model requires (here release and acquire),
not what happened to work, and re-test on every new platform.

```cpp bad
#include <atomic>
int data = 0;
std::atomic<bool> ready{false};
// BAD: compiles, and usually works on x86, but relaxed gives no ordering: on ARM
// the reader can see ready == true and still read the old `data` (a data race).
void publish() { data = 42; ready.store(true, std::memory_order_relaxed); }
int consume() {
    while (!ready.load(std::memory_order_relaxed)) {}
    return data;
}
```

```cpp
#include <atomic>
int data = 0;
std::atomic<bool> ready{false};
// GOOD: the release store and acquire load make the write to data visible to the reader.
void publish() { data = 42; ready.store(true, std::memory_order_release); }
int consume() {
    while (!ready.load(std::memory_order_acquire)) {}
    return data;
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.102: Carefully study the literature

*No example: the rule is a reading list (memory models, lock-free algorithms, the ABA problem) to study before writing lock-free code, not a coding pattern.*

## CP.110: Do not write your own double-checked locking for initialization

Lazy one-time initialization is built in: a function-local `static` is
initialized exactly once even when several threads reach it at the same time
(since C++11), and `std::call_once` covers the rest. A hand-written "check,
lock, check again" with a plain pointer is a data race.

```cpp bad
#include <mutex>
struct Registry { /* expensive to build */ };
Registry* instance = nullptr;
std::mutex init_mutex;
// BAD: compiles, but the first, unlocked read of `instance` races with the write,
// and a reader can see the pointer before the Registry it points to is built.
Registry& registry() {
    if (!instance) {
        std::scoped_lock l(init_mutex);
        if (!instance) instance = new Registry;
    }
    return *instance;
}
```

```cpp
struct Registry { /* expensive to build */ };
// GOOD: the language initializes it once, thread-safely, on first use.
Registry& registry() {
    static Registry instance;
    return instance;
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.111: Use a conventional pattern if you really need double-checked locking

When you do need "check cheaply, then lock and check again" (for something
other than initialization, say a flag that's set again later), use the known
correct form: an atomic flag read outside the lock, a mutex, and a second
check under it. A `volatile` or plain `bool` flag makes the first check a
data race.

```cpp bad
#include <mutex>
std::mutex flush_mutex;
volatile bool needs_flush = false;   // BAD: volatile doesn't make the unlocked read safe
void flush_now();
void maybe_flush() {
    if (needs_flush) {
        std::scoped_lock l(flush_mutex);
        if (needs_flush) { flush_now(); needs_flush = false; }
    }
}
```

```cpp
#include <atomic>
#include <mutex>
std::mutex flush_mutex;
std::atomic<bool> needs_flush{false};   // GOOD: the unlocked first check is an atomic load
void flush_now();
void maybe_flush() {
    if (needs_flush.load(std::memory_order_acquire)) {
        std::scoped_lock l(flush_mutex);
        if (needs_flush.load(std::memory_order_relaxed)) {
            flush_now();
            needs_flush.store(false, std::memory_order_release);
        }
    }
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).

## CP.200: Use `volatile` only to talk to non-C++ memory

`volatile` is for memory that something outside the C++ program reads or
writes: a hardware register, memory shared with a device. Every access really
happens, in order. On an ordinary variable it only makes the code slower and
suggests a thread-safety it doesn't give.

```cpp bad
// BAD: compiles, but a volatile local can't be seen by hardware or another program;
// it isn't atomic either, so it fixes nothing (CP.8).
int sum(const int* p, int n) {
    volatile int total = 0;
    for (int i = 0; i < n; ++i) total = total + p[i];
    return total;
}
```

```cpp
#include <cstdint>
// GOOD: a memory-mapped status register (the address is the device's), read every time.
inline volatile const std::uint32_t* const status_register =
    reinterpret_cast<volatile const std::uint32_t*>(0x4000'1000);

inline bool device_busy() { return (*status_register & 0x1u) != 0; }
```

## CP.201: ??? Signals

The official rule has no text yet; its note points out how little is safe
inside a signal handler. A handler may interrupt any code, including one in
the middle of `malloc` or holding a stream's lock, so calling those from it
can deadlock or corrupt state. Have it set a `volatile std::sig_atomic_t` (or
lock-free atomic) flag, and act on the flag in normal code.

```cpp bad
#include <csignal>
#include <iostream>
// BAD: compiles, but iostreams (and malloc, printf, mutexes) aren't safe in a signal handler.
extern "C" void on_interrupt(int) {
    std::cout << "interrupted, cleaning up\n";
}
```

```cpp
#include <csignal>
#include <iostream>
// GOOD: the handler only records the signal; the main loop reacts to it.
volatile std::sig_atomic_t interrupted = 0;
extern "C" void on_interrupt(int) { interrupted = 1; }

void main_loop() {
    std::signal(SIGINT, on_interrupt);
    while (!interrupted) { /* ... do work ... */ }
    std::cout << "interrupted, cleaning up\n";
}
```

Tested by [`tests/guidelines/concurrency_test.cpp`](../../../../tests/guidelines/concurrency_test.cpp).
