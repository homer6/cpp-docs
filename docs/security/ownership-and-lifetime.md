# Ownership and lifetime

> Give every resource one explicit owner, and give every borrowed pointer, reference, view or callback a reason it can't outlive what it refers to.

## Mental model

A use-after-free or double free is a lifetime bug that an attacker can steer:
they choose the timing (a connection closing while its callback is queued),
the order (a request that frees an object another request still uses), or the
data that lands in the freed memory. The defence is the Core Guidelines'
resource rules: an **owner** that releases exactly once (RAII), and
**borrowers** whose lifetime is shorter than the owner's, by construction.
[Memory & safety](../guidelines/safety.md) covers the rules in general; this
page is about the places where input and timing decide whether they hold.

```
owner (unique_ptr, container, value member, RAII handle) ── releases once
   └── borrowers (T*, T&, span, string_view, iterator, captured this)
          must end before the owner does, including:
          ├── returned or stored     ├── captured by a queued callback
          └── held across co_await   └── passed to another thread
```

## Make ownership explicit

**MUST:** Own resources through types that release them: value members,
standard containers, `std::unique_ptr` (exclusive), `std::shared_ptr` (shared,
when sharing is the design), and RAII wrappers for OS and library handles.
No owning raw pointers in ordinary code (**R.1**, **R.3**, **R.11**, **R.20**,
**I.11**; CERT [MEM51-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/memory-management-mem/mem51-cpp)).

| Situation | Use | Why it matters for security |
|---|---|---|
| Ordinary owned state | a value member or a standard container | lifetime follows the owner; nothing to forget |
| Exclusive dynamic ownership | `std::unique_ptr<T>` (`make_unique`, **R.23**) | moves are explicit; it can't be copied into a second owner |
| Shared ownership | `std::shared_ptr<T>` (`make_shared`, **R.22**), `weak_ptr` for observers (**R.24**) | an object used by callbacks or threads stays alive as long as any of them needs it |
| Borrowed object | `T&`, or `T*` when null is meaningful (**R.3**, **F.7**) | documents that the callee doesn't keep it |
| Borrowed sequence | `std::span<T>`, `std::string_view` | carries the extent; the storage must still outlive it |
| File, socket, lock, mapping, crypto context | an RAII wrapper (e.g. `unique_ptr` with a deleter) | released on every path, exceptions included (**C.31**, **R.12**) |

A C handle gets an owner the same way as memory:

```cpp
#include <cstdio>
#include <memory>

struct FileCloser {
    void operator()(std::FILE* f) const noexcept { if (f) std::fclose(f); }
};
using File = std::unique_ptr<std::FILE, FileCloser>;

File open_log(const char* path) {
    return File{std::fopen(path, "a")};   // closed exactly once, on every path (R.12)
}
```

**MUST:** Never give one raw pointer to two owners. Constructing a second
smart pointer from a pointer another one already owns frees it twice
(CERT [MEM56-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/memory-management-mem/mem56-cpp)):

```cpp bad run
#include <memory>

int main() {
    auto first = std::make_unique<int>(42);
    std::unique_ptr<int> second{first.get()};   // BAD: a second, unrelated owner
}   // both delete the same int: double free (ASan: attempting double-free)
```

Copy or move the owner itself (`std::move(first)`, or a `shared_ptr` copy), and
use `enable_shared_from_this` when an object needs a `shared_ptr` to itself.

## Give every borrow a lifetime argument

RAII releases resources; it doesn't stop a borrower from outliving its owner.
**MUST:** For every pointer, reference, iterator, view or captured `this` that
is **returned, stored, captured by a callback, queued, held across a
suspension or passed to another thread**, be able to say who owns the object
and what stops it being destroyed or invalidated before the last use
(**F.43**, **F.53**, **Lifetime.1**, **ES.65**; CERT
[EXP54-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp54-cpp),
[EXP61-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp61-cpp),
[MEM50-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/memory-management-mem/mem50-cpp)).

The common security case: a callback queued for later captures an object that
the peer can make go away first (by disconnecting, timing out, or sending a
"close" message):

