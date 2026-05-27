# std::error_code & std::system_error

> Report failures from the OS and libraries without exceptions, via a portable
> `(value, category)` pair. **Header:** `<system_error>` · **Since:** C++11

## Mental model

`std::error_code` = an integer **value** plus an **error category** (which
namespace the value belongs to: OS errno, a library's own codes, etc.). This lets
unrelated subsystems report errors through one type without colliding. It's the
non-throwing counterpart to exceptions — many standard APIs (filesystem,
networking-style code) take an `error_code&` out-parameter.

## Examples

### The error_code overload pattern

```cpp
#include <system_error>
#include <filesystem>
namespace fs = std::filesystem;

std::error_code ec;
fs::create_directory("out", ec);        // non-throwing overload
if (ec) {                                // error_code is truthy iff an error occurred
    std::cerr << ec.message()            // human-readable text
              << " (" << ec.value() << ", " << ec.category().name() << ")\n";
}
```

### Comparing against portable conditions

`std::errc` is an enum of portable error *conditions*; compare an `error_code` to
it without caring about the platform's raw numbers:

```cpp
if (ec == std::errc::no_such_file_or_directory) { /* handle missing file */ }
if (ec == std::errc::permission_denied)         { /* ... */ }
```

### Throwing form: std::system_error

```cpp
throw std::system_error{ec, "while opening config"};   // wraps an error_code as an exception
// or from errno:
throw std::system_error{errno, std::generic_category(), "open failed"};
```

## error_code vs error_condition

- **`error_code`** — the *concrete* error from a specific source (a platform errno
  value in its category).
- **`error_condition`** — a *portable* abstraction you compare against
  (`std::errc`). The category's equivalence rules map codes to conditions, so
  `ec == std::errc::permission_denied` works across platforms.

## Gotchas

- **`error_code` is truthy on *failure*** (non-zero) — `if (ec)` means "something
  went wrong," the opposite of a success bool.
- **Compare to `std::errc`, not raw numbers**, for portability — the underlying
  integer differs by platform/category.
- **Prefer the `error_code` overload for *expected* failures** (missing file, no
  permission) and reserve exceptions/`system_error` for unexpected ones — same
  philosophy as [`std::expected`](../utilities/expected.md).
- **Define your own category** (derive from `std::error_category`) to plug library
  error codes into this machinery.

## See also

- [std::error_code — cppreference](https://en.cppreference.com/w/cpp/error/error_code)
- [std::errc — cppreference](https://en.cppreference.com/w/cpp/error/errc)
- [std::system_error — cppreference](https://en.cppreference.com/w/cpp/error/system_error)
