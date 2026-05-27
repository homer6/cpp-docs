# `<utility>` helpers

> The small, foundational free functions that show up in almost every modern C++
> file: `move`, `forward`, `swap`, `exchange`, and friends.
> **Header:** `<utility>`

## std::move — cast to rvalue

`std::move` moves *nothing*. It's a cast to an rvalue reference that says "I'm
done with this; you may steal its guts." Whether anything is stolen is up to the
move constructor/assignment that receives it.

```cpp
std::string a = "hello";
std::string b = std::move(a);   // b steals a's buffer; a is now valid-but-unspecified
// reading a's value is allowed but meaningless; assigning a new value is fine
```

## std::forward — perfect forwarding

In a function template taking a *forwarding reference* `T&&`, `std::forward<T>`
preserves the original value category (lvalue stays lvalue, rvalue stays rvalue)
when passing it on. Use it only with deduced `T&&`.

```cpp
template<class T>
void relay(T&& x) {
    sink(std::forward<T>(x));   // lvalue→lvalue, rvalue→rvalue
}
```

Rule of thumb: **`move` an rvalue you own; `forward` a forwarding reference.**

## std::swap — exchange two values

```cpp
int x = 1, y = 2;
std::swap(x, y);                 // x==2, y==1
```

Provide an ADL-findable `swap` for your type (or rely on member `swap`); the
standard library uses the customization point. Most std types specialize it to
be O(1).

## std::exchange — set new, return old (C++14)

```cpp
int old = std::exchange(x, 42);  // x becomes 42, `old` holds the previous value
```

Handy in move constructors (`ptr_ = std::exchange(other.ptr_, nullptr);`) and
loop accumulators.

## Safe integer comparison (C++20)

Comparing signed and unsigned directly is a classic bug (`-1 < 0u` is *false*).
The `cmp_*` functions compare by mathematical value:

```cpp
#include <utility>
std::cmp_less(-1, 0u);           // true  (correct), unlike  -1 < 0u
std::cmp_greater_equal(x, y);
std::in_range<std::size_t>(-1);  // false — does -1 fit in size_t?
```

Family: `cmp_equal`, `cmp_not_equal`, `cmp_less`, `cmp_greater`,
`cmp_less_equal`, `cmp_greater_equal`, `in_range`.

## Smaller helpers

```cpp
std::as_const(obj);              // C++17: view as const (force const overloads)
std::to_underlying(some_enum);   // C++23: enum → its underlying integer
auto [a, b] = std::pair{1, 2};   // std::pair / std::make_pair live here too
std::unreachable();              // C++23: mark code as never reached (UB if it is)
```

## Gotchas

- **`std::move` on a `const` object moves nothing** — the move ctor can't bind to
  `const&&`, so it silently copies. Don't `move` from `const`.
- **Don't use a moved-from object's value.** It's valid (destructible,
  assignable) but unspecified — assign before reading.
- **`std::forward` with the wrong type defeats forwarding.** Always
  `std::forward<T>(x)` where `T` is the deduced template parameter, never on a
  named rvalue you own (use `move` there).
- **`std::move` in a `return` of a local is usually wrong** — it can disable copy
  elision (NRVO). Just `return local;`.

## See also

- [std::move — cppreference](https://en.cppreference.com/w/cpp/utility/move)
- [std::forward — cppreference](https://en.cppreference.com/w/cpp/utility/forward)
- [std::exchange — cppreference](https://en.cppreference.com/w/cpp/utility/exchange)
- [Integer comparison functions — cppreference](https://en.cppreference.com/w/cpp/utility/intcmp)
