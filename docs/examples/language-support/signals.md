# Signal handling

> Install a function to run when the program receives a signal (interrupt,
> termination request, arithmetic fault, …), or send one to yourself.
> **Header:** `<csignal>` · **Since:** C++98 (inherited from C)

## Mental model

A signal interrupts the program between (or in the middle of) any two
instructions and runs your handler on the current thread's stack. Whatever the
interrupted code was doing — holding a lock inside `malloc`, half-way through
updating a `std::string` — is still half done when the handler starts. That is
why a handler may do almost nothing: set a flag, and let ordinary code react.

```
main loop ──────────┐   ┌──────────────▶ sees g_stop == 1, shuts down cleanly
                    │   │
         SIGINT ──▶ handler: g_stop = 1; return
```

## Declaration

```cpp nocompile
using sig_atomic_t = /* integer type */;          // safe to access from a handler
extern "C" using handler = void(int);              // what a handler looks like
handler* signal(int sig, handler* h);              // returns previous handler or SIG_ERR
int      raise(int sig);                           // send sig to the calling program; 0 on success

SIG_DFL  SIG_IGN  SIG_ERR                          // default action, ignore, error return
SIGABRT  SIGFPE  SIGILL  SIGINT  SIGSEGV  SIGTERM  // the six signals C and C++ guarantee
```

The six standard signals: `SIGINT` (interactive attention, Ctrl-C), `SIGTERM`
(termination request), `SIGABRT` (`std::abort`), `SIGFPE` (erroneous arithmetic),
`SIGILL` (illegal instruction), `SIGSEGV` (invalid memory access). Everything
else (`SIGHUP`, `SIGPIPE`, `SIGUSR1`, …) is POSIX or platform-specific.

## When to use it

- **Graceful shutdown on Ctrl-C / `SIGTERM`**: set a flag, finish the current
  unit of work, flush, exit.
- **Ignoring a signal** (`signal(SIGPIPE, SIG_IGN)` on POSIX, so a closed socket
  gives you an error code instead of killing the process).

Anti-fits: anything more than setting a flag. On POSIX prefer `sigaction`
(well-defined semantics, masks, `SA_RESTART`), or `sigwait` / `signalfd` on a
dedicated thread so signals arrive as ordinary events. Don't try to "recover"
from `SIGSEGV` or `SIGFPE`: returning from such a handler is undefined
behaviour.

## Examples

### Set a flag, react outside the handler

```cpp
#include <atomic>
#include <csignal>
#include <cstdio>

namespace {
volatile std::sig_atomic_t g_signal = 0;   // the C type for handler communication
std::atomic<int> g_count{0};               // lock-free atomics are allowed too (C++17)
static_assert(std::atomic<int>::is_always_lock_free);
}

extern "C" void on_signal(int sig) {
    g_signal = sig;
    g_count.fetch_add(1, std::memory_order_relaxed);
}

int main() {
    if (std::signal(SIGINT, on_signal) == SIG_ERR) return 1;
    std::raise(SIGINT);                    // the handler runs before raise returns
    std::printf("got signal %d, count=%d\n", int(g_signal), g_count.load());

    auto prev = std::signal(SIGTERM, SIG_IGN);
    std::raise(SIGTERM);                   // ignored: we keep running
    std::printf("SIGTERM ignored; previous handler was %s\n",
                prev == SIG_DFL ? "SIG_DFL" : "something else");
    std::signal(SIGTERM, prev);

    std::raise(SIGINT);                    // is our handler still installed?
    std::printf("second SIGINT, count=%d\n", g_count.load());
}
```

Same output with GCC 15 and Clang 22 on macOS and GCC 16 on Linux (glibc):

```
got signal 2, count=1
SIGTERM ignored; previous handler was SIG_DFL
second SIGINT, count=2
```

### A shutdown loop

```cpp
#include <chrono>
#include <csignal>
#include <thread>

namespace { volatile std::sig_atomic_t g_stop = 0; }

extern "C" void request_stop(int) { g_stop = 1; }

int main() {
    std::signal(SIGINT, request_stop);
    std::signal(SIGTERM, request_stop);
    while (!g_stop) {
        // do one bounded unit of work...
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // ...then clean up normally: destructors run, buffers are flushed
}
```

## Gotchas

- **Only a tiny set of operations is allowed in a handler.** Since C++17 a
  handler may use `volatile std::sig_atomic_t` objects, plain lock-free atomic
  operations, and the functions the standard marks *signal-safe*: `std::abort`,
  `std::_Exit`, `std::quick_exit`, `std::signal` for the signal being handled,
  `std::memcpy`, `std::memmove`, `std::move`/`std::forward`, `numeric_limits`
  members and the `<type_traits>` functions. Anything else — `printf`, `new`,
  `std::mutex`, a `throw`, entering a `try` block, `dynamic_cast`, a
  `thread_local` — is undefined behaviour. POSIX adds its own async-signal-safe
  list (`write`, `_exit`, …).
- **Whether the handler stays installed is implementation-defined.** C allows
  the implementation to reset the disposition to `SIG_DFL` before calling the
  handler (old System V behaviour). macOS and glibc keep it installed, as the
  second `SIGINT` above shows; on POSIX use `sigaction` to choose explicitly.
- **C leaves `signal` in a multithreaded program undefined**; POSIX requires it
  to be thread-safe, but which thread runs the handler is unspecified.
  Block signals in worker threads and handle them in one place.
- **`raise` is synchronous; real signals are not.** A test using `raise` cannot
  show the races a real Ctrl-C can hit.
- **Returning from a `SIGFPE`, `SIGILL` or `SIGSEGV` handler for a real fault is
  undefined**, and on most systems just re-executes the faulting instruction.
- **Handlers should have C linkage** (`extern "C"`) to match `signal`'s
  parameter type; GCC and Clang accept a C++-linkage function too.
- **Destructors don't run** when the default action terminates the process.

## See also

- [`std::signal` — cppreference](https://en.cppreference.com/w/cpp/utility/program/signal)
- [Signal handlers and signal-safe operations — cppreference](https://en.cppreference.com/w/cpp/utility/program/signal#Signal_handler)
- [`std::sig_atomic_t` — cppreference](https://en.cppreference.com/w/cpp/utility/program/sig_atomic_t)
- [POSIX `sigaction`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/sigaction.html)
- [Non-local jumps](non-local-jumps.md) — the other C-era control-flow escape hatch
