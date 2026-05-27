# Diagnostics library

Reporting and inspecting errors: the standard exception types, OS/library error
codes, and stack traces. Mirrors
[cppreference's diagnostics library](https://en.cppreference.com/w/cpp/error).

| Page | Header | Since | What |
|---|---|---|---|
| [Exception hierarchy](exception-hierarchy.md) | `<stdexcept>`, `<exception>` | C++98 | the standard `std::exception` tree |
| [`system_error`](system_error.md) | `<system_error>` | C++11 | `error_code`, `errc`, OS error reporting |
| [`stacktrace`](stacktrace.md) | `<stacktrace>` | C++23 | capture and print a call stack |

> The *language mechanics* of `throw`/`try`/`catch`/`noexcept` and exception
> safety are covered under [Language → Exceptions](../language/exceptions/README.md).
> This area is about the standard **error types**.
