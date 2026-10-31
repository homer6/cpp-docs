# Attributes

> Standardized annotations in `[[ ]]` that convey intent to the compiler — warnings,
> optimization hints, and suppression. **Since:** C++11 (more added through C++23)

## The standard attributes

```cpp
enum class Status { ok, failed };

[[nodiscard]] int compute();              // warn if the return value is ignored
[[nodiscard("check the status")]]         //   with a reason (C++20)
Status run();

[[maybe_unused]] int debug_only;          // suppress "unused" warnings
void f(int x [[maybe_unused]]);

[[noreturn]] void fail();                  // never returns (throws / exits / loops forever)

[[deprecated]] void old_api();             // warn on use
[[deprecated("use new_api()")]] void g();
```

The rest attach to statements, so they live inside a function body:

```cpp body
int x = 1;
switch (x) {
    case 1: ++x; [[fallthrough]];          // intentional switch fallthrough (C++17)
    case 2: break;
}

if (x > 0) [[likely]] { /* ... */ }        // branch-probability hints (C++20)
else       [[unlikely]] { /* ... */ }

[[assume(x > 0)]];                         // tell the optimizer to assume a condition (C++23)
```

## Common uses

```cpp
struct Error { std::string message; };
void hot_path(); void rare_path(); void setup(); void run();

[[nodiscard]] std::expected<int, Error> parse(std::string_view);   // don't drop the result

[[noreturn]] void terminate_now();          // callers know control never returns

void dispatch(const int* ptr, int n) {
    if (ptr) [[likely]] { hot_path(); }    // hint the common branch
    else     [[unlikely]] { rare_path(); }

    switch (n) {
        case 1: setup(); [[fallthrough]];  // silences the fallthrough warning
        case 2: run(); break;
    }
}
```

## Gotchas

- **`[[nodiscard]]` is high-value** on functions whose result must be used — error
  codes, `expected`, factory results, `empty()` (which people confuse with
  "clear"). Apply it liberally; it catches real bugs at compile time.
- **Hints are not guarantees.** `[[likely]]`/`[[unlikely]]` and `[[assume]]` only
  *advise* the optimizer. Worse, `[[assume(cond)]]` with a `cond` that's actually
  false is **undefined behavior** — only assume things that are truly invariant.
- **Unknown attributes are ignored, not errors.** A compiler may silently skip an
  attribute it doesn't recognize (vendor `[[gnu::...]]`/`[[msvc::...]]` ones are
  namespaced) — so a typo'd standard attribute can do nothing silently.
- **Placement matters** — attributes attach to the entity immediately following
  (or preceding, depending on form); check cppreference for where each one goes.

## See also

- [Attribute specifier sequence — cppreference](https://en.cppreference.com/w/cpp/language/attributes)
- [`[[nodiscard]]` — cppreference](https://en.cppreference.com/w/cpp/language/attributes/nodiscard)
- [`[[likely]]` / `[[unlikely]]` — cppreference](https://en.cppreference.com/w/cpp/language/attributes/likely)
