# Safe reclamation: RCU and hazard pointers

> Free memory that lock-free readers might still be looking at — only once no
> reader can reach it. **Headers:** `<rcu>`, `<hazard_pointer>` · **Since:** C++26

## Mental model

A lock-free structure publishes data through an `std::atomic<T*>`. A writer can
swap in a new object at any time — but it can't `delete` the old one, because a
reader on another thread may have loaded the old pointer a nanosecond earlier and
be about to dereference it. Safe reclamation answers **"when is it safe to
delete?"**

```
writer:  p_old = head.exchange(p_new);   // unlink: no NEW reader can find p_old
         retire(p_old);                  // "delete it when it's safe"

RCU:      safe after every reader that was already inside a read-side
          critical section has left it    (readers lock a domain — very cheap)

hazard    safe once no hazard pointer is protecting p_old
pointers: (each reader publishes "I'm using p" before dereferencing)
```

Both schemes need the same discipline from the writer: **unlink first, retire
second**, and never modify an object other threads can still see — publish a new
copy instead.

## Declaration

### `<rcu>` (P2545)

```cpp nocompile
namespace std {
  template <class T, class D = default_delete<T>>
  class rcu_obj_base {                       // CRTP base for rcu-protectable types
  public:
    void retire(D d = D(), rcu_domain& dom = rcu_default_domain()) noexcept;
  protected:
    rcu_obj_base() = default;                // + defaulted copy/move, all protected
  };

  class rcu_domain {                         // non-copyable, Cpp17Lockable
  public:
    void lock() noexcept;                    // enter a read-side critical section
    bool try_lock() noexcept;
    void unlock() noexcept;                  // leave it
  };

  rcu_domain& rcu_default_domain() noexcept;
  void rcu_synchronize(rcu_domain& dom = rcu_default_domain()) noexcept;
  void rcu_barrier(rcu_domain& dom = rcu_default_domain()) noexcept;
  template <class T, class D = default_delete<T>>
  void rcu_retire(T* p, D d = D(), rcu_domain& dom = rcu_default_domain());
}
```

| Name | What it does |
|---|---|
| `rcu_domain::lock` / `unlock` | Open and close a **read-side critical section** (a "region of RCU protection"). It's `Cpp17Lockable`, so use `std::scoped_lock` / `std::unique_lock`. |
| `rcu_default_domain()` | The one static domain the library provides; it's the default everywhere. |
| `obj.retire(d)` | Schedule `d(&obj)` for after every critical section that might see `obj` has ended. Doesn't block. `T` must derive from `rcu_obj_base<T, D>`. |
| `rcu_retire(p, d)` | Same, for any `T*` — no base class needed. May allocate (and throw); if it throws, nothing is scheduled. |
| `rcu_synchronize()` | **Blocks** until every critical section that started before the call has ended. After it returns you can `delete` directly. |
| `rcu_barrier()` | Blocks until all deletions already scheduled in the domain have run (e.g. before shutdown). It does **not** imply `rcu_synchronize`, nor the reverse. |

### `<hazard_pointer>` (P2530)

```cpp nocompile
namespace std {
  template <class T, class D = default_delete<T>>
  class hazard_pointer_obj_base {            // CRTP base for hazard-protectable types
  public:
    void retire(D d = D()) noexcept;
  protected:
    hazard_pointer_obj_base() = default;     // + defaulted copy/move, all protected
  };

  class hazard_pointer {                     // move-only owner of one hazard pointer
  public:
    hazard_pointer() noexcept;               // EMPTY: owns nothing, can't protect
    hazard_pointer(hazard_pointer&&) noexcept;
    hazard_pointer& operator=(hazard_pointer&&) noexcept;
    ~hazard_pointer();                       // releases it, ends protection

    bool empty() const noexcept;
    template <class T> T* protect(const atomic<T*>& src) noexcept;
    template <class T> bool try_protect(T*& ptr, const atomic<T*>& src) noexcept;
    template <class T> void reset_protection(const T* ptr) noexcept;
    void reset_protection(nullptr_t = nullptr) noexcept;
    void swap(hazard_pointer&) noexcept;
  };

  hazard_pointer make_hazard_pointer();      // may throw bad_alloc
  void swap(hazard_pointer&, hazard_pointer&) noexcept;
}
```

