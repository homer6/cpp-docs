# std::stack

> A **LIFO** (last-in, first-out) adaptor that wraps another container and exposes
> only push/pop/top. **Header:** `<stack>` · **Since:** C++98

## Mental model

Not a container itself — an **adaptor** that restricts an underlying container to
stack operations. You only ever touch one end ("the top"). By default it sits on
a `std::deque`.

```
 push ─▶ ┌───┐ ◀─ pop / top
         │ c │   ← top
         │ b │
         │ a │
         └───┘
```

## Declaration

```cpp
template<
    class T,
    class Container = std::deque<T>
> class stack;
```

- `Container` — the backing store; must support `back`, `push_back`, `pop_back`.
  `std::deque` (the default), `std::vector`, and `std::list` all qualify.

## Complexity

All core operations are O(1) (delegated to the underlying container's back-end
operations): `push`, `pop`, `top`, `empty`, `size`.

## Interface (deliberately tiny)

| Member | Meaning |
|---|---|
| `push(x)` / `emplace(args...)` | add to top |
| `pop()` | remove top — **returns nothing** |
| `top()` | access top element (read/write) |
| `empty()` / `size()` | state |

There are **no iterators** — you can't traverse a `stack`. That's the point.

## Examples

```cpp
#include <stack>

std::stack<int> s;
s.push(1);
s.push(2);
s.emplace(3);          // construct in place at the top

s.top();               // 3   (does NOT remove)
s.pop();               // removes 3, returns void
s.size();              // 2

while (!s.empty()) {
    int x = s.top();   // read top
    s.pop();           // then remove — two steps, always
}
```

### Choosing a different backing container

```cpp
#include <vector>
std::stack<int, std::vector<int>> vs;   // back it with a vector instead of deque
```

## Gotchas

- **`pop()` returns `void`.** To both read and remove you must `top()` then
  `pop()` — there's no combined "pop and return." (This is intentional: returning
  by value while popping couldn't be exception-safe in C++98.)
- **`top()`/`pop()` on an empty stack is UB.** Always guard with `empty()`.
- **No iteration, no random access** — by design. If you need to look inside,
  you picked the wrong type (use the underlying container directly).

## See also

- [std::stack — cppreference](https://en.cppreference.com/w/cpp/container/stack)
