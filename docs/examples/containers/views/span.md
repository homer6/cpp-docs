# std::span

> A lightweight, **non-owning view** over a contiguous sequence of elements —
> "a pointer + a length" with a container-like interface.
> **Header:** `<span>` · **Since:** C++20

> Not a container: it owns nothing and allocates nothing. It's a window onto
> storage that lives somewhere else (an array, `vector`, `std::array`, …).

## Mental model

```
 std::vector<int> v{10,20,30,40,50};

 std::span<int> s = v;        // s just remembers: data ptr → v[0], size = 5
                              ┌────┬────┬────┬────┬────┐
                  v owns ──▶  │ 10 │ 20 │ 30 │ 40 │ 50 │
                              └────┴────┴────┴────┴────┘
                  s views ──▶  └──────────── 5 ─────────┘
```

Copying a span copies the pointer+length, **not** the elements. The viewed data
must outlive the span.

## Declaration

```cpp
template<
    class T,
    std::size_t Extent = std::dynamic_extent
> class span;
```

- `T` — element type. Use `const T` for a read-only view.
- `Extent` — either a **compile-time fixed size**, or `std::dynamic_extent` (the
  default) meaning the length is carried at runtime.
  - `std::span<int>` → dynamic extent (stores a size).
  - `std::span<int, 4>` → fixed extent of 4 (size known at compile time, not
    stored).

## Why it exists

Before C++20, "take a contiguous range of `T`" meant either templating on the
container, or passing `(T* ptr, size_t n)` as two error-prone arguments. `span`
is the single, safe parameter type:

```cpp
// Works for C arrays, std::array, std::vector, another span, ...
double sum(std::span<const double> xs) {
    double total = 0;
    for (double x : xs) total += x;     // range-for, .size(), [] all available
    return total;
}
```

## Examples

### Constructing from various sources

```cpp
#include <span>
#include <vector>
#include <array>

int c_arr[4]{1, 2, 3, 4};
std::array<int, 4> std_arr{1, 2, 3, 4};
std::vector<int> vec{1, 2, 3, 4};

std::span<int> s1{c_arr};               // deduces size 4
std::span<int> s2{std_arr};
std::span<int> s3{vec};
std::span<int> s4{vec.data(), 2};       // first 2 elements (ptr + count)
```

### Subviews (no copying)

```cpp
std::span<int> s = vec;                 // {1,2,3,4}
auto head = s.first(2);                 // view of {1,2}
auto tail = s.last(2);                  // view of {3,4}
auto mid  = s.subspan(1, 2);            // view of {2,3}

s[0] = 99;                              // writes through to vec[0]
```

### const view = read-only window

```cpp
void print(std::span<const int> xs) {   // callers can't mutate through this
    for (int x : xs) { /* ... */ }
}
```

### As bytes (C++20)

```cpp
std::span<const std::byte> raw = std::as_bytes(std::span{vec});
```

## Gotchas

- **It does not own anything — dangling is on you.** A span outliving its backing
  storage is a use-after-free. Never return a span to a local, and beware spans
  into a `vector` you then grow/reallocate.
- **No bounds checking on `operator[]`** (UB if out of range). `at()` was added
  later (C++26); otherwise check `size()` yourself.
- **Fixed vs dynamic extent are different types.** `span<int,4>` won't implicitly
  become `span<int>` in every context the way you might assume — though
  fixed→dynamic conversions are allowed.
- **`std::string_view` is the string-specific cousin** (read-only view of chars);
  prefer it for text.

## See also

- [std::span — cppreference](https://en.cppreference.com/w/cpp/container/span)
- [std::dynamic_extent — cppreference](https://en.cppreference.com/w/cpp/container/span/dynamic_extent)
- [std::as_bytes — cppreference](https://en.cppreference.com/w/cpp/container/span/as_bytes)
