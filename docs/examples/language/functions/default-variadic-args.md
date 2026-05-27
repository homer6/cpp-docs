# Default & variadic arguments

> Omitting trailing arguments, and accepting an arbitrary number of them.

## Default arguments

```cpp
void connect(std::string_view host, int port = 8080, bool tls = true);

connect("example.com");                 // port=8080, tls=true
connect("example.com", 443);            // tls=true
connect("example.com", 443, false);
```

- Only **trailing** parameters can have defaults (no gaps).
- Put the default in the **declaration** (header), not the definition, and only
  once.

## Variadic templates — the modern "any number of args"

A **parameter pack** holds zero or more arguments of varying types:

```cpp
template<class... Args>                  // Args is a type pack
void log(Args&&... args) {               // args is a function parameter pack
    (std::cout << ... << args) << '\n';  // fold expression — see below
}
log("x=", 42, " y=", 3.14);
```

`sizeof...(args)` gives the count. Packs are expanded with `...`.

## Fold expressions (C++17)

Reduce a pack with a binary operator — no recursion needed:

```cpp
template<class... T> auto sum(T... xs)   { return (xs + ...); }        // x0 + (x1 + ...)
template<class... T> bool all(T... bs)   { return (bs && ...); }
template<class... T> void print(T... xs) { ((std::cout << xs << ' '), ...); }  // comma fold
```

Forms: `(pack op ...)`, `(... op pack)`, `(pack op ... op init)`,
`(init op ... op pack)`.

## C-style varargs (avoid)

```cpp
int old_printf(const char* fmt, ...);    // C varargs: not type-safe — avoid in new code
```

Prefer variadic templates (type-safe) or [`std::format`](../../text-processing/formatting.md)
over `<cstdarg>` varargs.

## Gotchas

- **Default arguments are evaluated at the *call site*** each call, and live in the
  declaration — redefining them in the definition (or twice) is an error.
- **Defaults + overloading can be ambiguous.** A defaulted param can make two
  overloads viable for the same call; prefer overloads or clear defaults, not
  both.
- **Pack expansion `...` placement matters.** `f(args...)` forwards all;
  `f(g(args)...)` applies `g` to each. Misplacing `...` is a common compile error.
- **Forward packs with `std::forward<Args>(args)...`** to preserve value
  categories (perfect forwarding) — see
  [value categories](../expressions/value-categories.md).
- **Avoid C varargs** (`...` with `va_list`): no type checking, UB on type
  mismatch.

## See also

- [Default arguments — cppreference](https://en.cppreference.com/w/cpp/language/default_arguments)
- [Parameter pack — cppreference](https://en.cppreference.com/w/cpp/language/parameter_pack)
- [Fold expressions — cppreference](https://en.cppreference.com/w/cpp/language/fold)
