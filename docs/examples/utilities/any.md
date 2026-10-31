# std::any

> A container for a single value of **any** copyable type, with safe,
> type-checked retrieval. **Header:** `<any>` · **Since:** C++17

## Mental model

Type-erased storage: it remembers the `typeid` of whatever you put in, and you
get it back only by naming the exact type. Where [`variant`](variant.md) is a
*closed* set of known types, `any` is *open* — the type isn't known at compile
time.

## Declaration

```cpp
class any;
```

| Piece | Meaning |
|---|---|
| `any()`, `any(v)`, `any(std::in_place_type<T>, args…)` | Empty, a copy of `v`, or a `T` built in place |
| `emplace<T>(args…)`, `reset()`, `swap` | Replace the value, empty it, swap |
| `has_value()`, `type()` | Is there a value; its `typeid` (`typeid(void)` when empty) |
| `std::any_cast<T>(a)` | Value or reference to the stored `T`; throws `std::bad_any_cast` on a type mismatch |
| `std::any_cast<T>(&a)` | Pointer to the stored `T`, or `nullptr` |
| `std::make_any<T>(args…)` | Factory that builds the `T` in place |

## When to use it

- **Fits:** values whose type the code holding them genuinely can't know —
  property bags, plugin or scripting payloads, a `void*`-style "user data" slot
  made type-safe.
- **Anti-fits:** a known, small set of types → [`variant`](variant.md); "maybe
  a value" → [`optional`](optional.md); a common interface → a virtual base
  class or a concept.

## Examples

```cpp
#include <any>
#include <string>

void any_basics() {
    std::any a = 42;                 // holds int
    a = std::string{"hello"};        // now holds std::string
    a.has_value();                   // true
    a.type();                        // typeid(std::string)

    std::any_cast<std::string>(a);   // "hello" — throws std::bad_any_cast if wrong type
    if (auto* p = std::any_cast<int>(&a)) { /* nullptr here: not an int */ }

    a.reset();                       // now empty
}
```

### A heterogeneous bag

```cpp
#include <any>
#include <map>
#include <string>

void property_bag() {
    std::map<std::string, std::any> properties;
    properties["count"] = 10;
    properties["name"]  = std::string{"widget"};
    properties["scale"] = 1.5;
    // retrieve by knowing the stored type:
    int n = std::any_cast<int>(properties["count"]);
}
```

### Build in place, modify through a reference

```cpp
#include <any>
#include <string>

void in_place() {
    auto a = std::make_any<std::string>(3, 'x');   // "xxx", no temporary
    std::any_cast<std::string&>(a) += "!";         // a reference cast edits the stored value
    std::any_cast<const std::string&>(a);          // "xxx!"
}
```

## Gotchas

- **You must name the exact stored type.** `any_cast<long>` on an `any` holding
  `int` throws `bad_any_cast` — no implicit conversions. The pointer form
  (`any_cast<T>(&a)`) returns `nullptr` instead of throwing; prefer it for
  probing.
- **A string literal is stored as `const char*`.** `std::any a = "hi";` holds a
  pointer, so `any_cast<std::string>(a)` throws; store `std::string{"hi"}`.
- **`any_cast<T>(a)` returns a copy.** Cast to `T&` to modify the stored value.
- **Reach for `variant` first.** If the set of possible types is known and small,
  `variant` is faster, exhaustively checkable, and may avoid allocation. `any`
  is for genuinely open-ended type erasure (plugin values, property bags).
- **May allocate.** Small objects often use a small-buffer optimization, but
  larger ones heap-allocate; it's not free.
- **Copyable types only.** The stored type must be copy-constructible.

## See also

- [std::any — cppreference](https://en.cppreference.com/w/cpp/utility/any)
- [std::any_cast — cppreference](https://en.cppreference.com/w/cpp/utility/any/any_cast)
- [std::make_any — cppreference](https://en.cppreference.com/w/cpp/utility/any/make_any)
