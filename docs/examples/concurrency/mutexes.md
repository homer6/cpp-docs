# Mutexes & condition variables

> Protect shared data with mutual exclusion (always via an RAII lock), and let
> threads wait for conditions. **Headers:** `<mutex>`, `<condition_variable>` ·
> **Since:** C++11

## Lock with RAII, never by hand

Don't call `mutex.lock()`/`unlock()` directly — an exception or early return
leaks the lock. Use a guard:

```cpp
#include <mutex>

std::mutex m;
int shared = 0;

void increment() {
    std::lock_guard<std::mutex> lk{m};       // locks now, unlocks on scope exit
    ++shared;                                 // critical section
}                                             // unlocked here, even if it threw
```

## The lock types

| Guard | Use |
|---|---|
| `std::lock_guard` | simplest scoped lock; no extra features |
| `std::scoped_lock` (C++17) | lock **multiple** mutexes deadlock-free; the modern default |
| `std::unique_lock` | movable, deferrable, unlock/relock — needed for condition variables |
| `std::shared_lock` (C++14) | shared (reader) lock for `std::shared_mutex` |

```cpp
std::mutex m1, m2;

void transfer() {
    std::scoped_lock lk{m1, m2};              // locks both, avoids deadlock (C++17)
}
```

## Mutex flavors

`std::mutex`, `std::recursive_mutex` (re-lockable by the same thread),
`std::timed_mutex` (`try_lock_for`), and `std::shared_mutex` (many readers **or**
one writer):

```cpp
std::shared_mutex sm;
void reader() { std::shared_lock lk{sm}; /* read  */ }   // multiple readers concurrently
void writer() { std::unique_lock lk{sm}; /* write */ }   // exclusive writer
```

## Condition variables — wait for a state change

```cpp
#include <condition_variable>
#include <queue>

std::mutex m;
std::condition_variable cv;
std::queue<int> q;

void producer(int x) {
    { std::lock_guard lk{m}; q.push(x); }
    cv.notify_one();                          // wake a waiter
}

void consumer() {
    std::unique_lock lk{m};
    cv.wait(lk, [&]{ return !q.empty(); });   // releases lock while waiting; predicate guards spurious wakeups
    int x = q.front(); q.pop();
}
```

## Run something exactly once — `std::call_once`

`std::call_once(flag, f, args...)` runs `f` once per `std::once_flag`, however
many threads arrive at the same time. The others block until it finishes, then
return without calling `f`. When the thing to set up is a single object,
a **function-local static** does the same job with less code: since C++11 its
initialization is thread-safe ("magic statics"). Reach for `call_once` when the
"once" belongs to an object rather than the program (a `once_flag` member), or
when the setup isn't the construction of one variable.

```cpp
#include <mutex>
#include <string_view>

struct Config { int port = 8080; };

Config& config() {
    static Config c;                       // initialized once, thread-safe: prefer this
    return c;
}

class Connection {
    std::once_flag opened_;                // one flag per object: a static can't do this
    void open();                           // expensive, must run once per Connection
public:
    void send(std::string_view msg) {
        std::call_once(opened_, &Connection::open, this);  // first caller opens; the rest wait, then skip
        // ... write msg ...
    }
};
```

If `f` throws, the flag stays unset, the exception reaches the caller, and the
next `call_once` tries again. A static's initializer is retried the same way.

## Gotchas

- **`call_once` with a throwing callable is broken in libstdc++ on some
  targets** ([GCC PR 66146](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66146)).
  With GCC 15 on macOS the program died instead of retrying, while GCC 16 on
  Linux and Clang with libc++ retried. Don't rely on the retry with libstdc++
  off Linux.
- **A `once_flag` can't be reset, copied or moved.** For "once per object",
  make it a member; a class holding one isn't copyable unless you write the
  copy yourself.
- **Always use an RAII lock**, never raw `lock()`/`unlock()`. And to lock several
  mutexes, use `std::scoped_lock` — locking them one-by-one invites deadlock.
- **Always pass a predicate to `cv.wait`.** The predicate form loops internally,
  defeating **spurious wakeups** (a wait can return without a notify). Bare
  `wait(lk)` is almost always a bug.
- **Notify while or after holding the lock, consistently;** the waiter must
  re-check the shared state under the lock (which the predicate does).
- **Condition variables need `unique_lock`** (they unlock/relock), not
  `lock_guard`. Use `condition_variable_any` to wait on other lockables (and with
  `stop_token`).
- **A mutex protects data, not code.** Every access to the shared data — reads
  included — must hold the same mutex.

## See also

- [std::mutex — cppreference](https://en.cppreference.com/w/cpp/thread/mutex)
- [std::scoped_lock — cppreference](https://en.cppreference.com/w/cpp/thread/scoped_lock)
- [std::condition_variable — cppreference](https://en.cppreference.com/w/cpp/thread/condition_variable)
- [std::call_once — cppreference](https://en.cppreference.com/w/cpp/thread/call_once) ·
  [std::once_flag](https://en.cppreference.com/w/cpp/thread/once_flag) ·
  [static local variables](https://en.cppreference.com/w/cpp/language/storage_duration#Static_local_variables)
- [GCC PR 66146: call_once with an exception](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66146)
