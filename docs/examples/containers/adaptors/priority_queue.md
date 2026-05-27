# std::priority_queue

> A **max-heap** adaptor: the largest element (by the comparator) is always at the
> top. **Header:** `<queue>` · **Since:** C++98

## Mental model

An adaptor that keeps its underlying container arranged as a **binary heap**.
You can only see/remove the single highest-priority element (`top`). By default
"highest priority" means **largest** (`std::less` → max-heap). It sits on a
`std::vector` by default.

```
            (9)         ← top()
           /   \
        (7)     (8)
        / \
      (3) (5)         heap-ordered; not fully sorted
```

## Declaration

```cpp
template<
    class T,
    class Container = std::vector<T>,
    class Compare = std::less<typename Container::value_type>
> class priority_queue;
```

- `Compare` — `std::less` (default) gives a **max-heap**: `top()` is the
  greatest. Use `std::greater<T>` for a **min-heap**.
- `Container` — needs random access + `front`/`push_back`/`pop_back`:
  `std::vector` (default) or `std::deque`.

## Complexity

| Operation | Cost |
|---|---|
| `top` | O(1) |
| `push` / `emplace` | O(log n) |
| `pop` | O(log n) |
| build from a range (constructor) | O(n) |

## Interface

| Member | Meaning |
|---|---|
| `push(x)` / `emplace(args...)` | insert, restoring heap order |
| `pop()` | remove the top — **returns nothing** |
| `top()` | the highest-priority element (read-only: `const` ref) |
| `empty()` / `size()` | state |

No iteration; only the top is reachable.

## Examples

### Default max-heap

```cpp
#include <queue>

std::priority_queue<int> pq;
pq.push(3);
pq.push(9);
pq.push(5);

pq.top();              // 9  (largest)
pq.pop();              // removes 9
pq.top();              // 5
```

### Min-heap with std::greater

```cpp
#include <queue>
#include <vector>

std::priority_queue<int, std::vector<int>, std::greater<int>> minpq;
minpq.push(3); minpq.push(9); minpq.push(5);
minpq.top();           // 3  (smallest)
```

### Custom priority (e.g. by a field)

```cpp
struct Task { int priority; std::string name; };
auto cmp = [](const Task& a, const Task& b){ return a.priority < b.priority; };

std::priority_queue<Task, std::vector<Task>, decltype(cmp)> jobs(cmp);
jobs.push({5, "build"});
jobs.push({9, "deploy"});
jobs.top();            // {9, "deploy"} — highest priority first
```

### Build from existing data in O(n)

```cpp
std::vector<int> data{4, 1, 7, 3};
std::priority_queue<int> pq(data.begin(), data.end());   // heapify, O(n)
```

## Gotchas

- **Default is a *max*-heap.** People expect "priority queue = smallest first";
  in C++ it's largest first unless you pass `std::greater`.
- **The comparator's sense is inverted from intuition.** `Compare(a, b)` meaning
  "a goes *before* b in ordering" puts the *largest* on top — i.e. `std::less`
  gives max-heap. Flip it (`std::greater`) for a min-heap.
- **`top()` is `const`; `pop()` returns `void`.** Read with `top()`, then `pop()`.
  You can't mutate the top in place (it would break the heap).
- **Not sorted, just heap-ordered.** Iterating the underlying container does not
  give sorted order; only repeated `pop` does.

## See also

- [std::priority_queue — cppreference](https://en.cppreference.com/w/cpp/container/priority_queue)
- [Heap algorithms (`std::make_heap`, ...) — cppreference](https://en.cppreference.com/w/cpp/algorithm/make_heap)