```cpp bad run
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct Session {
    std::string user = "alice";
    void on_reply() { user += "!"; }
};

int main() {
    std::vector<std::function<void()>> queue;
    auto s = std::make_unique<Session>();
    queue.push_back([p = s.get()] { p->on_reply(); });   // BAD: captures a borrowed pointer
    s.reset();                                           // the peer disconnected
    for (auto& task : queue) task();                     // use after free (ASan: heap-use-after-free)
}
```

Make the callback hold something that keeps the object alive, or that can tell
it's gone. A `weak_ptr` does the second: the callback does nothing once the
session has ended (**R.24**, **CP.32**):

```cpp run
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct Session {
    std::string user = "alice";
    int replies = 0;
    void on_reply() { ++replies; }
};

int main() {
    std::vector<std::function<void()>> queue;
    auto s = std::make_shared<Session>();
    queue.push_back([w = std::weak_ptr<Session>{s}] {
        if (auto p = w.lock()) p->on_reply();             // only if it still exists
    });
    s.reset();                                           // the peer disconnected
    for (auto& task : queue) task();                     // safe: lock() fails, nothing happens
}
```

The alternatives are capturing a `shared_ptr` (the callback keeps the session
alive until it runs; fine when that's acceptable, but it can extend lifetimes
an attacker controls) or having the owner cancel its pending work in its
destructor. What matters is that the code says which one it is.

**SHOULD:** Return owning values when the caller needs an independent
lifetime; return a view only into storage the interface promises to keep
stable. A `string_view` into a temporary, a local, or a container that the
next request can grow, dangles (**F.43**; CERT
[CTR51-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/containers-ctr/ctr51-cpp)).
Container invalidation is on the [bounds](bounds.md) page; borrows held
across `co_await` and by other threads are on the
[concurrency](concurrency.md) page.

## Confine manual lifetime management

**MUST:** Keep placement `new`, `std::construct_at`/`std::destroy_at`, custom
allocators and pools, manual destructor calls, pointer tagging and casts that
reinterpret storage in small, reviewed components with tests. "The memory is
still allocated" is not a lifetime argument: an object is usable only between
its construction and its destruction, and storage must be suitably aligned and
large enough (CERT
[MEM53-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/memory-management-mem/mem53-cpp),
[MEM54-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/memory-management-mem/mem54-cpp)).
The object model's rules are on the
[object model](../examples/language/basic-concepts/object-model.md) and
[raw memory](../examples/memory/raw-memory.md) pages.

## Gotchas

- **`shared_ptr` keeps objects alive, which is also a resource.** A queue of
  callbacks each holding a `shared_ptr` to a large session lets a client keep
  memory pinned; bound the queue ([resource limits](resource-limits.md)).
- **`shared_ptr` isn't synchronization.** Its count is thread-safe; the object
  isn't ([concurrency](concurrency.md)).
- **ASan finds these only on the paths you run.** Exercise the disconnect,
  timeout and error paths under the sanitizer and with a fuzzer
  ([verification](verification.md)).

## See also

- Core Guidelines: [R.1–R.37](../guidelines/core-guidelines/resource.md),
  [F.42–F.53](../guidelines/core-guidelines/functions.md),
  [I.11](../guidelines/core-guidelines/interfaces.md),
  [C.31](../guidelines/core-guidelines/class.md),
  [Lifetime.1](../guidelines/core-guidelines/profile.md),
  [ES.65](../guidelines/core-guidelines/expr.md),
  [CP.32](../guidelines/core-guidelines/concurrency.md)
- [Memory & safety: ownership, dangling references, use after move](../guidelines/safety.md#ownership-with-smart-pointers)
- [unique_ptr](../examples/memory/unique_ptr.md) · [shared_ptr](../examples/memory/shared_ptr.md) ·
  [weak_ptr](../examples/memory/weak_ptr.md)
- Tests: [`tests/security/ownership-and-lifetime_test.cpp`](../../tests/security/ownership-and-lifetime_test.cpp)
