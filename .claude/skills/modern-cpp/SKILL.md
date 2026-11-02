---
name: modern-cpp
description: The preferred entry point for C++ work with the cpp-docs repo. It writes, modernizes or reviews C++ using the repo's verified reference (docs/examples/, C++11 to C++26) and calls the cpp-guidelines skill for the guidelines. Before writing, it learns the project's standard and toolchains, looks up the facility's page for usage, gotchas and version notes, checks that the feature is supported where the code must build, and then applies the guidelines and validates the result. Use for "/modern-cpp", "write this in modern C++", "what's the modern way to …", "should I use X or Y", "is <feature> supported on GCC/Clang/MSVC", "modernize this", or any C++ task where getting the standard library and language right matters.
---

# Write modern C++ with the cpp-docs reference

**This is the entry point for C++ work.** It covers the reference, toolchain
support and validation, and it calls `cpp-guidelines` (step 5) when the
guidelines matter. Send the user to `/cpp-guidelines` directly only for a
review or audit against the Core Guidelines with rule IDs, a question about a
specific rule, a focused ownership or lifetime fix, or the evals.

This repo is a verified reference. Every snippet compiles, and the tests in
`tests/` check what each page claims on GCC 15, GCC 16 and Clang 22 (libc++).
Use it as the first source when writing C++, not memory. A page also says
when *not* to use a feature, and which compilers have it.

**Finding the repo.** Inside a cpp-docs checkout, paths are relative to its
root. From another project, find the checkout from the skill's real path,
because `make install` links the skill back into it (`<repo>/.claude/skills/modern-cpp`):
`realpath ~/.claude/skills/modern-cpp/SKILL.md | xargs dirname | xargs dirname | xargs dirname | xargs dirname`.
`$CPP_DOCS` overrides that if it's set. Read the pages from there. To install
the skill for every project, run `make install` in the checkout.

## 1. Learn the project's constraints

Before choosing a feature, find out:
- **standard:** `-std=`, `CMAKE_CXX_STANDARD`;
- **compilers and standard libraries:** GCC/libstdc++, Clang/libc++,
  MSVC/MS STL, with versions. A feature in the standard isn't usable until
  every compiler the code must build with ships it;
- **policies:** exceptions or RTTI off, banned features, naming, an existing
  clang-tidy or sanitizer setup.

`cpp-guidelines` step 1 has the checklist. If a constraint decides the answer
and you can't find it, ask, or state your assumption.

## 2. Find the pages

| You need | Look in |
|---|---|
| Every page, by area | the index in `README.md` (generated from the pages) |
| A facility by name | `grep -rl "<name>" docs/examples` with the bare name (`fold_left`, not `std::ranges::fold_left`), or `python3 .claude/skills/add-to-topic/scripts/find_gaps.py <name>` |
| One area's pages | `docs/examples/<area>/README.md` |
| How to write it well, not what it is | `docs/guidelines/README.md`: modern idioms, safety, design, tooling, testing, build |
| Exactly how it behaves | `tests/<same path as the page>_test.cpp`: the assertions are the checked claims |

These are the common starting points:

| Task | Pages |
|---|---|
| Choosing a container | `containers/README.md`, then the container's page (complexity and invalidation tables) |
| Text, formatting, parsing | `text-processing/formatting.md`, `charconv.md`, `strings/` |
| Errors | `utilities/expected.md`, `diagnostics/`, `language/exceptions/` |
| Ownership and memory | `memory/` (`unique_ptr`, `indirect-polymorphic`, `raw-memory`) and `guidelines/safety.md` |
| Callbacks and type erasure | `utilities/callable-wrappers.md`, `utilities/functional.md` |
| Loops and pipelines | `ranges/views.md`, `algorithms/` |
| Compile-time code | `language/expressions/constant-expressions.md`, `constant-evaluation-rules.md`, `metaprogramming/` |
| Reflection, contracts (C++26) | `language/expressions/reflection.md`, `metaprogramming/reflection*.md`, `language/basic-concepts/contracts.md` |
| Threads and atomics | `concurrency/` and `execution/` |
| Building, modules, packaging | `language/modules/`, `guidelines/build-and-packaging.md` |

## 3. Read the page before writing

Every page has the same skeleton. Read these parts:
- **When to use it:** fits, anti-fits, and how it compares with its siblings.
- **Gotchas:** UB, invalidation, surprising defaults.
- **Version notes:** a feature marked "C++23" or "C++26" may be newer than the
  project's standard.
- **`needs=<feature>` snippets:** some local compiler lacks the feature, or no
  local compiler implements it yet. The page says which.

## 4. Check support where the code must build

1. **The page's own notes** say which of GCC, Clang and MSVC have it, often
   with versions.
2. **Feature-test macros:** `#include <version>` and test `__cpp_lib_<x>`
   (library) or `__cpp_<x>` (language). The value matters, because a bumped
   value can mean a renamed or extended API. The page and `[version.syn]` give it.
3. **The local compilers:**
   `python3 .claude/skills/add-to-topic/scripts/probe.py macros __cpp_lib_<x>`
   or `probe.py snippet file.cpp --run`.
4. **A consented CI run**, for MSVC, FreeBSD and other platforms you can't
   run locally: `docs/platform-support.md` in the checkout lists the last
   GitHub Actions run per platform and compiler, with its link. Cite that
   over a status page alone.
5. **The vendors' published status pages**, where no run covers it (older
   releases, or before the first run). The `add-to-topic` skill's step 2
   lists them and explains how to read them.

If the project can't use the feature, write the fallback the page suggests,
or guard it:
`#if __cpp_lib_<x> >= <value>` … `#else` … `#endif`.

## 5. Apply the guidelines

Use the `cpp-guidelines` skill for ownership, lifetime, safety, idioms and
design. `make install` installs it too. Without it, read
`<repo>/.claude/skills/cpp-guidelines/SKILL.md` and follow it. It loads only the guideline pages that matter, and its findings cite
Core Guidelines rule IDs. Prefer what the reference pages recommend
over habits from older standards, unless the project's constraints say
otherwise.

## 6. Write and validate

- Compile with the project's flags and keep warnings at zero.
- Run the tests that cover the change.
- For memory, lifetime or threading changes, run sanitizers
  (`guidelines/tooling.md`), plus clang-tidy if the project has a config.
- Say what you couldn't validate.

## 7. Cite, and feed gaps back

Name the pages you relied on (`docs/examples/ranges/views.md`), so the
reader can check them. If the repo has no page for what you needed, or a page
turned out to be wrong, say so and offer to fix it: an issue in
homer6/cpp-docs, then the `add-to-topic` skill to add or correct the page.