| Name | What it does |
|---|---|
| `make_hazard_pointer()` | Get a non-empty `hazard_pointer`. A default-constructed one is empty and its members can't be used. |
| `hp.protect(src)` | Load `src` and publish the result as protected, retrying until the value is stable; returns the protected pointer (may be null). |
| `hp.try_protect(ptr, src)` | One attempt: protect `ptr`, reload `src` (acquire); on a mismatch, store the new value in `ptr`, drop protection and return `false`. |
| `hp.reset_protection(p)` / `()` | Protect `p` directly / stop protecting anything. |
| `obj.retire(d)` | Schedule `d(&obj)` for once no hazard pointer protects `obj`. Doesn't block; may reclaim other retired objects in passing. |

The current working draft also declares `make_hazard_pointer_batch(span<hazard_pointer>)`
and `clear_hazard_pointer_batch(span<hazard_pointer>)`, which fill or empty a
whole array of hazard pointers at once (P3428, *Hazard Pointer Batches*).

### Requirements on the protected type

- `T` has **exactly one** public, non-virtual base that's a specialization of
  `rcu_obj_base` (or `hazard_pointer_obj_base`) for itself — CRTP:
  `struct Node : std::rcu_obj_base<Node> { … };`.
- `D` must be callable as `d(p)`; for the `*_obj_base` templates it must also be
  default-constructible and move-assignable (the object stores it).
- You may not specialize either base template.

## Compiler support

| Toolchain | `__cpp_lib_rcu` | `__cpp_lib_hazard_pointer` |
|---|---|---|
| GCC 15.2 (libstdc++) | not defined, no `<rcu>` | not defined, no `<hazard_pointer>` |
| GCC 16.2 (libstdc++) | not defined, no `<rcu>` | not defined, no `<hazard_pointer>` |
| Clang 22 + libc++ | not defined, no `<rcu>` | not defined, no `<hazard_pointer>` |

