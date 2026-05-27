# std::variant

> A type-safe union: holds exactly one value out of a fixed set of types, and
> always knows which. **Header:** `<variant>` · **Since:** C++17

## Mental model

Where a C `union` forgets what it stores (and lets you read it as the wrong
type), `variant<A, B, C>` tracks the active alternative and checks access. It's
the **closed sum type**: the set of possibilities is known at compile time.

```
 variant<int, std::string, double> v = "hi";
   index() == 1   →   active alternative is std::string
```

## Examples

### Assign, query, access

```cpp
#include <variant>

std::variant<int, std::string> v = 42;
v.index();                       // 0  (int is alternative #0)
v = "hello";                     // now holds std::string, index() == 1

std::holds_alternative<std::string>(v);   // true
std::get<std::string>(v);                  // "hello" — throws std::bad_variant_access if wrong
std::get<1>(v);                            // by index
if (auto* p = std::get_if<int>(&v)) { /* not taken here */ }   // pointer or nullptr
```

### Visiting — the idiomatic way to handle all cases

```cpp
std::variant<int, double, std::string> v = 3.14;

std::visit([](const auto& x) { /* one generic lambda for all types */ }, v);

// distinct handling per type via an overload set:
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;   // CTAD guide

std::visit(overloaded{
    [](int i)                { /* handle int */ },
    [](double d)             { /* handle double */ },
    [](const std::string& s) { /* handle string */ },
}, v);
```

## Gotchas

- **`std::get` with the wrong alternative throws `std::bad_variant_access`.** Use
  `get_if` (returns pointer/nullptr) or `holds_alternative` to test first, or
  prefer `visit` to handle every case exhaustively.
- **`std::visit` enforces exhaustiveness** — if your visitor can't handle every
  alternative, it won't compile. That's a feature: add a type, get told where to
  update.
- **The valueless-by-exception state.** If a type's move/copy throws mid-assign,
  a variant can become `valueless_by_exception()` (rare). Check it if you use
  throwing types.
- **Duplicate types need index access.** `variant<int, int>` can't be accessed by
  type with `get<int>`; use `get<0>`/`get<1>`.
- **Recursive variants** (e.g. a JSON tree) need indirection — a `variant` can't
  contain itself by value.

## See also

- [std::variant — cppreference](https://en.cppreference.com/w/cpp/utility/variant)
- [std::visit — cppreference](https://en.cppreference.com/w/cpp/utility/variant/visit)
- [std::get_if — cppreference](https://en.cppreference.com/w/cpp/utility/variant/get_if)
