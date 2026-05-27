# Futures, promises & async

> Get a result (or exception) from another thread, later. The highest-level,
> easiest-to-use concurrency tool. **Header:** `<future>` · **Since:** C++11

## Mental model

A **future** is a handle to a value that isn't ready yet. Something produces the
value on another thread; you call `.get()` to retrieve it (blocking until ready).
Exceptions thrown by the producer are re-thrown at `.get()` — error handling just
works.

## std::async — fire off work, get a future

```cpp
#include <future>

std::future<int> f = std::async(std::launch::async, []{
    return expensive_computation();
});
// ... do other work ...
int result = f.get();            // blocks until ready; re-throws if it threw
```

`std::launch::async` forces a new thread; the default policy *may* defer to run
lazily at `get()` — pass `async` explicitly when you want real concurrency.

## promise / future — hand-built channel

```cpp
std::promise<int> p;
std::future<int> f = p.get_future();

std::thread t{[&p]{ p.set_value(42); }};   // producer sets the value (or set_exception)
int v = f.get();                            // consumer receives it
t.join();
```

## packaged_task — wrap a callable as a future

```cpp
std::packaged_task<int(int)> task{[](int x){ return x * x; }};
std::future<int> f = task.get_future();
std::thread{std::move(task), 7}.detach();   // run it somewhere
int r = f.get();                             // 49
```

## Checking without blocking

```cpp
using namespace std::chrono_literals;
if (f.wait_for(0s) == std::future_status::ready) { /* done */ }
f.wait();                                    // block until ready (no get)
```

## Gotchas

- **`std::future::get()` can be called only once** and *moves* the result out.
  Need multiple waiters? Use `std::shared_future`.
- **The default `std::async` policy may defer.** Without `std::launch::async`, the
  task can run lazily on `get()` (single-threaded) — surprising for "parallel"
  code. Be explicit.
- **The destructor of an `async`-launched future *blocks*.** A discarded
  `std::async(...)` future joins at end of scope — `std::async(...)` as a bare
  statement runs synchronously. Keep the future alive intentionally.
- **A `promise` destroyed without setting a value** makes the future throw
  `std::broken_promise` at `get()`.

## See also

- [std::future — cppreference](https://en.cppreference.com/w/cpp/thread/future)
- [std::async — cppreference](https://en.cppreference.com/w/cpp/thread/async)
- [std::promise — cppreference](https://en.cppreference.com/w/cpp/thread/promise)
- [std::packaged_task — cppreference](https://en.cppreference.com/w/cpp/thread/packaged_task)
