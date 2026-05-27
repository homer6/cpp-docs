# std::expected

> Holds **either** a success value **or** an error value — explicit, allocation-free
> error handling without exceptions. **Header:** `<expected>` · **Since:** C++23

## Mental model

`expected<T, E>` is like an [`optional`](optional.md) that, when empty, carries a
*reason* (`E`) instead of nothing. It's the return type for "this can fail, and
the caller should know why" — a typed alternative to throwing or to
`errno`-style out-parameters.

```
 expected<int, Error> r;
   r.has_value()  →  holds T (the int)
  !r.has_value()  →  holds E (the Error), reachable via r.error()
```

## Examples

### Returning success or error

```cpp
#include <expected>

enum class Err { empty, not_a_number, overflow };

std::expected<int, Err> to_int(std::string_view s) {
    if (s.empty()) return std::unexpected{Err::empty};      // failure
    int value;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec == std::errc::invalid_argument)  return std::unexpected{Err::not_a_number};
    if (ec == std::errc::result_out_of_range) return std::unexpected{Err::overflow};
    return value;                                            // success
}
```

### Consuming the result

```cpp
auto r = to_int("42");
if (r) {                          // engaged == success
    int v = *r;                   // value (operator*)
    int w = r.value();            // throws std::bad_expected_access<E> if it's an error
}
int v = r.value_or(0);            // value, or fallback on error
if (!r) Err e = r.error();        // the error payload
```

### Monadic chaining (no manual if-ladders)

```cpp
auto out = to_int(text)
    .and_then(check_positive)     // runs only on success; returns expected<U,E>
    .transform(square)            // maps the value on success
    .transform_error(to_message); // maps the error on failure
```

## Gotchas

- **`*r` / `r->m` are UB on the error path.** Use `value()` (throws
  `std::bad_expected_access`) or guard with `if (r)`. `error()` is UB if there's
  a value — mirror image.
- **Wrap the error in `std::unexpected`** when returning it, so overload
  resolution knows it's the error path (especially when `T` and `E` are
  similar/convertible types).
- **`expected<void, E>`** is valid for "succeeds or fails with no success
  payload" — like a richer `bool`.
- **vs. exceptions:** `expected` makes the error path explicit in the type and
  costs nothing on the happy path, but you must thread it through call sites.
  Use exceptions for truly exceptional/unrecoverable conditions, `expected` for
  expected, recoverable failures.

## See also

- [std::expected — cppreference](https://en.cppreference.com/w/cpp/utility/expected)
- [std::unexpected — cppreference](https://en.cppreference.com/w/cpp/utility/expected/unexpected)
