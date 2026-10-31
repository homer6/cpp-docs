# Project conventions — naming, layout, API shape, tests

> A house style for C++ libraries: how to name things, lay out headers, shape
> parameters, and write tests. **Source:** the automerge-cpp style guide · pairs
> with [design principles](design-principles.md)

These are conventions, not laws — pick a set and apply it consistently. This is
the set we use.

## Naming

| Entity | Style | Examples |
|---|---|---|
| Namespace | `snake_case` | `automerge_cpp`, `automerge_cpp::detail` |
| Type / class / struct | `PascalCase` | `Document`, `ActorId` |
| Function / method | `snake_case` | `put_object`, `get_changes` |
| Variable | `snake_case` | `actor_id`, `op_count` |
| Constant / `constexpr` | `snake_case` | `root`, `max_actor_bytes` |
| Enumerator | `snake_case` | `ObjType::map` |
| Template parameter | `PascalCase` | `template <typename Fn>` |
| Macro (avoid) | `SCREAMING_SNAKE_CASE` | `PROJECT_ASSERT` |
| File | `snake_case` | `document.hpp`, `op_set.cpp` |

Principles:

- **Names say what, not how:** `active_users`, not `filtered_list`.
- **Predicates read as questions:** `is_empty()`, `has_key()`, `contains()`.
- **Mutators are verbs:** `put()`, `insert()`, `merge()`.
- **Accessors are nouns:** `length()`, `keys()`, `actor_id()`.
- **No abbreviations** unless universal (`doc` yes, `chg` no).
- **No Hungarian prefixes** (`m_`, `p_`, `s_`).

Public symbols live in one project namespace; implementation details in
`project::detail`. Users alias the namespace rather than `using` it:

```cpp
namespace am = automerge_cpp;
auto doc = am::Document{};
```

## Files and headers

### Layout

| Location | Contents |
|---|---|
| `include/<project>/*.hpp` | Public headers (installed) |
| `src/*.cpp` | Implementation |
| `src/*.hpp` | Internal headers (not installed) |
| `tests/`, `examples/`, `benchmarks/` | One target each |

- **Public headers declare;** only `constexpr`, `inline`, and template
  definitions belong there.
- **One concept per header** — one primary type, or a small group of types that
  depend on each other.

### Header shape and include order

```cpp
#pragma once

#include <project/error.hpp>     // 1. project headers (alphabetical)
#include <project/types.hpp>

#include <nlohmann/json.hpp>     // 2. third-party headers

#include <cstdint>               // 3. standard library headers
#include <expected>
#include <string>

namespace project {

// declarations

}  // namespace project
```

In a `.cpp` file, its own header comes first, so a header that's missing an
include fails to compile right away.

## Parameter passing

| Parameter | Pass as |
|---|---|
| Read-only, cheap to copy (≤ 4 words, per F.16) | by value |
| Read-only, expensive to copy | `const T&` |
| Read-only contiguous elements | `std::span<const T>` |
| Read-only string | `std::string_view` |
| Sink (will be stored/moved) | by value; caller `std::move`s in |
| In-out | `T&` |
| Callable, not stored | template parameter or `std::function_ref` (C++26) |

## Thread-safety contract

State it for every public type. A common default, matching the standard
library:

- `const` member functions are safe to call concurrently.
- Non-`const` member functions need external synchronization.

If a type does its own locking internally, say so and say what it costs.

## Tests

- **Name tests after the behaviour:**
  `TEST(ActorId, default_constructed_is_all_zeros)`.
- **Arrange / act / assert**, one logical property per test (several
  `EXPECT`s are fine if they check the same property).
- **No helpers that hide the logic.** Tests are documentation; clarity beats
  DRY.
- **Property-based tests for algebraic laws** — see
  [design principles](design-principles.md#7-test-the-laws).
- **Build warning-clean** at `-Wall -Wextra` (and `-Werror` in CI).

## Performance

- **Measure before optimizing;** keep a benchmark suite for exactly this.
- **Prefer standard algorithms** to hand-rolled clever code.
- **Keep small, frequent types on the stack** (a 16-byte ID shouldn't
  allocate); let `std::string`'s small-string optimization do its job.
- **Give heap-owning types `noexcept` moves** so containers move rather than
  copy them.

## Pre-merge checklist

- [ ] Follows [design principles](design-principles.md) checklist
- [ ] Names follow the table above
- [ ] Public headers declare only; internal headers aren't installed
- [ ] Thread-safety contract documented for new public types
- [ ] Tests added, named after behaviour, passing
- [ ] No new warnings

## See also

- [Core Guidelines — Naming and layout (NL)](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-naming)
- [Core Guidelines digest](core-guidelines.md)
