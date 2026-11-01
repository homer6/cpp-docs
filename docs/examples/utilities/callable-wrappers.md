# Callable wrappers

> Type-erased holders for "something callable with this signature":
> `std::function`, `std::move_only_function`, `std::copyable_function` and the
> non-owning `std::function_ref`. **Header:** `<functional>` · **Since:** C++11
> (`function`), C++23 (`move_only_function`), C++26 (`copyable_function`,
> `function_ref`)

## Mental model

A wrapper erases the callable's type behind one signature, so a lambda, a free
function and a functor can all sit in the same variable. The four differ in two
ways: whether they **own** the callable, and what they require of it.

```
owning (stores a copy)                         non-owning (points at it)
  function            copyable, const op() lies   function_ref  borrowed; 2 words, no alloc
  move_only_function  move-only OK, qualifiers
  copyable_function   copyable, qualifiers
```

## Which wrapper?

| Wrapper | Owns? | Callable must be | Wrapper copyable? | Signature qualifiers | Empty call |
|---|---|---|---|---|---|
| `function` (C++11) | yes | copyable | yes | none (`operator()` is `const` regardless) | throws `bad_function_call` |
| `move_only_function` (C++23) | yes | movable | no | `const`, `&`, `&&`, `noexcept` | UB |
| `copyable_function` (C++26) | yes | copyable | yes | `const`, `&`, `&&`, `noexcept` | UB |
| `function_ref` (C++26) | no | anything that outlives it | yes, trivially | `const`, `noexcept` | can't be empty |

- **A parameter you call and don't keep:** `function_ref` (or a template).
- **A callback you store:** `move_only_function`, or `copyable_function` if
  the holder must be copyable. Prefer either to `function` in new code.
- **A known callable type in a hot loop:** a template parameter, no wrapper.

## Examples

### std::function

Holds any copyable callable matching the signature. Costs type erasure (often a
heap allocation plus an indirect call).

```cpp body
#include <functional>

std::function<int(int, int)> op = [](int a, int b){ return a + b; };
op = std::multiplies<int>{};           // reassign to a different callable
int r = op(6, 7);                       // 42
if (op) { /* non-empty */ }             // empty std::function → calling it throws std::bad_function_call
```

### std::move_only_function (C++23)

`std::function` requires the callable be *copyable*. `std::move_only_function`
drops that — it can hold move-only callables (e.g. a lambda capturing a
`unique_ptr`), and supports `const`/`noexcept`/ref qualifiers in its signature.

```cpp
std::move_only_function<void()> task =
    [p = std::make_unique<int>(5)]{ /* uses *p */ };   // capture is move-only
```

### std::copyable_function (C++26)

The copyable counterpart of `move_only_function` (P2548): same qualifier
support, but the callable and the wrapper are copyable. Its `const` qualifier
means what it says, so a `mutable` lambda is rejected where `std::function`
would accept it and call it through a `const` `operator()`. Only GCC 16
ships it (`__cpp_lib_copyable_function` 202306L); Clang 22 and GCC 15 don't.

```cpp needs=copyable_function
#include <functional>
#include <type_traits>

std::copyable_function<int(int) const> add3 = [k = 3](int x){ return x + k; };
auto copy = add3;                                  // copies the lambda: copy(1) == 4
std::move_only_function<int(int) const> m = add3;  // converts to move_only_function

using Counter = decltype([n = 0]() mutable { return ++n; });
static_assert(!std::is_constructible_v<std::copyable_function<int() const>, Counter>);
static_assert( std::is_constructible_v<std::function<int()>, Counter>);  // function allows it
```

### std::function_ref (C++26)

A non-owning reference to a callable (P0792): typically two pointers (16 bytes
on GCC 16), never allocates,
trivially copyable, and has no empty state (no default constructor). It's the
right type for a callback parameter that's called during the call and not
kept. Only GCC 16 ships it (`__cpp_lib_function_ref` 202603L).

```cpp needs=function_ref
#include <functional>

int twice(int x) { return 2 * x; }
int apply(std::function_ref<int(int)> f, int x) { return f(x); }

struct Counter { int n = 0; int add(int k) { return n += k; } };

void demo() {
    int base = 10;
    auto plus_base = [&](int x) { return base + x; };
    apply(plus_base, 1);                 // 11: refers to the lambda, no copy
    apply(twice, 4);                     // 8: a function works too

    std::function_ref<int(int)> r{std::cw<twice>};  // function bound at compile time
    Counter c;
    std::function_ref<int(int)> add{std::cw<&Counter::add>, c};  // member + object
    add(2); add(3);                      // c.n == 5
}
```

`std::cw<f>` (a `std::constant_wrapper`) binds the target as a constant, so the
`function_ref` stores only the object pointer. Earlier drafts and papers spell
it `std::nontype<f>`; P3948 (March 2026) replaced that with `constant_wrapper`
and bumped the macro to 202603L. GCC 16 has only `std::cw`.

## Gotchas

- **`function_ref` dangles if you keep it.** It points at its target, so a
  `function_ref` initialised from a temporary lambda dangles at the end of the
  full-expression. Use it for parameters, not members or locals that outlive
  the callable.

  ```cpp nocompile
  std::function_ref<int(int)> r = [](int x) { return x; };  // temporary dies here
  r(1);                                                      // UB: dangling
  ```

- **Calling an empty `std::function` throws; an empty `move_only_function` or
  `copyable_function` is UB.** Check it in a bool context first.
- **`std::function`'s `const` `operator()` calls non-`const` targets.** A
  `const std::function` can still mutate a `mutable` lambda's state; the C++23
  and C++26 wrappers make `const` part of the signature instead.
- **Wrappers aren't free.** Type erasure usually means an indirect call, and
  the owning ones may allocate. For a known callable type, take a template
  parameter or a constrained `auto&&`.

## See also

- [Function objects](functional.md): `invoke`, `bind_front`, `not_fn`, `mem_fn`
- [std::function — cppreference](https://en.cppreference.com/w/cpp/utility/functional/function)
- [std::move_only_function — cppreference](https://en.cppreference.com/w/cpp/utility/functional/move_only_function)
- [std::copyable_function — cppreference](https://en.cppreference.com/w/cpp/utility/functional/copyable_function)
- [std::function_ref — cppreference](https://en.cppreference.com/w/cpp/utility/functional/function_ref)
- [P0792R14: function_ref](https://wg21.link/P0792R14)
- [P2548R6: copyable_function](https://wg21.link/P2548R6)
- [P3948R1: constant_wrapper is the only tool needed for passing constant expressions via function arguments](https://wg21.link/P3948R1)
