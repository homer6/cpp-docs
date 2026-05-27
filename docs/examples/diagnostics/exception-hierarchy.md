# The standard exception hierarchy

> All standard exceptions derive from `std::exception`, so one `catch` can handle
> them by reference. **Headers:** `<exception>`, `<stdexcept>` · **Since:** C++98

## The tree

```
 std::exception
 ├─ std::logic_error          (<stdexcept>)  — bugs: violated preconditions
 │   ├─ std::invalid_argument
 │   ├─ std::domain_error
 │   ├─ std::length_error
 │   └─ std::out_of_range      (e.g. vector::at, map::at)
 ├─ std::runtime_error        (<stdexcept>)  — conditions only knowable at runtime
 │   ├─ std::range_error
 │   ├─ std::overflow_error
 │   ├─ std::underflow_error
 │   └─ std::system_error      (<system_error>) — carries an error_code
 ├─ std::bad_alloc            — allocation failed
 ├─ std::bad_cast             — failed dynamic_cast to reference
 ├─ std::bad_optional_access  — optional::value() when empty
 ├─ std::bad_variant_access   — wrong variant alternative
 └─ std::bad_function_call    — calling an empty std::function
```

`logic_error` = a programming mistake (could have been prevented);
`runtime_error` = an external condition you couldn't predict.

## Examples

### Catch by const reference

```cpp
#include <stdexcept>

try {
    risky();
} catch (const std::out_of_range& e) {     // most-derived first
    std::cerr << e.what() << '\n';
} catch (const std::exception& e) {         // catch-all for std exceptions
    std::cerr << "error: " << e.what() << '\n';
}
```

### Throwing

```cpp
if (index >= size)
    throw std::out_of_range{"index " + std::to_string(index) + " out of range"};
```

### Custom exceptions — derive from a standard base

```cpp
class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& msg) : std::runtime_error{msg} {}
};
```

## Gotchas

- **Catch by `const&`, throw by value.** Catching by value slices derived
  exceptions down to the caught base and loses information.
- **Order catch blocks most-derived first.** A `catch (const std::exception&)`
  placed before a more specific one shadows it.
- **`what()` returns a `const char*` that may be transient** — copy it if you need
  to keep it. Don't put heavy logic in `what()`.
- **Derive custom exceptions from `std::exception`** (usually `runtime_error` or
  `logic_error`) so generic `catch (const std::exception&)` handlers work.
- **Don't use exceptions for normal control flow.** For expected, recoverable
  failures, [`std::expected`](../utilities/expected.md) (C++23) is often clearer.

## See also

- [std::exception — cppreference](https://en.cppreference.com/w/cpp/error/exception)
- [Standard exception classes (`<stdexcept>`) — cppreference](https://en.cppreference.com/w/cpp/header/stdexcept)
