# Concurrency and asynchronous code

> Give every piece of shared mutable state one synchronization rule, make security checks and the actions they guard atomic together, and give every queued, threaded or suspended operation a lifetime argument.

## Mental model

An attacker can't usually pick your thread schedule, but they can send many
requests at once, and that's often enough. Concurrency bugs become security
bugs in three ways:

1. **Data races** corrupt state, including the state a security decision
   reads: a reference count, a length, a permission flag. A data race is
   undefined behaviour (**CP.2**; CERT
   [CON43-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/concurrency-con/con43-c)).
2. **Check-then-act races**: each step is synchronized, but the check and the
   action aren't done together, so two requests both pass the check
   ("balance ≥ 60", "file doesn't exist", "token unused").
3. **Lifetime across time**: a callback, task or coroutine runs after the
   object it uses has gone (see [ownership and lifetime](ownership-and-lifetime.md)).

## One synchronization rule per shared object

**MUST:** For every shared mutable object, say what protects it (a specific
mutex, atomics, a single owning thread, immutability) and follow that rule on
every access. Keep the mutex next to the data it guards and lock it with RAII
(**CP.50**, **CP.20**; CERT
[CON51-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/concurrency-con/con51-cpp)).
Prefer sharing less: pass small data by value between threads (**CP.3**,
**CP.31**).

**MUST:** Make a security check and the action it guards one critical section.
A check under one lock and the action under another, or no lock, is a race:

```cpp bad
#include <mutex>

class Account {
public:
    bool withdraw(int amount) {
        if (balance() < amount) return false;   // check (locks, unlocks)
        debit(amount);                          // act (locks again): two requests can both pass
        return true;
    }
private:
    int balance() { std::scoped_lock l{m_}; return balance_; }
    void debit(int a) { std::scoped_lock l{m_}; balance_ -= a; }
    std::mutex m_;
    int balance_ = 100;
};
```

```cpp
#include <mutex>

class Account {
public:
    bool withdraw(int amount) {
        std::scoped_lock lock{m_};              // check and act together
        if (balance_ < amount) return false;
        balance_ -= amount;
        return true;
    }
private:
    std::mutex m_;
    int balance_ = 100;
};
```

**MUST:** Take multiple locks in one consistent order, or all at once with
`std::scoped_lock` (**CP.21**; CERT
[CON53-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/concurrency-con/con53-cpp)),
and never call unknown code (a callback, a virtual function from a plugin)
while holding a lock (**CP.22**). A deadlock is a denial of service.

**MUST:** Wait on a condition variable with a predicate, since it can wake
spuriously (CERT
[CON54-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/concurrency-con/con54-cpp)),
and don't destroy a mutex another thread may still hold
([CON50-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/concurrency-con/con50-cpp)).
Bit-fields that share a memory location can't be protected separately
([CON52-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/concurrency-con/con52-cpp)).

**SHOULD:** Prefer locks and message passing to lock-free code; write your own
lock-free structure or double-checked locking only with a measured need and
expert review (**CP.100**, **CP.110**; use `std::call_once` or a function-local
`static` for one-time initialization).

## `shared_ptr` is ownership, not synchronization

**MUST:** Don't treat `shared_ptr` as making the object thread-safe. Its
reference count is updated atomically, so copies of the *same* `shared_ptr`
held by different threads are fine; the object it points to is not protected
at all, and two threads writing one `shared_ptr` *variable* is a race on that
variable. Protect the pointee with its own rule (**CP.32**), and swap a shared
pointer variable under a lock or with `std::atomic<std::shared_ptr<T>>`
(C++20; libstdc++ has it, libc++ 22 doesn't define
`__cpp_lib_atomic_shared_ptr` yet). See
[atomics](../examples/concurrency/atomics.md) and
[shared_ptr](../examples/memory/shared_ptr.md).

## Lifetimes across threads, tasks and suspensions

**MUST:** For every queued callback, timer, completion handler, future and
coroutine frame, know what keeps the objects it uses alive and what happens
on cancellation and shutdown. Ask:

- Can the operation outlive the object that started it?
- Can cancellation race with completion?
- Can a callback run during the owner's destruction?
- Does a captured pointer keep the object alive, or only remember its address?
- Can shutdown wait forever on work that needs a lock shutdown is holding?

**MUST:** Treat a coroutine as code that can resume later, on another thread,
after its caller has gone. Pass coroutine parameters by value, not by
reference (**CP.53**); don't use capturing lambdas as coroutines, because the
captures live in the lambda object, not the coroutine frame (**CP.51**); and
don't hold a lock across `co_await` (**CP.52**), which can deadlock or hand the
lock to another thread. See [coroutines](../examples/language/coroutines/coroutines.md).

```cpp nocompile
Task<void> handle(const Request& req);   // BAD: the frame keeps a reference; the caller's Request may be gone on resume
Task<void> handle(Request req);          // GOOD: the frame owns its copy
```

## Find races with ThreadSanitizer

Build a separate configuration with `-fsanitize=thread` (it can't be combined
with ASan) and run tests that exercise the concurrent paths: concurrent
requests, cancellation, shutdown. The repo's CI runs the suite under TSan on
Linux with libstdc++ (see [verification](verification.md) and
[`docs/testing.md`](../testing.md)).

## See also

- Core Guidelines: [CP.2–CP.201](../guidelines/core-guidelines/concurrency.md)
- [mutex](../examples/concurrency/mutexes.md) · [atomics](../examples/concurrency/atomics.md) ·
  [coroutines](../examples/language/coroutines/coroutines.md)
- [Ownership and lifetime](ownership-and-lifetime.md) · [resource limits](resource-limits.md)
- Tests: [`tests/security/concurrency_test.cpp`](../../tests/security/concurrency_test.cpp)
