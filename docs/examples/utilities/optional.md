# std::optional

> A value that may or may not be present — a type-safe "maybe", without resorting
> to sentinel values or pointers. **Header:** `<optional>` · **Since:** C++17

## Mental model

`optional<T>` holds either a `T` (constructed in place, no heap) or *nothing*
(`std::nullopt`). It's the honest return type for "this might not produce a
value": parse failures, lookups that can miss, lazily-computed values.

```
 optional<int> a = 42;          → [ engaged | 42 ]
 optional<int> b = std::nullopt;→ [ empty       ]
```

## Examples

### Create, check, access

```cpp
#include <optional>

std::optional<int> find_even(std::span<const int> xs) {
    for (int x : xs) if (x % 2 == 0) return x;
    return std::nullopt;                  // or just `return {};`
}

auto r = find_even(data);
if (r) {                                  // contextually bool: engaged?
    int v = *r;                           // operator* — UB if empty
    int w = r.value();                    // throws std::bad_optional_access if empty
}
int v = r.value_or(-1);                   // value, or a fallback if empty
```

### In-place construction

```cpp
std::optional<std::string> s;
s.emplace(5, 'x');               // construct "xxxxx" in place, no temporary
std::optional<std::string> t{std::in_place, "hello"};
s.reset();                       // back to empty
```

### Monadic operations (C++23)

Chain without unwrapping by hand:

```cpp
std::optional<int> n = parse(str);
auto out = n.and_then(validate)        // optional<U>: run only if engaged
            .transform(square)          // optional<V>: map the value if engaged
            .or_else(default_value);    // supply an optional if empty
```

## Gotchas

- **`*opt` and `opt->m` are UB when empty.** Use `value()` (checked, throws) or
  guard with `if (opt)`. Pick `*` only when you've already checked.
- **`optional<bool>` and `optional<T*>` are subtle.** `if (opt)` tests
  *engagement*, not the contained value — an engaged `optional<bool>{false}` is
  truthy as an optional. Use `opt.has_value()` / compare explicitly to avoid
  confusion.
- **Not for "value or error".** If failure carries information, use
  [`expected`](expected.md) (C++23) so the caller learns *why*.
- **No reference support.** `optional<T&>` isn't in the standard (yet); store a
  pointer, or `std::reference_wrapper`, if you need optional reference semantics.

## See also

- [std::optional — cppreference](https://en.cppreference.com/w/cpp/utility/optional)
- [optional monadic ops — cppreference](https://en.cppreference.com/w/cpp/utility/optional/and_then)
