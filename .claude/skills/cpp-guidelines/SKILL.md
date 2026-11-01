---
name: cpp-guidelines
description: Review or audit C++ against this repo's guidelines (C++ Core Guidelines digest, safety, modern idioms, design principles, industry practice). It inspects the project's constraints, loads only the relevant guideline pages, cites rule IDs and validates fixes. For writing or modernizing C++, prefer the modern-cpp skill, which calls this one itself. Use this skill directly to review or audit existing code against the Core Guidelines with rule IDs, to answer a question about a specific rule ("what does R.11 say?"), to make a focused ownership, lifetime or memory-safety fix, or to run the guideline evals.
---

# Apply the C++ guidelines

**Prefer `/modern-cpp` for writing or modernizing C++.** It also checks the
reference pages and toolchain support, and it calls this skill when the
guidelines matter. Use this skill directly for:

- **Reviewing or auditing existing code against the Core Guidelines**, where
  you want findings with rule IDs (**R.11**, **C.13**) and no new code.
- **A question about a specific rule** ("what does F.16 say, and does this
  code follow it?") or about one guidelines page.
- **A focused ownership, lifetime or memory-safety fix** in code that already
  uses the right facilities.
- **Measuring the guidelines skill** with the case suite in `evals/`.

The guidance lives in `docs/guidelines/` of the cpp-docs repo (this skill's
repo). The full C++ Core Guidelines are in `modules/CppCoreGuidelines/`. Treat
them as the source; this file only says how to use them.

**Finding the repo.** The paths below are relative to the cpp-docs repo root,
not to this skill's folder. Inside a cpp-docs checkout, use them as they are.
From another project, find the checkout from the skill's real path, because
`make install` links the skill back into it (`<repo>/.claude/skills/cpp-guidelines`):
`realpath ~/.claude/skills/cpp-guidelines/SKILL.md | xargs dirname | xargs dirname | xargs dirname | xargs dirname`.
`$CPP_DOCS` overrides that if it's set. Read every page from there. The
full Core Guidelines need `git submodule update --init` in the checkout. There
is one copy of the guidance, and `git pull` keeps it current.

## 1. Learn the project's constraints first

Before recommending anything, find out (from build files, `CLAUDE.md`, a
feature matrix, or by asking):

- **Language standard and toolchain** — `-std=`, `CMAKE_CXX_STANDARD`, compiler
  and standard-library versions. Don't suggest a C++23/26 feature the project
  can't use; check its feature-test macro if unsure.
- **Policies** — exceptions on or off (`-fno-exceptions`), RTTI, banned
  features (e.g. Chromium-style bans on `std::function` or range pipelines),
  naming conventions.
- **Existing tools** — clang-tidy config, sanitizer builds, test command.

If a constraint matters to the answer and you can't find it, ask — or state
the assumption you're making.

## 2. Load only what's relevant

| Topic in the code | Read |
|---|---|
| Ownership, `new`/`delete`, smart pointers, RAII, parameters | `docs/guidelines/core-guidelines.md` (R, F, C sections) |
| Dangling, invalidation, bounds, uninitialized data, hardening | `docs/guidelines/safety.md` |
| Which modern feature to use (C++17–26) | `docs/guidelines/modern-idioms.md` |
| API shape, variants, strong types, error handling, loops | `docs/guidelines/design-principles.md` |
| Naming, headers, tests, feature matrix | `docs/guidelines/project-conventions.md` |
| A policy question (exceptions, `auto`, ranges) or an Abseil tip | `docs/guidelines/industry-practice.md` |
| The exact text, exceptions, or enforcement notes of a rule | search `modules/CppCoreGuidelines/CppCoreGuidelines.md` for the rule ID |

Read a rule's **Exception** and **Note** paragraphs before calling something a
violation.

## 3. Report findings with rule IDs

For each finding: the location, the rule ID (e.g. **R.11**, **C.13**, Abseil
**#177**), one sentence on the concrete risk, and a minimal fix. Group by
severity: bugs (leaks, dangling, data races, UB) before style.

Don't over-apply rules:

- **A raw pointer isn't automatically wrong.** `T*` and `T&` are correct for
  non-owning, observing access (R.3, F.7). Only flag a raw pointer that *owns*
  (it's `delete`d, or ownership crosses an interface — I.11).
- **Don't swap correct conventions for other correct ones** — e.g. a by-value
  sink parameter plus `std::move`, or small types passed by value (F.16).
- **Respect documented exceptions** — e.g. ES.20 allows a buffer that's filled
  from input immediately.
- **Respect project constraints** — with exceptions off, use E.25 patterns
  (factories returning `std::optional`/`std::expected`), never `throw`.
- **When ownership or lifetime can't be determined from the code, say so** and
  ask, rather than rewriting the interface.

## 4. Validate every change

- Compile with the project's flags; warnings stay at zero.
- Run the tests that cover the code.
- Run clang-tidy if the project has a config. A check that names a rule
  doesn't enforce all of it. For example, `cppcoreguidelines-owning-memory`
  (I.11, R.3, C.33) is type-based and only sees pointers marked
  `gsl::owner<T*>`. A clean run doesn't mean the code complies; a finding
  still needs the rule's exceptions checked.
- For memory or lifetime fixes, run the ASan/UBSan build if one exists.

Report anything you couldn't validate.

## Measuring this skill

`evals/` has a vendor-neutral case suite (ownership and lifetime pilot) for
comparing an agent with and without this skill. See `evals/README.md`.
