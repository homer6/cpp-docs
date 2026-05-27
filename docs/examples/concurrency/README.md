# Concurrency support library

Threads, the tools to coordinate them safely, and lock-free atomics. Mirrors
[cppreference's thread support library](https://en.cppreference.com/w/cpp/thread).

| Page | Header | Since | What |
|---|---|---|---|
| [Threads](threads.md) | `<thread>` | C++11 / C++20 | `std::thread`, `std::jthread`, `this_thread` |
| [Mutexes & condition variables](mutexes.md) | `<mutex>`, `<condition_variable>` | C++11 | locking, RAII guards, waiting |
| [Atomics](atomics.md) | `<atomic>` | C++11 / C++20 | lock-free ops, memory order, `atomic_ref` |
| [Futures](futures.md) | `<future>` | C++11 | `async`, `promise`/`future`, `packaged_task` |
| [Latches, barriers, semaphores](coordination.md) | `<latch>`, `<barrier>`, `<semaphore>` | C++20 | thread coordination primitives |

## The mental hierarchy (high → low level)

1. **`std::async` / futures** — fire off work, get a result later. Start here.
2. **`std::jthread` + RAII locks** — when you manage threads explicitly.
3. **Latches / barriers / semaphores** — structured coordination of many threads.
4. **`std::atomic`** — lock-free shared state; powerful and easy to get subtly
   wrong. Reach for it last, with care.

> The unifying rule: **a data race is undefined behavior.** Any data touched by
> multiple threads where at least one writes must be protected (mutex) or atomic.
