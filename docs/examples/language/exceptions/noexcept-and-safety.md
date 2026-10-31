# noexcept & exception safety

> Declaring that a function won't throw, and reasoning about what state your code
> leaves behind when something does.

## noexcept — "this won't throw"

```cpp
void cleanup() noexcept;                 // promises not to throw
int  compute() noexcept(false);           // explicitly may throw (the default)

template<class T>
void swap(T& a, T& b) noexcept(std::is_nothrow_move_constructible_v<T>);  // conditional
```

If a `noexcept` function *does* throw, the program calls `std::terminate` — no
unwinding. It's a hard promise, not a hint.

## The noexcept operator

`noexcept(expr)` is a compile-time `bool`: `false` if `expr` could throw
(it calls something not declared non-throwing, or contains a `throw`),
otherwise `true`. Like `sizeof`, it doesn't evaluate `expr`.

```cpp
void may_throw();
void wont_throw() noexcept;

static_assert(!noexcept(may_throw()));
static_assert( noexcept(wont_throw()));
static_assert( noexcept(1 + 1));                               // built-ins don't throw
static_assert(!noexcept(std::vector<int>{}.push_back(1)));     // may allocate

template<class T>                                              // the noexcept(noexcept(...)) idiom
void exchange_values(T& a, T& b) noexcept(noexcept(T(std::move(a))) && noexcept(a = std::move(b)));
```

The outer `noexcept(...)` is the specifier, the inner one the operator: the
function promises not to throw exactly when the moves it does can't.

## Why noexcept matters for moves

The standard library checks `noexcept` to decide whether it can safely move. The
big example: when `std::vector` reallocates, it **moves** elements only if their
move constructor is `noexcept` — otherwise it **copies** (to preserve the strong
guarantee). So a non-`noexcept` move can silently make `vector` growth far slower.

```cpp
struct Gadget {
    Gadget(Gadget&&) noexcept;            // mark moves noexcept → vector moves, not copies
    Gadget& operator=(Gadget&&) noexcept;
};
```

Destructors are implicitly `noexcept` — keep them that way.

## The exception-safety guarantees

A function's behavior when an exception propagates through it:

| Guarantee | Meaning |
|---|---|
| **No-throw** | never throws (`noexcept`) |
| **Strong** | if it throws, state is unchanged — as if the call never happened (commit-or-rollback) |
| **Basic** | if it throws, all invariants hold and nothing leaks, but state may have changed |
| **None** | (avoid) may leak or corrupt on throw |

Aim for at least **basic** everywhere; **strong** for operations that must be
transactional.

## Techniques

```cpp
// RAII makes the basic guarantee almost free — resources release on unwind:
void fill(std::mutex& m, std::size_t n) {
    std::lock_guard lk{m};
    auto buf = std::make_unique<char[]>(n);   // no leak if the next line throws
    // ...
}

// copy-and-swap gives the strong guarantee for assignment:
class Buffer {
    std::vector<int> data_;
public:
    Buffer() = default;
    Buffer(const Buffer&) = default;
    Buffer(Buffer&&) noexcept = default;
    friend void swap(Buffer& a, Buffer& b) noexcept { a.data_.swap(b.data_); }

    Buffer& operator=(Buffer other) noexcept {  // copy made before touching *this
        swap(*this, other);                     // swap is noexcept → commit can't fail
        return *this;
    }                                           // old state destroyed via `other`
};
```

## Gotchas

- **Mark move ctor/assign and `swap` `noexcept`.** It's the difference between
  `vector`/`string` moving vs. copying your elements on growth — a quiet
  performance cliff otherwise.
- **A `noexcept` function that throws = `std::terminate`.** Don't slap `noexcept`
  on something that might throw to "optimize"; it's a correctness promise.
- **Lean on RAII for the basic guarantee.** Manual `try`/`catch` cleanup is
  error-prone; let destructors do the work.
- **Use copy-and-swap (or build-then-commit) for the strong guarantee** — do all
  the throwing work on a copy, then swap in with a `noexcept` step.
- **`noexcept` is part of a function's type** (since C++17) — it affects function
  pointers and overload sets.

## See also

- [noexcept specifier — cppreference](https://en.cppreference.com/w/cpp/language/noexcept_spec)
- [noexcept operator — cppreference](https://en.cppreference.com/w/cpp/language/noexcept)
- [Exceptions (safety) — cppreference](https://en.cppreference.com/w/cpp/language/exceptions)
