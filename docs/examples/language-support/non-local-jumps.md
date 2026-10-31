# Non-local jumps

> `setjmp` saves the current execution context; a later `longjmp` returns to it
> from any depth of nested calls, without unwinding the stack. **Header:**
> `<csetjmp>` · **Since:** C++98 (inherited from C)

## Mental model

`setjmp(env)` returns **twice**: once normally with `0`, and again — possibly
much later — when some deeper function calls `longjmp(env, v)`, this time
returning `v`. The frames in between are discarded, not unwound: no
destructors run, no `catch` blocks see anything.

```
main ─ setjmp(env) == 0 ─▶ parse() ─▶ expr() ─▶ term() ── longjmp(env, 42)
  ▲                                                          │
  └──── setjmp(env) "returns" 42 ◀── frames thrown away ─────┘  (no destructors!)
```

In C++, exceptions do the same job safely. `setjmp`/`longjmp` remain for
interoperating with C libraries that use them for error recovery (libpng and
libjpeg are the well-known ones) and for C code compiled as C++.

## Declaration

```cpp nocompile
using jmp_buf = /* array type */;
#define setjmp(env) /* int: 0 directly, non-zero after longjmp — a macro */
[[noreturn]] void longjmp(std::jmp_buf env, int status);   // status 0 becomes 1
```

`setjmp` may only appear as:

- the entire controlling expression of `if`, `switch`, `while`, `do`, `for`;
- one operand of `==`, `!=`, `<`, … whose other operand is an integer constant,
  as the entire controlling expression;
- the operand of `!` as the entire controlling expression;
- an entire expression statement (result discarded).

`int rc = setjmp(env);` is **not** on that list and is undefined behaviour.
Use `switch (setjmp(env))` to get the value.

## When to use it

- **A C library's error callback requires it** (it calls `longjmp` back into
  your code, or expects you to). Keep the jumping region free of C++ objects
  with non-trivial destructors.

Anti-fits: everything else. Use exceptions, `std::expected`, or return codes.
Don't use it to implement coroutines or cancellation — C++20 coroutines and
`std::stop_token` exist.

## Examples

### Escaping from deep recursion

```cpp
#include <csetjmp>
#include <cstdio>

namespace { std::jmp_buf g_env; }

int depth(int n) {
    if (n == 3) std::longjmp(g_env, 42);     // abandon all frames up to main
    return depth(n + 1) + 1;
}

int main() {
    volatile int attempts = 0;               // volatile: keeps its value after longjmp
    switch (setjmp(g_env)) {                 // allowed context: controlling expression
    case 0:
        attempts = attempts + 1;
        depth(0);
        std::puts("not reached");
        break;
    case 42:
        std::printf("jumped back, attempts=%d\n", int(attempts));
        break;
    }
}
```

```
jumped back, attempts=1
```

### `longjmp(env, 0)` returns 1

```cpp
#include <csetjmp>
#include <cstdio>

int main() {
    std::jmp_buf env;
    if (setjmp(env) == 0)
        std::longjmp(env, 0);
    else
        std::puts("setjmp returned non-zero (1)");
}
```

## Gotchas

- **Non-`volatile` locals changed after `setjmp` are indeterminate after the
  jump.** The program below printed `plain=0 vol=7` at `-O2` and `plain=7 vol=7`
  at `-O0` with both GCC 15 and Clang 22: the optimizer kept `plain` in a
  register that `longjmp` restored. Make such variables `volatile`.

  ```cpp
  #include <csetjmp>
  #include <cstdio>
  std::jmp_buf env;
  [[gnu::noinline]] void deep() { std::longjmp(env, 1); }
  int main() {
      int plain = 0;
      volatile int vol = 0;
      if (setjmp(env) == 0) { plain = 7; vol = 7; deep(); }
      else std::printf("plain=%d vol=%d\n", plain, int(vol));  // plain is indeterminate
  }
  ```
- **Skipping destructors is undefined behaviour.** If replacing the `longjmp`
  with a `throw` caught at the `setjmp` point would run any non-trivial
  destructor, the `longjmp` has undefined behaviour. A `std::string`, `std::vector`
  or `std::lock_guard` in an abandoned frame means leaks at best.
- **Don't jump into a frame that has returned.** The `jmp_buf` is only valid
  while the function that called `setjmp` is still running.
- **Never `longjmp` across a `catch`, a coroutine, or a thread boundary.**
- **Signal masks**: whether `longjmp` restores the signal mask is
  platform-specific; POSIX offers `sigsetjmp`/`siglongjmp` to control it.
- **`setjmp` is a macro** — `std::setjmp` doesn't exist; write `setjmp`
  unqualified. (`std::jmp_buf` and `std::longjmp` are in `std`.)

## See also

- [`setjmp` — cppreference](https://en.cppreference.com/w/cpp/utility/program/setjmp)
- [`std::longjmp` — cppreference](https://en.cppreference.com/w/cpp/utility/program/longjmp)
- [Exceptions](../language/exceptions/README.md) — the C++ way to unwind
- [Signal handling](signals.md)
