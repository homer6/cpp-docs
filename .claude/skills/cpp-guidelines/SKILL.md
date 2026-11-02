---
name: cpp-guidelines
description: Review or audit C++ against this repo's guidelines (every C++ Core Guidelines rule, safety, modern idioms, design principles, industry practice). It inspects the project's constraints, loads only the relevant guideline pages, cites rule IDs and validates fixes. For writing or modernizing C++, prefer the modern-cpp skill, which calls this one itself. Use this skill directly to review or audit existing code against the Core Guidelines with rule IDs, to answer a question about a specific rule ("what does R.11 say?"), to make a focused ownership, lifetime or memory-safety fix, or to run the guideline evals.
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
repo):

- **Every rule of the C++ Core Guidelines**, section by section, in
  `docs/guidelines/core-guidelines/`. `README.md` lists the sections; each
  section page (`class.md` for C, `expr.md` for ES, `resource.md` for R,
  `profile.md` for Type/Bounds/Lifetime, …) lists every rule with its official
  ID, title and link, the pages of ours that cite it, and links to its
  example and test.
- **Our BAD/GOOD example for every rule** (or the reason there's none) in
  `docs/guidelines/core-guidelines/examples/<section page>.md`, compiled by the
  snippet checker; the unit tests that check them are in `tests/guidelines/`.
- **The official text** in `modules/CppCoreGuidelines/CppCoreGuidelines.md`.
  It's the source; our pages explain and illustrate it.

No rule is out of scope because it isn't in one of the topic pages.

**Finding the repo.** The paths below are relative to the cpp-docs repo root,
not to this skill's folder. Inside a cpp-docs checkout, use them as they are.
From another project, use `$CPP_DOCS` if it's set; otherwise follow the
skill's link back to its checkout (`make install` links it there):

```bash
CPP_DOCS="${CPP_DOCS:-$(realpath ~/.claude/skills/cpp-guidelines/SKILL.md | xargs dirname | xargs dirname | xargs dirname | xargs dirname)}"
test -f "$CPP_DOCS/docs/guidelines/core-guidelines/README.md" || echo "not a cpp-docs checkout: set CPP_DOCS"
```

Read every page from there. The
full Core Guidelines need `git submodule update --init` in the checkout; if
the submodule is empty and you can't initialize it, read the rule at the
official link its index entry gives. There
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
| Any Core Guidelines rule or section: find it by ID or topic | `docs/guidelines/core-guidelines/README.md`, then the section page (e.g. `resource.md` for R, `class.md` for C, `expr.md` for ES, `profile.md` for Type/Bounds/Lifetime) |
| Worked examples for ownership, RAII, parameters, classes and more | `docs/guidelines/core-guidelines.md` |
| Our BAD/GOOD example for a specific rule | `docs/guidelines/core-guidelines/examples/<section page>.md` (the rule's index entry links it when there is one) |
| Dangling, invalidation, bounds, uninitialized data, hardening | `docs/guidelines/safety.md` |
| Which modern feature to use (C++17–26) | `docs/guidelines/modern-idioms.md` |
| API shape, variants, strong types, error handling, loops | `docs/guidelines/design-principles.md` |
| Naming, headers, tests, feature matrix | `docs/guidelines/project-conventions.md` |
| A policy question (exceptions, `auto`, ranges) or an Abseil tip | `docs/guidelines/industry-practice.md` |
| The exact text, exceptions, or enforcement notes of a rule | search `modules/CppCoreGuidelines/CppCoreGuidelines.md` for the rule ID (`grep -n '^### .*R\.11:'`; profile rules are bullets: `grep -n 'Type\.1:'`), or open its official link |

Read a rule's **Exception** and **Note** paragraphs before calling something a
violation.

### Finding the right rule

- **From an ID** (`C.21`, `ES.46`, `Type.1`, `GSL.ptr`): the section page in
  `docs/guidelines/core-guidelines/` has its exact title, official link and
  our example. For the full text, `grep -n 'C\.21:' modules/CppCoreGuidelines/CppCoreGuidelines.md`
  (profile rules are bullets, not headings).
- **From what the code does** (a smell, not an ID): search the rule titles,
  then read the candidates' official text and pick the rule whose **Reason**
  matches the risk, not just the words:
  `grep -i -h '<keyword>' docs/guidelines/core-guidelines/*.md | grep '^- \['`
  (e.g. `lock`, `narrow`, `destructor`, `catch`, `out` parameter).
- **Several rules fit:** cite the most specific one (C.35 for a base-class
  destructor, not C.20), and mention a broader one only if it adds something.
- **A rule with no text yet** (the T section lists T.30 and T.31 with titles
  only): say so; cite it by its title, not as settled guidance.

## 3. Report findings with rule IDs

Write each finding in this shape, bugs (leaks, dangling, data races, UB)
before style:

```markdown
- **<file>:<line> · <rule ID>: <official title>.** <the concrete risk, one sentence>.
  Fix: <minimal change>. Example: docs/guidelines/core-guidelines/examples/<page>.md#<heading-slug>
```

The rule IDs are Core Guidelines IDs (**R.11**, **C.13**) or another source's
(Abseil **#177**). The example link shows the reader the BAD and GOOD forms side by
side; find it without reading the whole page:
`grep -n '^## C\.21:' docs/guidelines/core-guidelines/examples/*.md` (the slug
is the heading, lower-cased, punctuation dropped, spaces as hyphens). A
compliant result is one line: what the code does right, with the rule IDs.

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
- **Don't add `const` (or anything else) where the code relies on change**:
  Con.4 is for values that don't change after construction.
- **Code that already follows a rule is a result too**: say it's compliant
  rather than inventing a finding.

## 4. Validate every change

- Compare the fix with the rule's GOOD example in
  `docs/guidelines/core-guidelines/examples/`; if they differ, know why.
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

`evals/cases/` has a vendor-neutral case suite for comparing an agent with and
without this skill: ownership and lifetime (21 cases) plus classes,
expressions, concurrency, errors, constants and functions (10 cases), with
compliant traps as well as violations. `python3 evals/check.py` validates
them (it picks a working compiler unless `CXX` is set). See
`evals/README.md` for how to run and score them.
