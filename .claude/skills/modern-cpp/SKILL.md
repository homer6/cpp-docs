---
name: modern-cpp
description: The preferred entry point for C++ work with the cpp-docs repo. It writes, modernizes or reviews C++ using the repo's verified reference (docs/examples/, C++11 to C++26) and its measured platform results (which compiler, standard library and platform supports each feature), and calls the cpp-guidelines skill for the guidelines. Before writing, it learns the project's standard and toolchains, reads the facility's page for usage, gotchas and version notes, checks support on every compiler the code must build with, then applies the guidelines and validates the result. Use for "/modern-cpp", "write this in modern C++", "what's the modern way to …", "should I use X or Y", "is <feature> supported on GCC/Clang/MSVC/Apple Clang", "can I use <feature> with <compiler>", "modernize this", or any C++ task where getting the language and standard library right matters.
---

# Write modern C++ with the cpp-docs reference

**This is the entry point for C++ work.** It covers the reference, toolchain
support and validation, and calls `cpp-guidelines` (step 5) for the
guidelines.

| The task | Use |
|---|---|
| Write, modernize or review C++; choose a facility; "is X supported on Y?" | this skill |
| Audit code against the Core Guidelines with rule IDs; "what does R.11 say?"; a focused ownership or lifetime fix; the evals | `cpp-guidelines` directly |
| An attacker controls part of the input; a security review; hardening a build; security tests | `cpp-security` (it calls this skill for the writing) |

**Why trust the repo over memory.** Every snippet in `docs/` is compiled by
`tools/check_snippets.py`, and a GoogleTest suite in `tests/` checks what each
page claims. Both run locally (GCC 15, GCC 16, Clang 22) and in consented
GitHub Actions runs on about 50 toolchains: GCC 13–16, Clang 18–23 with
libstdc++ and libc++, Apple Clang, MSVC, clang-cl and MinGW, on Linux, macOS,
Windows and FreeBSD, x86_64 and arm64. A feature a toolchain lacks shows as a
skip with its reason, so the results say **what each compiler supports, as
measured**, not as advertised. Memory is a fallback, and it's often a standard
or two out of date.

**Finding the repo.** Inside a cpp-docs checkout, paths are relative to its
root. From another project, use `$CPP_DOCS` if it's set; otherwise follow the
skill's link back to its checkout (`make install` links it there):

```bash
CPP_DOCS="${CPP_DOCS:-$(realpath ~/.claude/skills/modern-cpp/SKILL.md | xargs dirname | xargs dirname | xargs dirname | xargs dirname)}"
test -f "$CPP_DOCS/docs/examples/README.md" || echo "not a cpp-docs checkout: set CPP_DOCS"
```

Read every page from there. `git -C "$CPP_DOCS" pull` keeps it current.

## 1. Learn the project's constraints

Before choosing a feature, find out (build files, `CLAUDE.md`, CI config):
- **standard:** `-std=`, `CMAKE_CXX_STANDARD`, `/std:`;
- **every compiler and standard library the code must build with**, with
  versions: GCC/libstdc++, Clang/libc++ or libstdc++, Apple Clang (Xcode),
  MSVC/MS STL. A feature in the standard isn't usable until **all** of them
  ship it;
- **policies:** exceptions or RTTI off, banned features, naming, an existing
  clang-tidy or sanitizer setup.

`cpp-guidelines` step 1 has the full checklist. If a constraint decides the
answer and you can't find it, ask, or state the assumption you're making.

## 2. Find the pages

| You need | Look in |
|---|---|
| Every page, by area | the index in `README.md` (generated from the pages) |
| A facility by name | `grep -rl "<name>" docs/examples` with the bare name (`fold_left`, not `std::ranges::fold_left`), or `python3 .claude/skills/add-to-topic/scripts/find_gaps.py <name>` |
| One area's pages | `docs/examples/<area>/README.md` |
| How to write it well, not what it is | `docs/guidelines/README.md`: modern idioms, safety, design, tooling, testing, build |
| A Core Guidelines rule, or our BAD/GOOD example for it | `docs/guidelines/core-guidelines/README.md` (every rule, by section; each rule links its example and test) |
| Exactly how it behaves | `tests/<same path as the page>_test.cpp`: the assertions are the checked claims |

Common starting points (under `docs/examples/`):

| Task | Pages |
|---|---|
| Choosing a container | `containers/README.md`, then the container's page (complexity and invalidation tables) |
| Text, formatting, parsing | `text-processing/formatting.md`, `text-processing/charconv.md`, `strings/` |
| Errors | `utilities/expected.md`, `diagnostics/`, `language/exceptions/` |
| Ownership and memory | `memory/` (`unique_ptr.md`, `indirect-polymorphic.md`, `raw-memory.md`) and `docs/guidelines/safety.md` |
| Callbacks and type erasure | `utilities/callable-wrappers.md`, `utilities/functional.md` |
| Loops and pipelines | `ranges/views.md`, `algorithms/` |
| Compile-time code | `language/expressions/constant-expressions.md`, `language/expressions/constant-evaluation-rules.md`, `metaprogramming/` |
| Reflection, contracts (C++26) | `language/expressions/reflection.md`, `metaprogramming/reflection.md`, `language/basic-concepts/contracts.md` |
| Threads and atomics | `concurrency/` and `execution/` |
| Building, modules, packaging | `language/modules/`, `docs/guidelines/build-and-packaging.md` |

