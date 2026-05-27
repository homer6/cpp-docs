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
std::scoped_lock lk{m1, m2};                  // locks both, avoids deadlock (C++17)
```

## Mutex flavors

`std::mutex`, `std::recursive_mutex` (re-lockable by the same thread),
`std::timed_mutex` (`try_lock_for`), and `std::shared_mutex` (many readers **or**
one writer):

```cpp
std::shared_mutex sm;
std::shared_lock read{sm};        // multiple readers concurrently
std::unique_lock write{sm};       // exclusive writer
```

## Condition variables — wait for a state change

```cpp
#include <condition_variable>

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

## Gotchas

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
