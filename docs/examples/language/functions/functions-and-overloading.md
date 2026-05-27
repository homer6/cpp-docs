# Functions & overloading

> Declaring functions, how parameters should be passed, and how the compiler picks
> among overloads.

## Passing parameters — the guidelines

```cpp
void read(const std::string& s);     // read-only, possibly large: const reference
void read(std::string_view s);        // read-only string, no ownership: even better for text
void mutate(std::string& s);          // modify the caller's object: non-const reference
void take(std::string s);             // need your own copy / will move: by value (then move from it)
void cheap(int x);                    // small/trivial types: by value
```

Rules of thumb: **by value** for small/trivial or when you need a copy you'll move
from; **`const&`** for large read-only inputs; **`&`** to modify; **`string_view`/
`span`** for non-owning views.

## Overloading

Multiple functions, same name, distinguished by parameter types:

```cpp
void print(int);
void print(double);
void print(const std::string&);
print(42);        // → print(int)
print(3.14);      // → print(double)
```

Overloads differ by parameter **types/count**, not by return type alone. The
compiler runs **overload resolution** to pick the best match (exact > promotion >
conversion); ambiguity is an error.

## Attributes that matter

```cpp
[[nodiscard]] int compute();          // warn if the return value is ignored
[[nodiscard("check the error")]] Status run();   // with a reason (C++20)
[[noreturn]] void fail();              // never returns (throws/exits)
[[deprecated("use g()")]] void f();
void g(int x [[maybe_unused]]);
```

## Default vs deleted

```cpp
struct S {
    S() = default;                     // ask for the compiler-generated one
    S(const S&) = delete;              // forbid copying (deleted function)
};
void f(double) = delete;               // ban a specific overload (e.g. block implicit int→double)
```

## Gotchas

- **`const&` to avoid copies, but `string_view`/`span` are often better** for
  read-only views — no dependency on the exact container/string type.
- **Don't overload on subtly-convertible types** (`f(int)` vs `f(long)` vs
  `f(size_t)`) — calls become ambiguous or pick surprising overloads. `= delete`
  can block unwanted implicit conversions.
- **Return type alone can't distinguish overloads.** `int f();` and `double f();`
  is an error.
- **Use `[[nodiscard]]`** on functions whose result must be used (error codes,
  `expected`, factory results) — it catches dropped returns at compile time.
- **Beware accidental copies in `by value` params for big types** — pass `const&`
  unless you genuinely need a copy.

## See also

- [Functions — cppreference](https://en.cppreference.com/w/cpp/language/functions)
- [Overload resolution — cppreference](https://en.cppreference.com/w/cpp/language/overload_resolution)
- [Attributes — cppreference](https://en.cppreference.com/w/cpp/language/attributes)
