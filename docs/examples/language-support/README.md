# Language support library

Small but essential facilities that prop up core language features. Mirrors
[cppreference's language support library](https://en.cppreference.com/w/cpp/utility#Language_support).

| Page | Header | Since | What |
|---|---|---|---|
| [`numeric_limits`](numeric-limits.md) | `<limits>` | C++98 | range/precision of arithmetic types |
| [Three-way comparison](comparison.md) | `<compare>` | C++20 | `operator<=>`, `= default` comparisons |
| [`source_location`](source-location.md) | `<source_location>` | C++20 | file/line/function, replacing `__FILE__`/`__LINE__` |
| [`initializer_list`](initializer-list.md) | `<initializer_list>` | C++11 | the `{...}` brace-list type |

> Also in this library (not given their own pages here): `std::type_info`
> (`typeid`), `std::nullptr_t`, `<csignal>`/`<csetjmp>` (legacy C facilities),
> `std::move_only_function` lives under [Utilities](../utilities/functional.md).
