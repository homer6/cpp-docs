# std::tuple

> A fixed-size collection of heterogeneous values — `pair` generalized to N
> elements. **Header:** `<tuple>` · **Since:** C++11

## Mental model

A compile-time-sized, type-heterogeneous bundle. Elements are reached by
**index** (`std::get<0>`) or, if types are unique, by **type** (`std::get<T>`).
Great for multiple return values and generic code; a named `struct` is usually
clearer when the fields have meaning.

## Examples

### Make, access, unpack

```cpp
#include <tuple>

std::tuple<int, double, std::string> t{1, 2.5, "hi"};
std::tuple u{1, 2.5, "hi"};                  // CTAD (C++17)

std::get<0>(t);                 // 1        (by index)
std::get<std::string>(t);       // "hi"     (by type — only if unique)
std::tuple_size_v<decltype(t)>; // 3

auto [i, d, s] = t;             // structured bindings (C++17) — the usual way
```

### Multiple return values

```cpp
std::tuple<int, int> divmod(int a, int b) { return {a / b, a % b}; }

auto [q, r] = divmod(17, 5);    // q == 3, r == 2
```

### tie and ignore

```cpp
int q; int r;
std::tie(q, r) = divmod(17, 5);          // assign into existing variables
std::tie(std::ignore, r) = divmod(9, 4); // discard the quotient

// tie is also the classic way to write lexicographic comparisons:
bool less(const P& a, const P& b) {
    return std::tie(a.x, a.y) < std::tie(b.x, b.y);
}
```

### Combine and forward

```cpp
auto cat = std::tuple_cat(std::tuple{1, 2}, std::tuple{'a'});  // tuple<int,int,char>
std::apply([](int a, int b){ return a + b; }, std::tuple{3, 4}); // call with unpacked args → 7
```

## Gotchas

- **Prefer structured bindings over `std::get`** for readability; reserve `get`
  for generic/index-computed access.
- **`std::get<T>` requires the type be unique** in the tuple, else it's
  ill-formed.
- **Don't overuse tuples as pseudo-structs.** If the elements have names/meaning,
  a `struct` self-documents and still supports structured bindings.
- **`std::make_tuple` decays types** (strips references); use `std::tie` for
  references and `std::forward_as_tuple` to forward.

## See also

- [std::tuple — cppreference](https://en.cppreference.com/w/cpp/utility/tuple)
- [std::apply — cppreference](https://en.cppreference.com/w/cpp/utility/apply)
- [std::tie — cppreference](https://en.cppreference.com/w/cpp/utility/tuple/tie)
