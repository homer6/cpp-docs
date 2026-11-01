# std::atomic

> Lock-free (usually) operations on shared data, with explicit control over memory
> ordering. The lowest level — powerful and subtle. **Header:** `<atomic>` ·
> **Since:** C++11

## Mental model

`std::atomic<T>` makes reads, writes, and read-modify-write operations on a `T`
**indivisible** — no torn values, no data race — without a mutex. It also lets you
specify how operations on *other* memory are ordered relative to the atomic op
(the memory order).

```cpp body
std::atomic<int> counter{0};     // #include <atomic>
counter.fetch_add(1);            // atomic ++, returns old value
++counter;                       // also atomic
int v = counter.load();          // atomic read
counter.store(5);                // atomic write
```

## Read-modify-write & CAS

```cpp body
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
int produce();
int data = 0;                                            // plain, non-atomic
std::atomic<bool> ready{false};

void producer() {                                        // one thread
    data = produce();
    ready.store(true, std::memory_order_release);        // publish
}
void consumer() {                                        // other thread
    if (ready.load(std::memory_order_acquire)) use(data); // sees `data`
}
```

## atomic_ref and atomic<shared_ptr> (C++20)

```cpp body
struct Node { int value; };

int plain = 0;
std::atomic_ref<int> ar{plain};      // atomic ops on a non-atomic object (e.g. an array slot)
ar.fetch_add(1);

std::atomic<std::shared_ptr<Node>> head;   // lock-free-ish atomic smart pointer (C++20)
```

libc++ (Clang 22) doesn't provide `std::atomic<std::shared_ptr>` yet
(`__cpp_lib_atomic_shared_ptr` undefined); libstdc++ has it since GCC 12.

## wait and notify (C++20)

`a.wait(old)` blocks while `a` still holds `old`, and returns once it holds
anything else. `notify_one()` / `notify_all()` wake threads blocked in `wait`.
It's a futex-style wait on the atomic itself: no mutex, no condition variable,
no spinning (`atomic_flag` and `atomic_ref` have the same three members).

```cpp
#include <atomic>

std::atomic<int> state{0};

void worker() {
    state.wait(0);                 // sleeps while state == 0; returns at once if it's already changed
    // state != 0 here
}

void start() {
    state.store(1);                // change the value first...
    state.notify_all();            // ...then wake the waiters (notify_one wakes just one)
}
```

`wait` re-checks the value after every wakeup, so spurious wakeups are handled
for you. It compares the value, not a predicate: if the value changes and changes
back before the waiter looks, the waiter keeps sleeping (ABA).

## fetch_max and fetch_min (C++26)

Atomically store the larger (or smaller) of the current value and the argument,
and return the old value, with no CAS loop. They exist for integral types (not
`bool`) and object pointers, on `std::atomic` and `std::atomic_ref`, plus the free
functions `std::atomic_fetch_max` / `_min` (P0493, `__cpp_lib_atomic_min_max`
`202403L`). GCC 16 has them; GCC 15 and Clang 22 (libc++) don't.

```cpp needs=atomic_min_max
#include <atomic>

std::atomic<int> peak{3};

void record(int sample) {
    peak.fetch_max(sample);        // peak = max(peak, sample), atomically
}
// peak.fetch_max(7) returns 3 and leaves 7; a later fetch_max(5) returns 7 and leaves 7
```

Before C++26, write the CAS loop: `while (v > cur && !a.compare_exchange_weak(cur, v)) {}`.

## False sharing and interference sizes (C++17)

Two atomics that sit on the same cache line make their threads fight over that
line, even though they never touch each other's data: **false sharing**.
`<new>` gives two constants. `std::hardware_destructive_interference_size` is
the minimum offset that keeps two objects from sharing a line, and
`std::hardware_constructive_interference_size` is the most memory you can expect
to share one line (and be fetched together).

```cpp
#include <atomic>
#include <new>

struct Counters {                  // one thread bumps produced, another consumed
    alignas(std::hardware_destructive_interference_size) std::atomic<long> produced{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<long> consumed{0};
};
static_assert(sizeof(Counters) >= 2 * std::hardware_destructive_interference_size);

struct Hot { int key; int value; };     // read together, so keep them on one line
static_assert(sizeof(Hot) <= std::hardware_constructive_interference_size);
```

GCC 15, GCC 16 and Clang 22 all define them (`__cpp_lib_hardware_interference_size`
`201703L`). The values depend on the target: 64/64 with GCC on x86-64, and
256 (destructive) / 64 (constructive) on AArch64 with all three toolchains.
**GCC warns** (`-Winterference-size`, on by default) when a header uses them,
because the value can change with the compiler version or `-mtune`/`-mcpu`, so a
struct laid out with it isn't a stable ABI. Clang doesn't warn. In a public
header, define your own constant (`inline constexpr std::size_t cache_line = 64;`),
or pin GCC's value with `--param destructive-interference-size=64`.

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
- **`notify` without a store wakes nobody for good.** The waiter re-checks the
  value, sees it unchanged, and sleeps again. Store the new value, then notify.
- **`wait` isn't a condition variable.** It wakes on "value differs from `old`",
  can miss an A→B→A change, and takes no predicate. Use a condition variable
  for conditions over several variables.
- **Don't put `hardware_destructive_interference_size` in a public ABI.** It
  varies between targets and compiler flags; GCC warns about it in headers.

## See also

- [std::atomic — cppreference](https://en.cppreference.com/w/cpp/atomic/atomic)
- [std::memory_order — cppreference](https://en.cppreference.com/w/cpp/atomic/memory_order)
- [std::atomic_ref — cppreference](https://en.cppreference.com/w/cpp/atomic/atomic_ref)
- [std::atomic::wait — cppreference](https://en.cppreference.com/w/cpp/atomic/atomic/wait) ·
  [notify_one](https://en.cppreference.com/w/cpp/atomic/atomic/notify_one) ·
  [notify_all](https://en.cppreference.com/w/cpp/atomic/atomic/notify_all)
- [std::atomic::fetch_max — cppreference](https://en.cppreference.com/w/cpp/atomic/atomic/fetch_max) ·
  [P0493R5: Atomic minimum/maximum](https://wg21.link/P0493R5)
- [std::hardware_destructive_interference_size — cppreference](https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size)
- [GCC `-Winterference-size`](https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Dialect-Options.html#index-Winterference-size)
