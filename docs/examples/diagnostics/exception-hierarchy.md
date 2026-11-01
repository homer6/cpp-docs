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

void risky();

void run() {
    try {
        risky();
    } catch (const std::out_of_range& e) {     // most-derived first
        std::cerr << e.what() << '\n';
    } catch (const std::exception& e) {         // catch-all for std exceptions
        std::cerr << "error: " << e.what() << '\n';
    }
}
```

### Throwing

```cpp
void check_index(std::size_t index, std::size_t size) {
    if (index >= size)
        throw std::out_of_range{"index " + std::to_string(index) + " out of range"};
}
```

### Custom exceptions — derive from a standard base

```cpp
class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& msg) : std::runtime_error{msg} {}
};
```

### Capture and rethrow: `std::exception_ptr` (C++11)

An exception can't cross a thread boundary by itself: if it leaves a thread's
function, `std::terminate` is called. Catch it, keep it in a
`std::exception_ptr` and rethrow it on the other side:

```cpp
#include <exception>
#include <stdexcept>
#include <thread>

void run_worker() {
    std::exception_ptr err;                       // null until something is stored
    std::thread worker{[&err] {
        try {
            throw std::runtime_error{"worker failed"};
        } catch (...) {
            err = std::current_exception();       // keeps the exception object alive
        }
    }};
    worker.join();
    if (err)
        std::rethrow_exception(err);              // throws runtime_error here, in this thread
}
```

`exception_ptr` is a nullable, shared-ownership handle: the exception object
lives as long as some `exception_ptr` refers to it. `std::current_exception()`
returns a null one when no exception is being handled.
`std::make_exception_ptr(e)` builds one without a `throw`. This is what a
[`std::promise`](../concurrency/futures.md#promise--future--hand-built-channel)
carries to its future, and what a sender passes to
[`set_error`](../execution/senders-receivers.md):

```cpp
#include <exception>
#include <future>
#include <stdexcept>

std::future<int> failed_result() {
    std::promise<int> promise;
    promise.set_exception(std::make_exception_ptr(std::runtime_error{"no result"}));
    return promise.get_future();                  // get() rethrows the runtime_error
}
```

### Is this scope failing? `std::uncaught_exceptions` (C++17)

`std::uncaught_exceptions()` counts the exceptions thrown but not yet caught in
this thread; once a handler is entered, its exception no longer counts. A
scope guard records the count when it's created and compares in its
destructor:

```cpp
#include <exception>

class Transaction {
    int exceptions_on_entry_ = std::uncaught_exceptions();
public:
    void commit() noexcept;
    void rollback() noexcept;
    ~Transaction() {
        if (std::uncaught_exceptions() > exceptions_on_entry_)
            rollback();                           // this scope is being unwound
        else
            commit();                             // normal exit, even mid-unwinding
    }
};
```

The old `bool std::uncaught_exception()` only said "some unwinding is in
progress". A guard created inside a destructor that runs during unwinding
saw `true` and rolled back, although its own scope was finishing normally.
[N4259](https://wg21.link/n4259) replaced it with the count. It was deprecated
in C++17 and removed in C++20. libc++ no longer declares it; libstdc++ (GCC 15
and 16) still does, with a deprecation warning.

### Inspect without rethrowing: `std::exception_ptr_cast` (C++26)

`std::exception_ptr_cast<E>(p)` gives access to the exception object if a
`catch (const E&)` would match it, with no `throw` or `try`. A null `p` gives
an empty result:

```cpp needs=exception_ptr_cast
#include <exception>
#include <print>
#include <stdexcept>

void log_if_logic_error(const std::exception_ptr& err) {
    if (auto e = std::exception_ptr_cast<std::logic_error>(err))
        std::println("logic error: {}", e->what());   // also matches out_of_range etc.
}
```

C++26 returns `std::optional<const E&>` ([P3981R2](https://wg21.link/p3981r2),
`__cpp_lib_exception_ptr_cast == 202603L`). GCC 16 ships the earlier
[P2927R3](https://wg21.link/p2927r3) form (`202506L`), which returns `const E*`.
The `auto` / `if` / `->` code above works with both. GCC 15 and Clang 22
(libc++) don't have it yet.

### Constexpr exceptions (C++26)

In C++26, `throw` and `catch` work during constant evaluation (see
[throw / try / catch](../language/exceptions/throw-try-catch.md)). The library
follows ([P3068R6](https://wg21.link/p3068r6) and
[P3378R2](https://wg21.link/p3378r2), `__cpp_lib_constexpr_exceptions`
`202502L`): `exception_ptr`, `make_exception_ptr`, `rethrow_exception` and the
standard exception types are `constexpr`, so a `vector::at` that throws
`out_of_range` can be caught inside a constant expression. Of the local
toolchains, only GCC 16 has it.

## Gotchas

- **Catch by `const&`, throw by value.** Catching by value slices derived
  exceptions down to the caught base and loses information.
- **Order catch blocks most-derived first.** A `catch (const std::exception&)`
  placed before a more specific one shadows it.
- **`what()` returns a `const char*` that may be transient** — copy it if you need
  to keep it. Don't put heavy logic in `what()`.
- **Derive custom exceptions from `std::exception`** (usually `runtime_error` or
  `logic_error`) so generic `catch (const std::exception&)` handlers work.
- **`std::rethrow_exception` on a null `exception_ptr` is undefined behaviour.**
  Check it first, as above: `current_exception()` outside a `catch` is null.
- **Don't rely on identity.** Whether `current_exception()` and
  `rethrow_exception()` copy the exception object is unspecified.
- **`exception_ptr_cast` rejects a temporary.** Its `exception_ptr&&` overload
  is deleted, because the result would dangle once the temporary dies.
- **Don't use exceptions for normal control flow.** For expected, recoverable
  failures, [`std::expected`](../utilities/expected.md) (C++23) is often clearer.

## See also

- [std::exception — cppreference](https://en.cppreference.com/w/cpp/error/exception)
- [Standard exception classes (`<stdexcept>`) — cppreference](https://en.cppreference.com/w/cpp/header/stdexcept)
- [std::exception_ptr — cppreference](https://en.cppreference.com/w/cpp/error/exception_ptr)
- [std::uncaught_exceptions — cppreference](https://en.cppreference.com/w/cpp/error/uncaught_exception)
- [std::exception_ptr_cast — cppreference](https://en.cppreference.com/w/cpp/error/exception_ptr_cast)
- [[propagation] — C++ draft](https://eel.is/c++draft/propagation)
- [N4259: Wording for std::uncaught_exceptions](https://wg21.link/n4259)
- [P2927R3: Observing exceptions stored in exception_ptr](https://wg21.link/p2927r3)
- [P3981R2: Better return types in std::inplace_vector and std::exception_ptr_cast](https://wg21.link/p3981r2)
- [P3068R6: Allowing exception throwing in constant-evaluation](https://wg21.link/p3068r6)
- [P3378R2: constexpr exception types](https://wg21.link/p3378r2)
