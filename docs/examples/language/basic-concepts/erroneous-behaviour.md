# Erroneous behaviour

> Reading an uninitialized local variable is no longer undefined behaviour: it
> is **erroneous behaviour**, a defined-but-wrong operation the implementation
> may diagnose. **Since:** C++26 (P2795R5)

## Mental model

```
                 C++23                       C++26
int x;           indeterminate value         erroneous value (implementation-chosen)
return x;        undefined behaviour         erroneous behaviour: still a bug, but bounded
```

Erroneous behaviour is a new category between "defined" and "undefined". The
program reads *some* value the implementation chose, so the optimizer can't
assume the read never happens and remove code around it. It is still a bug: the
implementation is encouraged to diagnose it and may terminate the program.

## What gets an erroneous value

| Storage | Uninitialized bytes hold | Reading them is |
|---|---|---|
| Automatic (locals, including members of a local struct) | an erroneous value | erroneous behaviour |
| Automatic, marked `[[indeterminate]]` | an indeterminate value | undefined behaviour |
| Dynamic (`new int`, `std::malloc`) | an indeterminate value | undefined behaviour |
| Static / thread-local | zero (zero-initialized before anything else) | fine |

```cpp body
int x;                      // erroneous value
struct P { int a; int b = 7; };
P p;                        // p.a erroneous, p.b == 7
int* h = new int;           // *h indeterminate: reading it is still UB
delete h;
```

## Opting out: `[[indeterminate]]`

For a measured hot path (a large buffer filled straight away), the attribute
restores the old semantics on one block variable or function parameter. It
goes after the name, and for a parameter it must be on the first declaration:

```cpp
void fill(char* out, int n);
void read_packet(int len [[indeterminate]]);    // on a parameter

void handle() {
    char buf [[indeterminate]] [4096];          // skip the implicit initialization
    fill(buf, sizeof buf);                      // must write before any read
}
```

GCC 15 and Clang 22 don't know the attribute and ignore it with a warning
(`-Wattributes` / `-Wunknown-attributes`), so it's safe to write today.

## Compiler support (checked locally)

| Compiler, `-std=c++26` | Behaviour |
|---|---|
| GCC 16 | Implements it. `__has_cpp_attribute(indeterminate) == 202403L`. Uninitialized locals read as `0`, at `-O0` and `-O2`; `[[indeterminate]]` locals keep whatever was on the stack. `-ftrivial-auto-var-init=pattern` overrides the value (`0xFEFEFEFE` for an `int`). |
| GCC 15, Clang 22 | Not implemented: uninitialized locals are indeterminate, as in C++23. |

All three warn with `-Wall` (`-Wuninitialized`) when they can see the read.
There is no feature-test macro for erroneous behaviour itself; test for the
attribute.

## Gotchas

- **Still a bug.** Erroneous behaviour limits the damage; it doesn't make the
  value meaningful. Initialize every variable
  ([ES.20](../../../guidelines/safety.md#c26-erroneous-behaviour)).
- **Don't rely on the value.** Zero on GCC 16 is that implementation's choice;
  another may use a pattern or trap. And if the chosen value isn't valid for
  the type (a `bool` that is neither `false` nor `true`), the behaviour is
  undefined after all.
- **Heap memory isn't covered.** `new T` for a trivial `T` and `std::malloc`
  still give indeterminate bytes; use `new T()` / `new T{}` to value-initialize.
- **`[[indeterminate]]` brings UB back.** Read such a variable before writing it
  and you're in C++23 territory again.
- **Not a substitute for tools.** Sanitizers (MemorySanitizer, Valgrind) still
  find the read; `-ftrivial-auto-var-init` is a compiler flag, not the language
  rule.

## See also

- [Undefined behavior (erroneous behavior) — cppreference](https://en.cppreference.com/w/cpp/language/ub)
- [`[[indeterminate]]` — cppreference](https://en.cppreference.com/w/cpp/language/attributes/indeterminate)
- [Default initialization — cppreference](https://en.cppreference.com/w/cpp/language/default_initialization)
- [P2795R5: Erroneous behaviour for uninitialized reads](https://wg21.link/p2795r5)
- [[basic.indet] in the C++ draft](https://eel.is/c++draft/basic.indet)