## 3. Read the page before writing

Every page has the same skeleton. Read these parts:
- **When to use it:** fits, anti-fits, and how it compares with its siblings.
- **Gotchas:** UB, invalidation, surprising defaults.
- **Version notes:** a feature marked "C++23" or "C++26" may be newer than the
  project's standard.
- **`needs=<feature>` snippets:** not every compiler has the feature; the page
  says which.

## 4. Check support on every compiler the code must build with

Go down this list until it answers the question; cite what answered it.

1. **The platform support notes**, `docs/platform-support/notes.md`: one
   paragraph per compiler family on what it supports and lacks (for example
   "GCC 14: no `flat_map`/`flat_set`, `#embed`…", "Apple Clang: no named
   modules", "MSVC: no `#embed`, no C++26 mode"). Start here for any
   compiler question.
2. **The measured result for the exact feature.** Each toolchain page in
   `docs/platform-support/` (one per platform × compiler × architecture, e.g.
   `macos-26-arm64-apple-clang-21.md`, `windows-server-2025-x86_64-msvc-19.51.md`)
   lists every test and snippet with ✅, or ⏭ and the skip reason. Find the
   page's tests (`tests/<page path>_test.cpp`), then:
   `grep -n "<TestName or snippet ID>" docs/platform-support/*.md`.
   `docs/platform-support.md` has the summary matrix (areas × compilers) and
   the known gaps per toolchain.
3. **Compiler bugs** that make a feature unusable even where it's
   "supported": `docs/platform-support/known-bugs.json` (toolchain, summary,
   upstream bug, our issue). Example: libc++ 19/20 `ranges::contains` over
   `views::join` returns the wrong answer.
4. **The page's own notes** on GCC, Clang and MSVC versions, and its
   `needs=` tags.
5. **Feature-test macros:** `#include <version>` and test `__cpp_lib_<x>`
   (library) or `__cpp_<x>` (language). The value matters: a bumped value can
   mean a renamed or extended API. The page and `[version.syn]` give it.
6. **The local compilers:**
   `python3 .claude/skills/add-to-topic/scripts/probe.py macros __cpp_lib_<x>`
   or `probe.py snippet file.cpp --run`.
7. **Vendor status pages**, only for toolchains no run covers (older
   releases). The `add-to-topic` skill's step 2 lists them and how to read them.

Say which version and platform the evidence covers: a result for GCC 15 on
Linux says nothing certain about GCC 13, and a libc++ result doesn't cover
libstdc++.

If a compiler the project needs lacks the feature, **write the fallback** the
page suggests (or the closest facility that every target has), or guard it:
`#if __cpp_lib_<x> >= <value>` … `#else` … `#endif`. Never recommend a
feature that one of the project's compilers can't build.

## 5. Apply the guidelines

Use the `cpp-guidelines` skill for ownership, lifetime, safety, idioms and
design (`make install` installs it too; without it, read
`$CPP_DOCS/.claude/skills/cpp-guidelines/SKILL.md` and follow it). Its
findings cite Core Guidelines rule IDs, and every rule has a BAD/GOOD example
in `docs/guidelines/core-guidelines/examples/`. Prefer what the reference
pages recommend over habits from older standards, unless the project's
constraints say otherwise.

### When the code faces untrusted input

If any input crosses a trust boundary (network, files from users, IPC,
plugins, command lines of a privileged program), use the `cpp-security` skill
(`$CPP_DOCS/.claude/skills/cpp-security/SKILL.md`) for the review. It
works from `docs/security/`. Even for a quick change, check these before
writing:

- **Sizes and indexes from input:** check before the arithmetic
  (`offset > size || length > size - offset`, never `offset + length > size`),
  and before the access (`docs/security/integers.md`, `bounds.md`).
- **Parsing:** parse into a constrained type at the boundary, reject what's
  ambiguous, and bound the work (`parsing-and-validation.md`, `resource-limits.md`).
- **Lifetimes across callbacks and coroutines:** nothing queued captures
  `this` or a reference unless something keeps it alive (`concurrency.md`).
- **Failure:** a security decision that can't be made denies (`error-handling.md`).
- **Secrets and randomness:** OS randomness, constant-time comparison, no
  secrets in logs (`cryptography-and-secrets.md`).
- **Processes and files:** an argument vector, never a shell; no check-then-use
  on paths (`filesystem-and-processes.md`).
- **The build:** library hardening and the protections in `hardening.md`,
  for the library actually linked.

Cite the page and anchor (`docs/security/integers.md#check-before-the-arithmetic`),
the CERT ID where the page names one (INT30-C), and Core Guidelines IDs.

## 6. Write and validate

- Compile with the project's flags and keep warnings at zero.
- Run the tests that cover the change.
- For memory, lifetime or threading changes, run the sanitizers
  (`docs/guidelines/tooling.md`), plus clang-tidy if the project has a config.
- Say what you couldn't validate, and on which compilers.

## 7. Cite, and feed gaps back

Name the pages and results you relied on (`docs/examples/ranges/views.md`,
`docs/platform-support/notes.md`), so the reader can check them. If the repo
has no page for what you needed, or a page or result turned out to be wrong,
say so and offer to fix it: an issue in homer6/cpp-docs, then the
`add-to-topic` skill to add or correct the page.
