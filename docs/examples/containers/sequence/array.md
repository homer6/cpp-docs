# std::array

> A fixed-size array whose size is part of its type, with the convenience of a
> standard container. **Header:** `<array>` · **Since:** C++11

## Mental model

A thin, zero-overhead wrapper around a C array `T[N]`. No heap, no pointer
indirection — the elements live **inside** the object (on the stack, or wherever
the array itself lives). The size `N` is a compile-time constant baked into the
type.

```
std::array<int, 4>   →   [ a | b | c | d ]   (contiguous, inline, size fixed at 4)
```

## Declaration

```cpp
template<class T, std::size_t N>
struct array;
```

- `T` — element type.
- `N` — number of elements, a compile-time constant. `N == 0` is allowed
  (empty array; `data()` may return anything, don't dereference).

It's an **aggregate**: no user-declared constructors, so you initialize it with
braces (note the brace behavior below).

## Complexity

| Operation | Cost |
|---|---|
| `a[i]`, `at`, `front`, `back`, `data` | O(1) |
| `fill`, `swap` | O(n) |
| size | O(1), and it's a compile-time constant |

There is no insert/erase/push_back — the size never changes.

## Iterator & reference invalidation

**Never** (the storage never moves or resizes). The only thing that ends an
iterator's life is the array object itself going out of scope. `swap` does *not*
invalidate iterators but does swap the elements they refer to.

## When to use it

- A **fixed count known at compile time**: an RGBA color, a 3-D vector, a lookup
  table, a small scratch buffer.
- You want value semantics (copy/assign/compare the whole thing) that raw C
  arrays don't give you, with zero runtime overhead.
- **Avoid** when the size varies at runtime (use `vector`, or `inplace_vector`
  for a runtime size under a fixed cap, C++26).

## Examples

### Initialization (mind the braces)

```cpp
#include <array>

std::array<int, 3> a{1, 2, 3};
std::array<int, 3> z{};            // all zero-initialized
std::array<int, 3> p{1};           // {1, 0, 0} — rest value-initialized

std::array b{1, 2, 3};             // CTAD → std::array<int, 3>  (C++17)
```

CTAD deduces both `T` and `N` from the initializer, so `b` is
`std::array<int, 3>`. All elements must share a common type.

### Access and iteration

```cpp
a[0];                 // unchecked
a.at(0);              // bounds-checked → std::out_of_range
a.front(); a.back();
int* raw = a.data();  // contiguous; pass to C APIs / std::span

for (int x : a) { /* ... */ }
std::array<int, 3>::size_type n = a.size();  // == 3, also a.size() is constexpr
```

### constexpr / compile-time use

```cpp
constexpr std::array<int, 5> squares = []{
    std::array<int, 5> r{};
    for (int i = 0; i < 5; ++i) r[i] = i * i;
    return r;
}();                                  // computed at compile time
static_assert(squares[4] == 16);
```

### Structured bindings & std::get

```cpp
std::array<int, 3> pt{10, 20, 30};
auto [x, y, z2] = pt;                 // structured bindings (C++17)
int second = std::get<1>(pt);         // compile-time-checked index
```

### to_array (deduce from a C array literal, C++20)

```cpp
#include <array>
auto a = std::to_array({1, 2, 3});        // std::array<int, 3>
auto s = std::to_array("hi");             // std::array<char, 3> incl. '\0'
```

## Gotchas

- **No decay, no implicit size loss.** Unlike `int[]`, passing a `std::array`
  carries its size in the type — great for safety, but it means functions
  templated on `N` get instantiated per size.
- **Empty arrays.** `std::array<T, 0>` is valid but `front`/`back`/`data`
  dereference is UB.
- **Large arrays on the stack.** Since storage is inline, a huge `std::array`
  can blow the stack — use `vector` for big runtime data.
- **Aggregate init quirk (pre-C++14):** historically needed double braces
  `{{...}}`; modern compilers accept single braces.

## See also

- [std::array — cppreference](https://en.cppreference.com/w/cpp/container/array)
- [std::to_array — cppreference](https://en.cppreference.com/w/cpp/container/array/to_array)
