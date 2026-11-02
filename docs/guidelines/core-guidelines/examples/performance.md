# Per: Performance — examples

> Our examples for the [Per rules](../performance.md): measure before you
> optimize, then give the compiler and the cache what they need: types,
> compile-time values, few allocations and compact, predictable data. How to
> measure is in [performance tooling](../../performance-tooling.md).

## Per.1: Don't optimize without reason

*No example: the rule is about deciding whether a speed-up is needed at all (a requirement, a measured problem), which no code pattern shows.*

See [performance tooling](../../performance-tooling.md).

## Per.2: Don't optimize prematurely

*No example: the rule is about when to optimize (after the design works and a profile points somewhere), not about how code is written.*

See [performance tooling](../../performance-tooling.md).

## Per.3: Don't optimize something that's not performance critical

*No example: which code is critical is found by profiling the whole program, not by reading a snippet.*

See [performance tooling](../../performance-tooling.md).

## Per.4: Don't assume that complicated code is necessarily faster than simple code

Compilers are very good at simple loops: they unroll and vectorize them
themselves. Unrolling by hand makes the code longer and easier to get wrong,
and usually buys nothing over what the optimizer already does.

```cpp nocompile
#include <cstddef>
#include <vector>
// BAD: compiles, but hand-unrolled "for speed": four accumulators, and the tail loop
// is missing, so the last size() % 4 elements are silently dropped.
long total(const std::vector<int>& v) {
    long a = 0, b = 0, c = 0, d = 0;
    for (std::size_t i = 0; i + 4 <= v.size(); i += 4) {
        a += v[i]; b += v[i + 1]; c += v[i + 2]; d += v[i + 3];
    }
    return a + b + c + d;
}
```

```cpp
#include <numeric>
#include <vector>
// GOOD: says what it does, handles every element, and optimizers unroll and vectorize it.
long total(const std::vector<int>& v) {
    return std::accumulate(v.begin(), v.end(), 0L);
}
```

## Per.5: Don't assume that low-level code is necessarily faster than high-level code

A standard algorithm knows things a hand-written loop hides: for trivially
copyable types `std::ranges::copy` typically becomes a single `memmove`. Writing it
out by hand with raw pointers is no faster and easier to get wrong.

```cpp nocompile
#include <cstddef>
// BAD: compiles, but hand-rolled pointer code: easy to get the bounds or overlap
// wrong, and no faster than the library's copy.
void copy_ints(const int* src, int* dst, std::size_t n) {
    const char* s = reinterpret_cast<const char*>(src);
    char* d = reinterpret_cast<char*>(dst);
    for (std::size_t i = 0; i < n * sizeof(int); ++i) d[i] = s[i];
}
```

```cpp
#include <algorithm>
#include <span>
// GOOD: high-level, typed, and implementations lower it to memmove for ints.
void copy_ints(std::span<const int> src, std::span<int> dst) {
    std::ranges::copy(src, dst.begin());
}
```

## Per.6: Don't make claims about performance without measurements

*No example: the rule is about backing claims with benchmarks and profiles (Google Benchmark, `perf`, a `<chrono>` timer), not about the code being measured.*

See [performance tooling](../../performance-tooling.md).

## Per.7: Design to enable optimization

An interface decides what the implementation can ever do. One that erases
types and calls through function pointers stops the compiler from inlining
and stops you from changing the implementation later; one that keeps the
types (a template, a range, a callable) leaves both open.

```cpp nocompile
#include <cstddef>
// BAD: compiles, but every element goes through void* and an opaque function pointer:
// no inlining, no type checking, and the element size is the caller's problem.
void for_each_element(void* data, std::size_t count, std::size_t size,
                      void (*fn)(void* element, void* ctx), void* ctx);
```

```cpp
#include <ranges>
// GOOD: the element type and the operation are visible to the compiler, which can
// inline fn and vectorize the loop; the caller passes any range and any callable.
template <std::ranges::input_range R, class F>
    requires std::invocable<F&, std::ranges::range_reference_t<R>>
void for_each_element(R&& r, F fn) {
    for (auto&& e : r) fn(e);
}
```

## Per.10: Rely on the static type system

Types tell the optimizer what can alias and what a comparison means, and
tell the compiler what to check. `void*` interfaces throw that away: a
comparator for the wrong type still compiles.

```cpp nocompile
#include <cstdlib>
#include <vector>
// BAD: compiles, but the comparator reads the doubles as ints: a silent, wrong sort.
int compare_ints(const void* a, const void* b) {
    return *static_cast<const int*>(a) - *static_cast<const int*>(b);
}
void sort_prices(std::vector<double>& prices) {
    std::qsort(prices.data(), prices.size(), sizeof(double), compare_ints);
}
```

