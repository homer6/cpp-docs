# General utilities library

The "vocabulary types" and core helpers modern C++ leans on everywhere. Mirrors
[cppreference's general utilities](https://en.cppreference.com/w/cpp/utility).

| Page | Header | Since | What |
|---|---|---|---|
| [`utility` helpers](utility.md) | `<utility>` | C++11+ | `move`, `forward`, `swap`, `exchange`, `cmp_*`, `to_underlying` |
| [`pair`](pair.md) | `<utility>` | C++98 | Two heterogeneous values |
| [`tuple`](tuple.md) | `<tuple>` | C++11 | N heterogeneous values |
| [`optional`](optional.md) | `<optional>` | C++17 | A value, or nothing |
| [`variant`](variant.md) | `<variant>` | C++17 | A type-safe union (one of N types) |
| [`expected`](expected.md) | `<expected>` | C++23 | A value **or** an error |
| [`any`](any.md) | `<any>` | C++17 | A value of *any* type |
| [`bitset`](bitset.md) | `<bitset>` | C++98 | Fixed-size bit array |
| [Function objects](functional.md) | `<functional>` | C++11+ | `function`, `bind`, `invoke`, `ref`, `move_only_function` |

## The "sum type / product type" map

- **Product** (all fields at once): [`pair`](pair.md), [`tuple`](tuple.md), or a
  plain `struct` (prefer a named struct when fields have meaning).
- **Sum** (exactly one of several): [`variant`](variant.md) when the set of types
  is fixed and known; [`any`](any.md) when it isn't.
- **Optional value** (zero or one): [`optional`](optional.md).
- **Value or error** (success/failure with payload): [`expected`](expected.md).
