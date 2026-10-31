# Jump statements

> Unconditional transfers of control: `break`, `continue`, `return`, `goto` (and
> `co_return` in coroutines). Leaving a scope this way still runs destructors.
> **Since:** C++98 (`co_return`: C++20)

## Mental model

```
for (...) {            <-------------------+
    ...                                    |
    continue;  ---- skip to the end of ----+  (then the loop's increment/condition)
    ...
    break;     ---- leave the innermost loop or switch -----+
}                                                           |
<-----------------------------------------------------------+

return x;      ---- leave the function; locals destroyed in reverse order
goto label;    ---- jump anywhere in the same function (labels have function scope)
```

Every jump that leaves a scope destroys the automatic variables of that scope,
in reverse order of construction — exactly as falling off the end of the block
would. That's why RAII works with early `return` and `break`.

## Declaration

| Statement | Valid in | Effect |
|---|---|---|
| `break;` | loop, `switch`, `template for` (C++26) | leave the innermost enclosing one |
| `continue;` | loop, `template for` (C++26) | jump to the end of the innermost loop body |
| `return;` / `return expr;` / `return {…};` | function | leave the function, initialising the result |
| `goto identifier;` | function | jump to `identifier:` in the same function |
| `co_return;` / `co_return expr;` | coroutine | finish the coroutine — see [coroutines](../coroutines/coroutines.md) |

A **label** is `identifier:` before a statement. Labels have their own name space
and **function scope**: a `goto` can name a label that appears later, but never
one in another function. `case` and `default` are labels too (only for
`switch`).

## When to use it

- `break` / `continue` — for loop control. If a loop needs several of them,
  consider an [algorithm](../../algorithms/README.md) (`std::ranges::find_if`,
  `any_of`) instead.
- Early `return` — guard clauses at the top of a function keep the main path
  unindented, and destructors make it safe.
- `goto` — rarely. The defensible uses are breaking out of nested loops (C++
  has no labelled `break`) and C-style cleanup chains in code that can't use
  RAII. Prefer extracting the loops into a function and `return`ing.

## Examples

### `break` and `continue`

```cpp
#include <span>

constexpr int first_negative(std::span<const int> xs) {
    int index = 0;
    for (int x : xs) {
        if (x >= 0) { ++index; continue; }   // next element
        break;                                // found one: stop
    }
    return index < static_cast<int>(xs.size()) ? index : -1;
}

constexpr int data[] = {3, 1, -4, 1};
static_assert(first_negative(data) == 2);
```

In a `switch` inside a loop, `break` leaves the `switch`, not the loop;
`continue` goes to the loop.

### `return`

```cpp
#include <string>
#include <vector>

std::vector<int> make_ids() { return {1, 2, 3}; }       // copy-list-initialises the result
void log_line(const std::string&) {}
void log_twice(const std::string& s) { log_line(s); return log_line(s); }  // void expression: OK

std::string greet(bool formal) {
    std::string s = formal ? "Good day" : "Hi";
    return s;   // implicitly moved (or elided) — don't write std::move(s)
}
```

- Flowing off the end of a non-`void` function is **undefined behaviour**,
  except in `main` (which then returns `0`). In a coroutine it means
  `co_return;` (UB if the promise has no `return_void`).
- `return expr;` in a `void` function is allowed only if `expr` has type `void`.

### `goto` out of nested loops

```cpp
bool contains(const int (&grid)[3][3], int value) {
    for (const auto& row : grid)
        for (int x : row)
            if (x == value) goto found;
    return false;
found:
    return true;
}
```

The equivalent without `goto` — usually clearer:

```cpp
#include <algorithm>
#include <ranges>

bool contains2(const int (&grid)[3][3], int value) {
    return std::ranges::contains(grid | std::views::join, value);   // C++23
}
```

### `break` in an expansion statement (C++26)

`break` and `continue` work in a `template for`. Every iteration is still
instantiated at compile time; at run time `break` skips the remaining ones.

```cpp needs=expansion-statements
#include <cstdio>
#include <tuple>
#include <type_traits>

int main() {
    std::tuple<int, char, double> t{1, 'x', 3.5};
    template for (auto e : t) {
        if constexpr (std::is_same_v<decltype(e), double>) break;
        std::printf("%d\n", static_cast<int>(e));   // prints 1, then 120
    }
}
```

GCC 16 compiles and runs this with `-std=c++26`; GCC 15 and Clang 22 don't
implement expansion statements.

### Labels at the end of a block (C++23)

```cpp
#include <cstdio>

void report(int errors) {
    if (errors == 0) goto done;
    std::printf("%d errors\n", errors);
done:          // C++23: a label may end a compound statement (no `;` needed)
}
```

Before C++23 this needed `done: ;`. GCC 15 rejects it with `-std=c++20
-pedantic-errors`.

## Gotchas

- **A jump can't skip an initialization.** Jumping forward past a declaration
  into its scope is ill-formed unless the variable has *vacuous* initialization
  (a scalar or trivial type with no initializer). `int n = 1;` counts as an
  initialization; `int n;` doesn't:

  ```cpp compile-fail="jump|crosses|bypasses"
  #include <string>
  void f(bool b) {
      if (b) goto skip;
      std::string s = "hi";
  skip:
      ;
  }
  ```

  The same rule makes `case 2:` an error after `case 1: int x = 0;` in a
  `switch` — wrap the case body in braces.
- **Jumping backward re-runs the declaration.** `goto` to a label before a
  declaration destroys the variable and constructs it again when control
  reaches it.
- **You can't jump into a `try` block or a handler** (or into an `if constexpr`
  / `if consteval` branch); you can jump out of them.
- **`goto` in a `constexpr` function: allowed to appear, not to run.** Since
  C++23 (P2242) a `constexpr` function may *contain* labels and `goto`, but
  evaluating a `goto` during constant evaluation is still an error:

  ```cpp compile-fail="goto"
  constexpr int count_to(int n) {
      int i = 0;
  again:
      if (i < n) { ++i; goto again; }
      return i;
  }
  static_assert(count_to(3) == 3);   // error: 'goto' is not a constant expression
  ```

- **No labelled `break`.** C2y (the next C) has `break label;`; C++26 doesn't.
- **Don't `return std::move(local);`** — it blocks copy elision. Since C++23
  (P2266) a returned local is treated as an rvalue anyway.
- **`continue` in a `do`-`while`** jumps to the condition, not the top of the
  body — the condition is still evaluated.

## See also

- [break — cppreference](https://en.cppreference.com/w/cpp/language/break)
- [continue — cppreference](https://en.cppreference.com/w/cpp/language/continue)
- [return — cppreference](https://en.cppreference.com/w/cpp/language/return)
- [goto — cppreference](https://en.cppreference.com/w/cpp/language/goto)
- [Labeled statements — cppreference](https://en.cppreference.com/w/cpp/language/statements#Labeled_statements)
- [\[stmt.jump\]](https://eel.is/c++draft/stmt.jump),
  [\[stmt.dcl\]](https://eel.is/c++draft/stmt.dcl) — C++ working draft
- [P2242 — non-literal variables, labels and gotos in constexpr functions](https://wg21.link/p2242)
- [Iteration](iteration.md) · [Selection](selection.md) · [Coroutines](../coroutines/coroutines.md)
