# Contracts (C++26)

> Express a function's preconditions, postconditions, and internal assertions in
> the language itself, with configurable checking. **Keywords:** `pre`, `post`,
> `contract_assert` · **Since:** C++26

## Mental model

A **contract assertion** states a `bool` predicate that's expected to hold at a
point in execution. Three forms:

```cpp
int factorial(int n)
    pre (n >= 0)              // precondition: checked on entry
    post(r: r >= 1)           // postcondition: `r` names the return value
{
    contract_assert(n < 13);  // assertion: checked at this point in the body
    // ...
}
```

`pre`/`post` are **function contract specifiers** (after the parameter list);
`contract_assert(cond);` is a statement usable anywhere in a body.

## Evaluation semantics — checking is configurable

A contract assertion is evaluated under one of four implementation-chosen
semantics:

| Semantic | Checks predicate? | Terminates on violation? |
|---|---|---|
| `ignore` | no | — |
| `observe` | yes | no (calls the handler, then continues) |
| `enforce` | yes | yes (handler, then terminate) |
| `quick-enforce` | yes | yes (terminate immediately) |

Which one applies is implementation-defined (typically a build flag), so the same
contract can be off in a release build and enforced in a debug/hardened build.

## Contract violations

When a checking semantic finds the predicate `false` (or it throws), a **contract
violation** occurs and the **contract-violation handler** is invoked:

```cpp
// the program's handler (implementation provides a default):
void ::handle_contract_violation(std::contracts::contract_violation);
```

It receives a `std::contracts::contract_violation` describing the failure
(location, kind, the predicate's source text where available). Under `enforce`
the program is then *contract-terminated*; under `observe` execution continues.

## Why use them

- **Document and enforce the API's expectations** at the boundary, in the
  signature, instead of in comments or scattered `assert`s.
- **Tunable cost:** ship with contracts `ignore`d in hot release paths,
  `enforce`d in tests/hardened builds — no source changes.
- A richer replacement for `assert` (`contract_assert`) and for hand-written
  precondition checks.

## Gotchas

- **The predicate may be evaluated zero or more times** (even repeated), and may
  be skipped under `ignore` — so contract predicates must be **side-effect-free**.
  `pre((count++, true))` is a bug: the increment might or might not happen.
- **`post(r: ...)` binds the return value** to a name you choose (here `r`) — that's
  how a postcondition refers to the result.
- **Differing semantics across translation units can break the ODR** if a
  predicate has side effects affecting a constant expression — another reason to
  keep them pure.
- **C++26, very early support.** Feature-test with `__cpp_contracts` (`202502L`);
  this is one of the last C++26 features to land and tooling is nascent. It is
  *not* the older Contracts TS (which was removed from C++20).

## See also

- [Contract assertions — cppreference](https://en.cppreference.com/w/cpp/language/contracts)
- [function contract specifiers (`pre`/`post`) — cppreference](https://en.cppreference.com/w/cpp/language/function-contract-specifier)
- [`contract_assert` statement — cppreference](https://en.cppreference.com/w/cpp/language/contract_assert)
