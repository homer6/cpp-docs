# std::thread / std::jthread

> Run a function on another thread. `std::jthread` (C++20) adds automatic joining
> and cooperative cancellation. **Headers:** `<thread>` · **Since:** C++11 / C++20

## std::thread — the original

Starts running immediately on construction; you **must** `join()` (wait for it) or
`detach()` it before it's destroyed, or the program calls `std::terminate`.

```cpp
#include <thread>

std::thread t{[]{ do_work(); }};
std::thread t2{worker_fn, arg1, arg2};      // args are copied into the thread
// ... 
t.join();                                    // wait for it to finish
// (or t.detach() to let it run independently — rarely what you want)
```

## std::jthread — prefer this (C++20)

`std::jthread` **joins automatically in its destructor** (RAII), and supports a
**stop token** for cooperative cancellation:

```cpp
#include <thread>

void worker(std::stop_token st) {
    while (!st.stop_requested()) { /* do a chunk of work */ }
}

std::jthread jt{worker};          // a stop_token is passed in automatically
// ...
jt.request_stop();                // ask it to stop
// no explicit join needed — destructor requests stop AND joins
```

If your callable's first parameter is a `std::stop_token`, `jthread` supplies one.

## this_thread

```cpp
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

std::this_thread::sleep_for(100ms);
std::this_thread::sleep_until(deadline);
std::this_thread::yield();                   // hint: let others run
std::this_thread::get_id();
unsigned n = std::thread::hardware_concurrency();   // ~number of cores (a hint)
```

## Gotchas

- **`std::thread` that's neither joined nor detached → `std::terminate`** when
  destroyed. `std::jthread` fixes this by joining in its destructor — prefer it.
- **Arguments are *copied* into the thread.** To pass a reference, wrap it in
  `std::ref` — and ensure the referent outlives the thread.
- **`detach()` is a footgun.** A detached thread touching objects that have since
  been destroyed is UB; you also lose any way to wait for it. Avoid unless you
  truly need fire-and-forget.
- **`hardware_concurrency()` is a hint** and may return 0; don't divide by it
  blindly.
- **Don't roll your own cancellation flag** — use `stop_token`/`stop_source`,
  which integrate with `jthread` and `condition_variable_any`.

## See also

- [std::thread — cppreference](https://en.cppreference.com/w/cpp/thread/thread)
- [std::jthread — cppreference](https://en.cppreference.com/w/cpp/thread/jthread)
- [std::stop_token — cppreference](https://en.cppreference.com/w/cpp/thread/stop_token)