```cpp
#include <algorithm>
#include <vector>
// GOOD: the element type is known, so the comparison is checked and can be inlined.
void sort_prices(std::vector<double>& prices) {
    std::ranges::sort(prices);
}
```

Tested by [`tests/guidelines/performance_test.cpp`](../../../../tests/guidelines/performance_test.cpp).

## Per.11: Move computation from run time to compile time

A value that depends only on constants can be computed by the compiler: no
start-up cost, no initialization-order or data-race questions (you can't race
on a constant), and errors show up as compile errors. The proof that a
run-time table isn't a constant:

```cpp compile-fail="constant"
#include <array>
std::array<int, 8> make_squares() {
    std::array<int, 8> t{};
    for (int i = 0; i < 8; ++i) t[i] = i * i;
    return t;
}
const std::array<int, 8> squares = make_squares();   // BAD: built at program start-up
static_assert(squares[3] == 9);                      // error: not a constant expression
```

```cpp
#include <array>
constexpr std::array<int, 8> make_squares() {
    std::array<int, 8> t{};
    for (int i = 0; i < 8; ++i) t[i] = i * i;
    return t;
}
constexpr std::array<int, 8> squares = make_squares();   // GOOD: computed by the compiler
static_assert(squares[3] == 9);
```

Tested by [`tests/guidelines/performance_test.cpp`](../../../../tests/guidelines/performance_test.cpp).

## Per.12: Eliminate redundant aliases

When two references might name the same object, the compiler has to reload
through them after every write, and the code can do something surprising
when they really do alias. Take small inputs by value so there's nothing to
alias.

```cpp nocompile
// BAD: compiles, but `step` may alias `total`, so the compiler must reload it after the
// first write, and add_twice(x, x) gives 4x, not 3x.
void add_twice(int& total, const int& step) {
    total += step;
    total += step;
}
```

```cpp
// GOOD: step is a copy: kept in a register, and add_twice(x, x) gives 3x.
void add_twice(int& total, int step) {
    total += step;
    total += step;
}
```

Tested by [`tests/guidelines/performance_test.cpp`](../../../../tests/guidelines/performance_test.cpp).

## Per.13: Eliminate redundant indirections

Each pointer you follow is a possible cache miss. Small values stored behind
pointers (a vector of `unique_ptr<Point>`) scatter across the heap; stored
directly, they sit next to each other in one allocation.

```cpp nocompile
#include <memory>
#include <vector>
struct Point { float x, y; };
// BAD: compiles, but every Point is a separate heap allocation reached through a pointer.
float sum_x(const std::vector<std::unique_ptr<Point>>& pts) {
    float s = 0;
    for (const auto& p : pts) s += p->x;
    return s;
}
```

```cpp
#include <vector>
struct Point { float x, y; };
// GOOD: the Points are stored contiguously; the loop streams through memory.
float sum_x(const std::vector<Point>& pts) {
    float s = 0;
    for (const auto& p : pts) s += p.x;
    return s;
}
```

Tested by [`tests/guidelines/performance_test.cpp`](../../../../tests/guidelines/performance_test.cpp).

## Per.14: Minimize the number of allocations and deallocations

Allocation is slow compared with most arithmetic, and a growing vector
reallocates (and moves everything) several times on the way up. When you
know the size, reserve it once; reuse buffers instead of rebuilding them.

```cpp nocompile
#include <string>
#include <vector>
// BAD: compiles, but the vector reallocates repeatedly as it grows.
std::vector<std::string> labels(int n) {
    std::vector<std::string> out;
    for (int i = 0; i < n; ++i) out.push_back("item " + std::to_string(i));
    return out;
}
```

```cpp
#include <string>
#include <vector>
// GOOD: one allocation for the vector's storage.
std::vector<std::string> labels(int n) {
    std::vector<std::string> out;
    out.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) out.push_back("item " + std::to_string(i));
    return out;
}
```

Tested by [`tests/guidelines/performance_test.cpp`](../../../../tests/guidelines/performance_test.cpp).

## Per.15: Do not allocate on a critical branch

On a latency-critical path, an allocation can take a lock inside the
allocator or go to the OS, and its cost varies. Allocate up front, outside
the critical path, and reuse the memory: `clear()` keeps a vector's capacity.

```cpp nocompile
#include <span>
#include <vector>
void send(std::span<const char>);
// BAD: compiles, but each packet allocates a fresh buffer on the hot path.
void on_packet(std::span<const char> in) {
    std::vector<char> frame(in.begin(), in.end());
    frame.push_back('\n');
    send(frame);
}
```

