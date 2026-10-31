# Language support library

Small but essential facilities that prop up core language features. Mirrors
[cppreference's language support library](https://en.cppreference.com/w/cpp/utility#Language_support).

| Page | Header | Since | What |
|---|---|---|---|
| [`numeric_limits`](numeric-limits.md) | `<limits>` | C++98 | range/precision of arithmetic types |
| [Three-way comparison](comparison.md) | `<compare>` | C++20 | `operator<=>`, `= default` comparisons |
| [`source_location`](source-location.md) | `<source_location>` | C++20 | file/line/function, replacing `__FILE__`/`__LINE__` |
| [`initializer_list`](initializer-list.md) | `<initializer_list>` | C++11 | the `{...}` brace-list type |
| [`typeid`, `type_info`, `type_index`](type-info.md) | `<typeinfo>`, `<typeindex>` | C++98 / C++11 | run-time type identification, type-keyed maps |
| [Signal handling](signals.md) | `<csignal>` | C++98 | `std::signal`, `std::raise`, what a handler may do |
| [Non-local jumps](non-local-jumps.md) | `<csetjmp>` | C++98 | `setjmp` / `longjmp` and why C++ code avoids them |

> Not given their own pages: `std::nullptr_t` (`<cstddef>`).
> `std::move_only_function` lives under [Utilities](../utilities/functional.md);
> the floating-point environment is under [Numerics](../numerics/cfenv.md).
