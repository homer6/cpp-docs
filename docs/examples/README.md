# C++ examples — master index

Example-driven notes for learning all of modern C++ (C++11 → C++26), mirroring
[cppreference.com](https://cppreference.com/). Each area below is a folder with
its own index. We're working through them roughly in order of everyday usefulness.

> Legend: ✅ done · 🚧 in progress · ⬜ planned

## Standard library

| Area | Status | What's in it |
|---|---|---|
| [Containers](containers/README.md) | ✅ | vector, map, span, the lot |
| [Strings](strings/README.md) | ✅ | `string`, `string_view`, char traits |
| [Text processing](text-processing/README.md) | ✅ | `format`/`print`, `charconv`, regex, locales |
| [Algorithms](algorithms/README.md) | ✅ | `<algorithm>`, `<numeric>`, parallel & ranges algos |
| [Ranges](ranges/README.md) | ✅ | views, adaptors, factories, pipelines (C++20) |
| [Iterators](iterators/README.md) | ✅ | categories, adaptors, `iterator_traits` |
| [Utilities](utilities/README.md) | ✅ | `optional`, `variant`, `tuple`, `expected`, function objects |
| [Memory](memory/README.md) | ✅ | smart pointers, allocators, memory resources |
| [Metaprogramming](metaprogramming/README.md) | ✅ | `type_traits`, `ratio`, `integer_sequence` |
| [Numerics](numerics/README.md) | ✅ | `random`, `complex`, `<cmath>`, `<bit>`, SIMD, linalg |
| [Date & time](datetime/README.md) | ✅ | `<chrono>`: durations, clocks, calendars, time zones |
| [Input/output](io/README.md) | ✅ | streams, files, string streams, filesystem |
| [Concurrency](concurrency/README.md) | ✅ | threads, mutexes, atomics, futures |
| [Execution](execution/README.md) | ✅ | senders/receivers (C++26) |
| [Diagnostics](diagnostics/README.md) | ✅ | exceptions, `system_error`, `stacktrace` |
| [Language support](language-support/README.md) | ✅ | `numeric_limits`, `<=>`, `source_location`, `initializer_list` |
| [Concepts](concepts/README.md) | ✅ | the standard concepts library (C++20) |
| [Feature-test macros](feature-test-macros/README.md) | ✅ | detecting what your compiler supports |

## The core language

| Area | Status | What's in it |
|---|---|---|
| [Basic concepts](language/basic-concepts/README.md) | ✅ | types, names/lookup, storage/linkage, `main` |
| [Expressions](language/expressions/README.md) | ✅ | value categories, operators, conversions, literals, constant exprs |
| [Statements](language/statements/README.md) | ✅ | `if`/`switch`, loops, range-`for` |
| [Declarations & initialization](language/declarations-initialization/README.md) | ✅ | `auto`, `const`/`constexpr`, the many initializations |
| [Functions](language/functions/README.md) | ✅ | overloading, default args, lambdas |
| [Classes](language/classes/README.md) | ✅ | special members, RAII, inheritance, virtual, unions |
| [Templates](language/templates/README.md) | ✅ | function/class templates, variadics, CTAD |
| [Exceptions](language/exceptions/README.md) | ✅ | `throw`/`try`/`catch`, `noexcept`, exception safety |
| [Coroutines](language/coroutines/README.md) | ✅ | `co_await`/`co_yield`/`co_return` (C++20) |
| [Modules](language/modules/README.md) | ✅ | `import`/`export` (C++20) |
| [Preprocessor](language/preprocessor/README.md) | ✅ | macros, includes, conditional compilation |

---

Conventions live in [`/CLAUDE.md`](../../CLAUDE.md): cppreference is the only
cited source; pages follow a fixed skeleton; examples target the latest standard.
