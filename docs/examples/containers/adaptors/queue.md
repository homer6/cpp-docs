# std::queue

> A **FIFO** (first-in, first-out) adaptor: push at the back, pop from the front.
> **Header:** `<queue>` · **Since:** C++98

## Mental model

An adaptor that restricts an underlying container to queue operations. Items
enter at the back and leave from the front, in order. Defaults to a
`std::deque`, which does both ends in O(1).

```
        front                         back
 pop ◀─ ┌───┬───┬───┬───┐ ◀─ push
        │ a │ b │ c │ d │
        └───┴───┴───┴───┘
```

## Declaration

```cpp
template<
    class T,
    class Container = std::deque<T>
> class queue;
```

- `Container` — must support `front`, `back`, `push_back`, `pop_front`.
  `std::deque` (default) and `std::list` qualify; **`std::vector` does not**
  (no `pop_front`).

## Complexity

All O(1): `push`, `pop`, `front`, `back`, `empty`, `size`.

## Interface

| Member | Meaning |
|---|---|
| `push(x)` / `emplace(args...)` | enqueue at the back |
| `pop()` | dequeue from the front — **returns nothing** |
| `front()` / `back()` | peek at either end |
| `empty()` / `size()` | state |

No iterators — you can't traverse a `queue`.

## Examples

```cpp
#include <queue>

std::queue<int> q;
q.push(1);
q.push(2);
q.emplace(3);          // {1,2,3}, 1 at front

q.front();             // 1  (next to be served)
q.back();              // 3  (most recently added)
q.pop();               // removes 1 (front), returns void

while (!q.empty()) {
    int x = q.front();
    q.pop();           // read front, then remove
}
```

## Gotchas

- **`pop()` returns `void`** and removes the **front**; read it with `front()`
  first.
- **`front()`/`back()`/`pop()` on an empty queue is UB.** Guard with `empty()`.
- **`std::vector` can't back a `queue`** (it lacks `pop_front`); use `deque`
  (default) or `list`.
- For a **double-ended** queue you can index/iterate, drop the adaptor and use
  [`std::deque`](../sequence/deque.md) directly. For **priority** ordering, use
  [`std::priority_queue`](priority_queue.md).

## See also

- [std::queue — cppreference](https://en.cppreference.com/w/cpp/container/queue)
