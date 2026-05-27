# Latches, barriers & semaphores (C++20)

> Lightweight primitives for coordinating groups of threads.
> **Headers:** `<latch>`, `<barrier>`, `<semaphore>` · **Since:** C++20

## std::latch — a one-shot countdown

A counter that threads decrement; waiters block until it hits zero. **Single
use** — once it reaches zero it stays there.

```cpp
#include <latch>

std::latch done{3};                  // expect 3 arrivals

for (int i = 0; i < 3; ++i)
    std::jthread{[&done]{ do_work(); done.count_down(); }};

done.wait();                         // blocks until all 3 counted down
// also: done.arrive_and_wait();  (count down, then wait)
```

Great for "wait until N workers have finished startup / a phase."

## std::barrier — a reusable rendezvous

Like a latch, but **reusable across phases**: threads `arrive_and_wait()`, and
when all have arrived, an optional completion function runs and the barrier resets
for the next phase.

```cpp
#include <barrier>

std::barrier sync{4, []{ /* runs once per phase when all 4 arrive */ }};

void worker() {
    for (int phase = 0; phase < N; ++phase) {
        compute_phase();
        sync.arrive_and_wait();      // wait for all 4, then proceed together
    }
}
```

Ideal for lock-step parallel algorithms (each phase depends on the previous).

## std::counting_semaphore — a permit counter

Holds a count of available "permits"; `acquire()` takes one (blocking if none),
`release()` returns one. `std::binary_semaphore` is the count-1 special case.

```cpp
#include <semaphore>

std::counting_semaphore<8> slots{4};     // max 8, start with 4 permits

void use_resource() {
    slots.acquire();                     // wait for a permit
    // ... use one of 4 limited resources ...
    slots.release();                     // give it back
}

std::binary_semaphore signal{0};         // like a lightweight one-shot event
signal.release();                         // from one thread
signal.acquire();                         // wakes another
```

## Gotchas

- **Latch is one-shot; barrier is reusable.** Reaching for a latch in a loop is
  the wrong tool — use a barrier for repeated phase synchronization.
- **A semaphore is not a mutex.** It has no ownership concept — any thread can
  `release()`, including one that never acquired. Don't use it for mutual
  exclusion where a mutex's ownership semantics matter.
- **Count mismatches hang.** A latch initialized to N that only gets N-1
  `count_down()`s blocks forever; same for a barrier expecting more arrivals than
  show up.
- **These are C++20** — older toolchains may lack them; a `condition_variable`
  can emulate them if needed.

## See also

- [std::latch — cppreference](https://en.cppreference.com/w/cpp/thread/latch)
- [std::barrier — cppreference](https://en.cppreference.com/w/cpp/thread/barrier)
- [std::counting_semaphore — cppreference](https://en.cppreference.com/w/cpp/thread/counting_semaphore)
