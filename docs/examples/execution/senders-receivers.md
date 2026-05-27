# Senders & receivers

> The C++26 model for composable async work: build a *sender* (a task graph),
> attach it to a *scheduler*, and run it. **Header:** `<execution>` ·
> **Namespace:** `std::execution`

## The four vocabulary pieces

| Concept | Role |
|---|---|
| **Sender** | a *description* of asynchronous work (lazy — does nothing until started) |
| **Receiver** | a callback with three channels: `set_value` (success), `set_error`, `set_stopped` (cancelled) |
| **Operation state** | the result of `connect(sender, receiver)` — holds the work's state; runs on `start` |
| **Scheduler** | a lightweight handle to an execution resource (thread pool, GPU, event loop) |

The flow: `connect` a sender to a receiver → get an operation state → `start` it →
it eventually calls one of the receiver's three completion channels.

## Sender factories — make work from nothing

```cpp
namespace ex = std::execution;

ex::just(42)                 // a sender that completes with the value 42
ex::just_error(err)          // completes on the error channel
ex::just_stopped()           // completes as cancelled
ex::schedule(sched)          // a sender that runs on scheduler `sched`
```

## Sender adaptors — compose with `|`

```cpp
auto work = ex::just(21)
          | ex::then([](int x){ return x * 2; })       // transform the value (→ 42)
          | ex::then([](int x){ return x + 1; });        // chain another step (→ 43)
```

Key adaptors:
```cpp
ex::then(fn)             // map the value channel
ex::upon_error(fn)       // handle the error channel
ex::upon_stopped(fn)     // handle cancellation
ex::let_value(fn)        // fn returns a new sender (dynamic continuation)
ex::when_all(s1, s2, …)  // complete when all inputs complete
ex::bulk(shape, fn)      // run fn for each index (data parallelism)
ex::on(sched, sender)    // transfer execution onto `sched`, then back
ex::continues_on(sched)  // continue subsequent work on `sched`
```

## Running it — sender consumers

```cpp
// block until done and retrieve the result:
auto [result] = std::this_thread::sync_wait(std::move(work)).value();   // 43
```

`sync_wait` (in namespace `std::this_thread`) drives the sender to completion on
the calling thread and returns the value (or rethrows the error).

## Worked example (from cppreference)

```cpp
#include <execution>
namespace ex = std::execution;

ex::run_loop loop;                                   // a simple execution resource
std::jthread worker([&](std::stop_token st){
    std::stop_callback cb{st, [&]{ loop.finish(); }};
    loop.run();
});

ex::scheduler auto io = loop.get_scheduler();
ex::sender auto work =
    ex::on(io, ex::just("hello world"s)
             | ex::then([](std::string m){ return std::puts(m.c_str()); }));

auto [rc] = std::this_thread::sync_wait(std::move(work)).value();
```

## Gotchas

- **Senders are lazy.** Nothing runs until an operation state is started (e.g. via
  `sync_wait` or `start_detached`). Building the graph has no effect by itself.
- **Three completion channels, not one.** Errors and cancellation are first-class
  (`set_error`/`set_stopped`) — handle them with `upon_error`/`upon_stopped`, not
  just exceptions.
- **Choose *where* with schedulers.** `on`/`continues_on`/`schedule` move work
  between execution resources; that explicit control is the whole point versus
  `std::async`.
- **C++26 and very new.** Standard-library support is early; the widely-used
  reference implementation is **stdexec** (experimental), which uses an
  `experimental`-flavored namespace. Expect churn.

## See also

- [Execution control library — cppreference](https://en.cppreference.com/w/cpp/execution)
- [std::execution::then — cppreference](https://en.cppreference.com/w/cpp/execution/then)
- [std::this_thread::sync_wait — cppreference](https://en.cppreference.com/w/cpp/execution/sync_wait)
