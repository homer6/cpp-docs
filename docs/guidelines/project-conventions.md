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

## Feature matrix

"Requires C++23" isn't enough: compilers and standard libraries implement
features at different times, and projects restrict features on purpose (see
[industry practice](industry-practice.md)). Keep a short table in the repo:

| Item | Example (illustrative) |
|---|---|
| Compilers and exact versions | GCC 16.2, Clang 22, MSVC 19.50 |
| Standard libraries and versions | libstdc++ 16, libc++ 22, MSVC STL |
| Language mode | `-std=c++23` (`c++26` for opted-in targets) |
| Beyond-baseline features in use | contracts, `std::optional<T&>` — each with its feature-test macro |
| Banned or restricted features, with the reason | modules: build system support not ready |
| Exceptions / RTTI policy | no exceptions in library code; RTTI on |
| Hardening and sanitizer builds | see [safety](safety.md#build-configurations) |

Gate optional features with their feature-test macros, and keep them out of
public headers unless every supported consumer toolchain has them.

## Warnings and static analysis

Start from a reviewed baseline and turn on more as the code gets clean — not
every check at once.

```text
Always:     -Wall -Wextra -Wpedantic        (-Werror in CI)
Migrate to: -Wconversion -Wsign-conversion -Wshadow
```

A clang-tidy starting point focused on real bugs:

```yaml
Checks: >
  -*,
  bugprone-*,
  clang-analyzer-*,
  cppcoreguidelines-*,
  modernize-use-nullptr,
  modernize-use-override,
  performance-*
```

Pin the clang-tidy version with the compiler — check names and behaviour
change between releases. Disable individual checks that don't fit, with a
comment saying why.

## Modules

Pilot modules on one target end to end before adopting them: dependency
scanning, clean and incremental builds, caching, debugging, IDE indexing,
static analysis, packaging, and downstream consumers. Google and Chromium both
still ban modules.

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

For property-based tests, fuzzing, testing contracts, coverage, and
compile-time tests, see [testing](testing.md).

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
