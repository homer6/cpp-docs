# Selection statements

> `if` and `switch`, plus the modern init-statement and compile-time forms.

## if — with an init-statement (C++17)

Declare a variable scoped to the `if`/`else`, keeping it out of the surrounding
scope:

```cpp
if (auto it = m.find(key); it != m.end()) {
    use(it->second);          // `it` visible in both branches...
} else {
    insert(key);
}                             // ...and gone after the if
```

This is the idiomatic way to handle "look something up, then branch on it."

## if constexpr — compile-time branch (C++17)

The untaken branch is discarded per instantiation — see
[constant expressions](../expressions/constant-expressions.md):

```cpp
template<class T> void f(T v) {
    if constexpr (std::is_pointer_v<T>) use(*v);
    else                                use(v);
}
```

## switch

```cpp
switch (auto code = classify(x); code) {     // init-statement here too (C++17)
    case 1:
    case 2:  handle_low(); break;            // fallthrough between 1 and 2 (no code between)
    case 3:  handle3();
             [[fallthrough]];                 // intentional fallthrough — silences warnings (C++17)
    case 4:  handle4(); break;
    default: handle_default();
}
```

- Cases require **integral or enum** values (not strings, not ranges).
- Execution **falls through** to the next case unless you `break` — the most
  common `switch` bug.

## Gotchas

- **Missing `break` falls through.** Annotate deliberate fallthrough with
  `[[fallthrough]];` (C++17) so it's visibly intentional and warning-free.
- **Declaring a variable in a `case` without braces** can be ill-formed (it could
  be skipped by another label). Wrap case bodies in `{ }` when they declare
  variables.
- **`switch` only matches integers/enums.** For strings, use `if`/`else` chains, a
  `map`, or hashing.
- **Use the C++17 init-statement** (`if (init; cond)`) to scope helpers tightly —
  cleaner than declaring before the `if`.
- **`if constexpr` vs `if`:** use `if constexpr` only when you need branches to be
  *compiled* conditionally (templates); a runtime `if` is right otherwise.

## See also

- [if statement — cppreference](https://en.cppreference.com/w/cpp/language/if)
- [switch statement — cppreference](https://en.cppreference.com/w/cpp/language/switch)
