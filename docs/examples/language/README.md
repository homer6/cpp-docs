# The core language

The language itself — separate from the standard library. Mirrors the "Language"
column of [cppreference](https://cppreference.com/).

| Area | What |
|---|---|
| [Basic concepts](basic-concepts/README.md) | fundamental types, scope/storage/linkage, `main`, the object model |
| [Expressions](expressions/README.md) | value categories, operators, conversions, literals, constant expressions |
| [Statements](statements/README.md) | `if`/`switch`, the loops, range-`for` |
| [Declarations & initialization](declarations-initialization/README.md) | `auto`, `const`/`constexpr`, the forms of initialization |
| [Functions](functions/README.md) | overloading, default/variadic args, lambdas |
| [Classes](classes/README.md) | special members & RAII, inheritance/virtual, access, unions |
| [Templates](templates/README.md) | function/class templates, variadics & folds, specialization, CTAD |
| [Exceptions](exceptions/README.md) | `throw`/`try`/`catch`, `noexcept`, exception safety |
| [Coroutines](coroutines/README.md) | `co_await`/`co_yield`/`co_return` (C++20) |
| [Modules](modules/README.md) | `import`/`export` (C++20) |
| [Preprocessor](preprocessor/README.md) | macros, includes, conditional compilation |

> The *standard concepts* live with the [Concepts library](../concepts/README.md);
> the standard *exception types* with [Diagnostics](../diagnostics/README.md).
> Here we cover the language mechanics.
