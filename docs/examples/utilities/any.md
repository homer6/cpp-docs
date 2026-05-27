# std::any

> A container for a single value of **any** copyable type, with safe,
> type-checked retrieval. **Header:** `<any>` · **Since:** C++17

## Mental model

Type-erased storage: it remembers the `typeid` of whatever you put in, and you
get it back only by naming the exact type. Where [`variant`](variant.md) is a
*closed* set of known types, `any` is *open* — the type isn't known at compile
time.

## Examples

```cpp
#include <any>

std::any a = 42;                 // holds int
a = std::string{"hello"};        // now holds std::string
a.has_value();                   // true
a.type();                        // typeid(std::string)

std::any_cast<std::string>(a);   // "hello" — throws std::bad_any_cast if wrong type
if (auto* p = std::any_cast<int>(&a)) { /* nullptr here: not an int */ }

a.reset();                       // now empty
```

### A heterogeneous bag

```cpp
std::map<std::string, std::any> properties;
properties["count"] = 10;
properties["name"]  = std::string{"widget"};
properties["scale"] = 1.5;
// retrieve by knowing the stored type:
int n = std::any_cast<int>(properties["count"]);
```

## Gotchas

- **You must name the exact stored type.** `any_cast<long>` on an `any` holding
  `int` throws `bad_any_cast` — no implicit conversions. The pointer form
  (`any_cast<T>(&a)`) returns `nullptr` instead of throwing; prefer it for
  probing.
- **Reach for `variant` first.** If the set of possible types is known and small,
  `variant` is faster, exhaustively checkable, and may avoid allocation. `any`
  is for genuinely open-ended type erasure (plugin values, property bags).
- **May allocate.** Small objects often use a small-buffer optimization, but
  larger ones heap-allocate; it's not free.
- **Copyable types only.** The stored type must be copy-constructible.

## See also

- [std::any — cppreference](https://en.cppreference.com/w/cpp/utility/any)
- [std::any_cast — cppreference](https://en.cppreference.com/w/cpp/utility/any/any_cast)
