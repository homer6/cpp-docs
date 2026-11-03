# Resource limits and availability

> Make resource use part of validation: give every request a budget for memory, work, items, depth and time, check it before the expensive part, and make rejection itself cheap.

## Mental model

Availability is a security property. A request can be perfectly valid by
every type and range check and still be designed to exhaust you: a
"1 GB" length field, a zip bomb, ten million tiny JSON objects, keys chosen to
collide in your hash table, a thousand slow connections. The defence is the
same as for any other input: a policy, checked early.

```
cheap checks (size, rate, auth) ─▶ budgeted parse (items, depth, bytes) ─▶ bounded work ─▶ response
        reject here is cheap            reject here is still cheap           never unbounded
```

## Give each request a budget

**MUST:** Define explicit limits, from your threat model, for everything an
input can make you consume: memory and total allocation, message size and
decompressed size, item counts and nesting depth, CPU-heavy operations,
threads, queued tasks, handles and connections, request duration and
retries, and the total per user or tenant. These are policy numbers, not the
largest value a type can hold (see [integers](integers.md) for the
arithmetic).

**MUST:** Charge the budget for the whole operation, not per allocation. A
parser that checks each string against a 1 MB limit but accepts any number of
them has no limit.

```cpp
#include <cstddef>
#include <stdexcept>

// One per request: every allocation the request causes draws from it.
class Budget {
public:
    explicit Budget(std::size_t bytes) : left_{bytes} {}
    bool take(std::size_t n) noexcept {          // false: over budget, reject the request
        if (n > left_) return false;
        left_ -= n;
        return true;
    }
    std::size_t left() const noexcept { return left_; }
private:
    std::size_t left_;
};
```

**MUST:** Bound expansion. Check decompressed size against the budget as you
inflate, and reject a ratio no legitimate input would have, rather than
inflating fully and checking after.

## Bound queues and concurrency

**MUST:** Give every queue, cache and pool a capacity, and decide what happens
when it's full (reject, drop the oldest, apply back-pressure). An unbounded
queue turns a burst of requests into memory exhaustion; a cache keyed by
attacker input grows until it's evicted by the OOM killer.

```cpp
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

template <class T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_{capacity} {}
    bool try_push(T value) {                      // false: full, caller rejects or retries later
        std::scoped_lock lock{m_};
        if (q_.size() >= capacity_) return false;
        q_.push_back(std::move(value));
        return true;
    }
    std::optional<T> try_pop() {
        std::scoped_lock lock{m_};
        if (q_.empty()) return std::nullopt;
        T v = std::move(q_.front());
        q_.pop_front();
        return v;
    }
private:
    std::mutex m_;
    std::deque<T> q_;
    std::size_t capacity_;
};
```

## Watch the algorithm, not just the size

**MUST:** Know the worst case of what you run on input. Recursion without a
depth limit overflows the stack (see the bounded parser on
[parsing and validation](parsing-and-validation.md#bound-the-parsers-work));
backtracking regexes can take exponential time; sorting or searching with a
comparator the input controls can degrade.

**SHOULD:** Don't key an `unordered_map` directly by attacker-chosen strings
without a limit on its size. The standard lets `std::hash` be salted per run,
but libstdc++ and libc++ don't salt it: the same string hashes the same way in
every process, so an attacker can precompute keys that collide and turn every
lookup into a linear scan.[^hash] Cap the number of entries, or use a keyed
hash (SipHash with a per-process random key) for tables that face untrusted
keys.

## Handle allocation failure deliberately

**MUST:** Decide what an allocation failure does, and test it. `new` throws
`std::bad_alloc` (CERT
[MEM52-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/memory-management-mem/mem52-cpp));
a container asked for more than `max_size()` throws `std::length_error`; but on
systems that overcommit memory a huge allocation may *succeed* and the
process is killed later when it touches the pages. A budget checked before
allocating is the only reliable control; catching `bad_alloc` is a last
resort. With exceptions off, the failure terminates
([error handling](error-handling.md)).

**MUST:** Check that rejection, cancellation and cleanup don't open a new
exhaustion path: a rejected request should cost less than an accepted one,
and a cancelled operation should release what it held (**P.8**).

## See also

- Core Guidelines: [P.8, P.9](../guidelines/core-guidelines/philosophy.md),
  [R.5](../guidelines/core-guidelines/resource.md)
- [unordered_map](../examples/containers/unordered/unordered_map.md) ·
  [memory resources](../examples/memory/memory-resources.md) (bounded arenas)
- [Integers](integers.md) · [parsing and validation](parsing-and-validation.md) ·
  [concurrency](concurrency.md)
- Tests: [`tests/security/resource-limits_test.cpp`](../../tests/security/resource-limits_test.cpp)

[^hash]: cppreference, [`std::hash`](https://en.cppreference.com/w/cpp/utility/hash): "Hash functions are only required to produce the same result for the same input within a single execution of a program; this allows salted hashes that prevent collision denial-of-service attacks." libstdc++ seeds its string hash with a fixed value; checked locally that `std::hash<std::string>` gives the same result in separate processes with libstdc++ 15 and libc++ 22.
