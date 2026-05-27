# std::atomic

> Lock-free (usually) operations on shared data, with explicit control over memory
> ordering. The lowest level — powerful and subtle. **Header:** `<atomic>` ·
> **Since:** C++11

## Mental model

`std::atomic<T>` makes reads, writes, and read-modify-write operations on a `T`
**indivisible** — no torn values, no data race — without a mutex. It also lets you
specify how operations on *other* memory are ordered relative to the atomic op
(the memory order).

```cpp
#include <atomic>

std::atomic<int> counter{0};
counter.fetch_add(1);            // atomic ++, returns old value
++counter;                       // also atomic
int v = counter.load();          // atomic read
counter.store(5);                // atomic write
```

## Read-modify-write & CAS

```cpp
std::atomic<int> x{0};
x.fetch_add(3);  x.fetch_sub(1);  x.fetch_or(0b10);     // atomic arithmetic/bitwise

int expected = 0;
x.compare_exchange_strong(expected, 1);   // if x==expected, set x=1 (CAS); else load x into expected
// the building block of lock-free algorithms — loop on the weak form:
while (!x.compare_exchange_weak(expected, expected + 1)) { /* expected updated */ }
```

## Memory order

Each atomic op takes an optional `std::memory_order`. Default is
`seq_cst` (sequentially consistent — simplest to reason about, strongest):

| Order | Meaning |
|---|---|
| `seq_cst` | global total order; the safe default |
| `acquire` / `release` | pair up to publish/consume data (release a write, acquire to see it) |
| `relaxed` | atomicity only, **no** ordering of surrounding memory |

```cpp
std::atomic<bool> ready{false};
data = produce();
ready.store(true, std::memory_order_release);            // publish
// other thread:
if (ready.load(std::memory_order_acquire)) use(data);    // sees `data`
```

## atomic_ref and atomic<shared_ptr> (C++20)

```cpp
int plain = 0;
std::atomic_ref<int> ar{plain};      // atomic ops on a non-atomic object (e.g. an array slot)
ar.fetch_add(1);

std::atomic<std::shared_ptr<Node>> head;   // lock-free-ish atomic smart pointer (C++20)
```

## Gotchas

- **Start with the default `seq_cst`** (or just a mutex). Hand-tuning to
  `acquire`/`release`/`relaxed` is an expert optimization; getting it wrong is a
  Heisenbug that "works on my machine." Profile first.
- **Atomic ≠ a lock.** A *sequence* of atomic ops isn't atomic as a whole — use
  CAS loops or a mutex for compound invariants.
- **Not always lock-free.** `std::atomic<BigStruct>` may use a hidden lock; check
  `is_lock_free()` / `is_always_lock_free` if it matters.
- **`compare_exchange_weak` can fail spuriously** (by design, for efficiency) —
  always use it in a loop; use `_strong` for one-shot checks.
- **`volatile` is not for threading.** It does not provide atomicity or ordering;
  use `std::atomic`.

## See also

- [std::atomic — cppreference](https://en.cppreference.com/w/cpp/atomic/atomic)
- [std::memory_order — cppreference](https://en.cppreference.com/w/cpp/atomic/memory_order)
- [std::atomic_ref — cppreference](https://en.cppreference.com/w/cpp/atomic/atomic_ref)