```cpp
#include <span>
#include <vector>
void send(std::span<const char>);
// GOOD: the buffer is allocated once, at start-up; the hot path only reuses it.
class Framer {
public:
    explicit Framer(std::size_t max_packet) { frame_.reserve(max_packet + 1); }
    void on_packet(std::span<const char> in) {   // in.size() <= max_packet
        frame_.clear();                           // keeps the capacity
        frame_.insert(frame_.end(), in.begin(), in.end());
        frame_.push_back('\n');
        send(frame_);
    }
private:
    std::vector<char> frame_;
};
```

Tested by [`tests/guidelines/performance_test.cpp`](../../../../tests/guidelines/performance_test.cpp).

## Per.16: Use compact data structures

Memory access dominates the run time of most programs, so data that packs
tightly runs faster. A `std::list<int>` spends two pointers and a heap
allocation on every 4-byte value; a `std::vector<int>` stores the values back
to back.

```cpp nocompile
#include <list>
#include <numeric>
// BAD: compiles, but each element is a separate node: pointer-chasing and wasted space.
int total(const std::list<int>& values) {
    return std::accumulate(values.begin(), values.end(), 0);
}
```

```cpp
#include <numeric>
#include <vector>
// GOOD: contiguous storage, the most compact sequence and the fastest to scan.
int total(const std::vector<int>& values) {
    return std::accumulate(values.begin(), values.end(), 0);
}
```

Tested by [`tests/guidelines/performance_test.cpp`](../../../../tests/guidelines/performance_test.cpp).

## Per.17: Declare the most used member of a time-critical struct first

The first member is at the object's address, so it shares a cache line with
the start of the object, and on some machines it's reached with the shortest
instruction. Put the hot field first and the bulky, rarely used ones after it.

```cpp nocompile
// BAD: compiles, but the field read on every lookup sits behind 128 bytes of
// rarely used name, on another cache line from the start of the object.
struct Session {
    char user_name[128];
    int  last_seen;      // read on every request
};
```

```cpp
// GOOD: the hot field is first, at offset 0.
struct Session {
    int  last_seen;      // read on every request
    char user_name[128];
};
```

Tested by [`tests/guidelines/performance_test.cpp`](../../../../tests/guidelines/performance_test.cpp).

## Per.18: Space is time

Smaller data means more of it per cache line and fewer misses. Members are
padded to their alignment, so the order you declare them in can change a
struct's size: largest first wastes the least.

```cpp nocompile
// BAD: compiles, but padding after each bool makes this 24 bytes on typical 64-bit ABIs.
struct Order {
    bool   paid;
    double amount;
    bool   shipped;
    int    quantity;
};
```

```cpp
// GOOD: the same members, largest first: 16 bytes on typical 64-bit ABIs.
struct Order {
    double amount;
    int    quantity;
    bool   paid;
    bool   shipped;
};
```

Tested by [`tests/guidelines/performance_test.cpp`](../../../../tests/guidelines/performance_test.cpp).

## Per.19: Access memory predictably

Caches and prefetchers reward walking memory in order. In row-major storage
(C++ arrays, a flat `vector` indexed as `r * cols + c`) neighbours in a row
are adjacent, so the inner loop should run along the row; striding down a
column touches a new cache line every step.

```cpp nocompile
#include <vector>
// BAD: compiles, but the inner loop jumps `cols` elements at a time.
double sum(const std::vector<double>& m, int rows, int cols) {
    double s = 0;
    for (int c = 0; c < cols; ++c)
        for (int r = 0; r < rows; ++r) s += m[r * cols + c];
    return s;
}
```

```cpp
#include <vector>
// GOOD: the inner loop walks consecutive elements.
double sum(const std::vector<double>& m, int rows, int cols) {
    double s = 0;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) s += m[r * cols + c];
    return s;
}
```

Tested by [`tests/guidelines/performance_test.cpp`](../../../../tests/guidelines/performance_test.cpp).

## Per.30: Avoid context switches on the critical path

Blocking on a lock, a condition variable, a future or I/O can put the thread
to sleep and cost a context switch, with microseconds of latency and cold
caches after. On the critical path, do the small work inline and hand
anything slow to another thread without waiting for it.

```cpp nocompile
#include <future>
int price(int order);
// BAD: compiles, but each order starts a task and blocks on it: up to two context
// switches per order for a computation that takes nanoseconds.
int quote(int order) {
    return std::async(std::launch::async, price, order).get();
}
```

```cpp
int price(int order);
// GOOD: the small computation runs inline; nothing on the critical path blocks.
int quote(int order) {
    return price(order);
}
```