**No local toolchain ships either header yet**, so the snippets below are tagged
`needs=rcu` / `needs=hazard_pointer` and were only syntax-checked against the
working-draft declarations. Both designs come from production libraries:
[Folly](https://github.com/facebook/folly) has `folly::rcu_domain` and
`folly::hazptr_holder`, the reference implementations the papers were based on.
Test for the standard ones with:

```cpp
#include <version>
#if defined(__cpp_lib_rcu) && defined(__cpp_lib_hazard_pointer)
inline constexpr bool have_safe_reclamation = true;   // 202306L in C++26
#else
inline constexpr bool have_safe_reclamation = false;
#endif
```

## When to use it

| | RCU | Hazard pointers | `std::atomic<std::shared_ptr<T>>` |
|---|---|---|---|
| Reader cost | lock/unlock a domain — near zero | publish + re-check per pointer | atomic ref-count traffic on every read |
| Unreclaimed garbage | unbounded while a reader lingers | bounded by the number of protected pointers | none (freed when the last ref goes) |
| Protects | everything read inside the section | exactly the pointers you protect | the object you hold a `shared_ptr` to |
| Fits | read-mostly data: config, routing tables, snapshots | long-lived readers, many nodes (lists, maps) | when simplicity beats speed |

- **Anti-fit:** anything a `std::mutex` or `std::shared_mutex` handles fast
  enough. These are building blocks for lock-free structures; profile first.
- **Anti-fit:** objects that are *mutated in place*. Both schemes only manage
  lifetime; concurrent writes to fields are still data races.

## Examples

### RCU: a read-mostly configuration

```cpp needs=rcu
#include <atomic>
#include <mutex>
#include <rcu>
#include <string>
#include <utility>

struct Config : std::rcu_obj_base<Config> {
    Config(std::string n, int l) : name(std::move(n)), limit(l) {}
    std::string name;
    int limit;
};

std::atomic<Config*> current{new Config("default", 10)};

int read_limit() {
    std::scoped_lock guard(std::rcu_default_domain());   // read-side section
    Config* c = current.load(std::memory_order_acquire);
    return c->limit;            // safe: c can't be deleted until guard unlocks
}

void update(std::string name, int limit) {
    auto* fresh = new Config(std::move(name), limit);
    Config* old = current.exchange(fresh, std::memory_order_acq_rel);  // unlink
    old->retire();              // deleted after all pre-existing readers leave
}
```

### RCU: block instead of deferring, or retire a type you don't own

```cpp needs=rcu
#include <atomic>
#include <rcu>
#include <vector>

std::atomic<std::vector<int>*> table{new std::vector<int>{1, 2, 3}};

void replace_sync(std::vector<int> v) {
    auto* old = table.exchange(new std::vector<int>(std::move(v)));
    std::rcu_synchronize();     // wait out every reader that might hold `old`
    delete old;                 // now nobody can
}

void replace_async(std::vector<int> v) {
    auto* old = table.exchange(new std::vector<int>(std::move(v)));
    std::rcu_retire(old);       // no rcu_obj_base needed; may allocate
}

void shutdown() {
    std::rcu_barrier();         // run every deletion scheduled so far
}
```

### Hazard pointers: protect one node

```cpp needs=hazard_pointer
#include <atomic>
#include <hazard_pointer>

struct Node : std::hazard_pointer_obj_base<Node> {
    explicit Node(int v) : value(v) {}
    int value;
};

std::atomic<Node*> head{new Node(1)};

int read_value() {
    std::hazard_pointer hp = std::make_hazard_pointer();
    Node* p = hp.protect(head);     // published: p won't be reclaimed
    return p->value;
}                                   // hp destroyed -> protection released

void replace(int v) {
    Node* old = head.exchange(new Node(v));   // unlink
    old->retire();                  // reclaimed once no hazard pointer holds it
}
```

### Hazard pointers: the `try_protect` loop

`protect` is `try_protect` in a loop. Writing it out shows why it works: protect
the value you saw, then re-read the source; if it changed, the value you
protected may already be retired, so go round again.

```cpp needs=hazard_pointer
#include <atomic>
#include <hazard_pointer>

struct Node : std::hazard_pointer_obj_base<Node> { int value = 0; };

Node* acquire(std::hazard_pointer& hp, const std::atomic<Node*>& src) {
    Node* p = src.load(std::memory_order_relaxed);
    while (!hp.try_protect(p, src)) {
        // p now holds the newer value; try again with it
    }
    return p;                       // protected until hp is reset or destroyed
}
```

## Gotchas

- **Unlink before you retire.** Retiring an object that's still reachable from
  the shared structure lets new readers find it after it's freed.
- **Retire once.** Calling `retire` twice on the same object (or retiring it and
  also `delete`-ing it) is undefined behaviour.
- **Don't call `rcu_synchronize()` inside your own read-side section.** It waits
  for every section that began before it — including yours — so it never
  returns.
- **`rcu_barrier()` ≠ `rcu_synchronize()`.** The barrier waits for *already
  scheduled* deletions; the synchronize waits for *readers*. Neither implies the
  other.
- **A default-constructed `hazard_pointer` is empty.** `protect` on it is a
  precondition violation; get one from `make_hazard_pointer()`.
- **Protection is one pointer at a time.** Protecting a new pointer, calling
  `reset_protection()`, moving from, or destroying the `hazard_pointer` ends the
  old protection — don't keep using the old pointer after that.
- **RCU garbage is unbounded.** One reader that sits in a critical section
  stops all deferred deletions in that domain; keep sections short and never
  block inside them.
- **The protected type needs the base class exactly once**, public and
  non-virtual. For types you can't change, RCU has `rcu_retire`; hazard
  pointers have no non-intrusive form.

## See also

- [`<rcu>` — cppreference](https://en.cppreference.com/w/cpp/header/rcu)
- [`<hazard_pointer>` — cppreference](https://en.cppreference.com/w/cpp/header/hazard_pointer)
- [P2545 — Read-Copy Update (RCU)](https://wg21.link/p2545)
- [P2530 — Hazard Pointers for C++26](https://wg21.link/p2530)
- [P3428 — Hazard Pointer Batches](https://wg21.link/p3428)
- [`[saferecl]` in the working draft](https://eel.is/c++draft/saferecl)
- [Atomics](atomics.md) · [Mutexes](mutexes.md) ·
  [Smart pointers](../memory/README.md)
