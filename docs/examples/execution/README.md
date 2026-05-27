# Execution control library (C++26)

> A framework for **structured asynchronous programming**: describe work as
> composable *senders*, then run it on a *scheduler*. **Header:** `<execution>` ·
> **Namespace:** `std::execution` · **Since:** C++26

Mirrors [cppreference's execution control library](https://en.cppreference.com/w/cpp/execution).

| Page | What |
|---|---|
| [Senders & receivers](senders-receivers.md) | the model, factories, adaptors, and running work |

## Why it exists

`std::async`/futures don't compose well: chaining, cancellation, and choosing
*where* work runs are awkward. The execution library (P2300) gives a uniform
vocabulary — **sender** (a description of async work), **receiver** (a callback
with success/error/stopped channels), **scheduler** (a handle to an execution
resource like a thread pool or GPU stream) — so you can build and compose task
graphs that run on any backend.

> Brand-new in C++26 and large; this area introduces the model and the common
> algorithms rather than every customization point.
