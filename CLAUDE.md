# Working in cpp-docs

This repo is a learning reference for **all of modern C++** — the language and
the standard library — written as example-driven Markdown. There is **no build
system and no compiler step**; the deliverable is documentation, not a program.

We mirror how cppreference organizes the reference and fill it in topic by topic.
**Containers are the current starting point, not the whole scope.** Expect to
grow into strings, algorithms, ranges, utilities, numerics, concurrency, and the
core language over time.

## Source of truth

- The authoritative reference is **cppreference.com**. Follow its facts:
  signatures, complexity, since-version, and section organization.
- **Only cite cppreference.com** in "See also" sections for now. No blogs,
  Stack Overflow, isocpp, etc.
- cppreference blocks automated fetches (HTTP 403 from WebFetch). To verify
  facts, use the `perplexity` MCP tools with
  `search_domain_filter: ["cppreference.com"]`.
- **Exception: `docs/guidelines/`** (opinionated guidance, not reference) may
  cite its own sources — the C++ Core Guidelines, conference talks, style
  guides. Each page's sources are recorded in `docs/guidelines/sources.md`.
  Language/library facts on those pages still follow cppreference.

## Guidelines section

`docs/guidelines/` is about *how to write* C++ well (Core Guidelines digest,
safety, modern idioms, design principles, project conventions). To refresh it
against upstream or import a new reference, use the `refresh-guidelines` skill
(`.claude/skills/refresh-guidelines/`). Keep `docs/guidelines/sources.md`
updated whenever a guidelines page changes.

The full C++ Core Guidelines are a git submodule at `modules/CppCoreGuidelines`
(pinned to the edition the pages track). Run
`git submodule update --init` after cloning. Third-party sources go under
`modules/`; don't edit them.

`modules/perf-report` is the owner's Linux performance-analysis guide
(https://github.com/homer6/perf-report, also perf-report.com: perf, bpftrace,
flame graphs, CPU/memory/disk/network analysis). Link to it for profiling
commands instead of duplicating them.

## Layout

Each cppreference area is a subfolder under `docs/examples/`:

```
docs/examples/
  containers/{sequence,associative,unordered,adaptors,views}/
  strings/  algorithms/  ranges/  utilities/  language/  ...   (added as we go)
  README.md   # master index + progress table
```

Keep `docs/examples/README.md` updated whenever a page is added or finished.

## House style for a page

Keep every page consistent. Skeleton:

```markdown
# std::NAME   (or: Topic name)

> One-sentence description. **Header:** `<header>` · **Since:** C++NN

## Mental model
A sentence or two + an ASCII sketch if it helps.

## Declaration
The signature with its parameters/options explained.

## Complexity        (for containers/algorithms)
A table of the operations that matter, Big-O.

## Iterator & reference invalidation     (for containers)
What invalidates iterators, pointers, references. The #1 footgun.

## When to use it
Fits and anti-fits; how it compares to its siblings.

## Examples
Short, focused snippets. Prefer modern C++ (CTAD, structured bindings, ranges,
`auto`). Note the C++ version when a feature isn't universal.

## Gotchas
The non-obvious traps.

## See also
Links to the relevant cppreference pages (cppreference.com only).
```

## Conventions

- Target the **latest C++ standard**; flag version requirements inline
  (e.g. "`contains()` — since C++20").
- Code blocks use ```cpp.
- Use `#include` + `std::`-qualified names; avoid `using namespace std`.
- One concept per snippet; keep examples minimal and illustrative.

## Compile-checked snippets

CI compiles every ```cpp block in `docs/guidelines/` (blocking) and
`docs/examples/` (report only) with `tools/check_snippets.py`, using
`-std=c++26 -fsyntax-only -pedantic-errors` on GCC and Clang. Each block is
compiled after `tools/snippet_prelude.hpp` (common headers and placeholders
like `Widget`, `use()`, `process()`). Tag the fence's info string:

| Fence | Meaning |
|---|---|
| ` ```cpp ` | Must compile at namespace scope |
| ` ```cpp body ` | Statements; checked inside a function body |
| ` ```cpp nocompile ` | Deliberately wrong (BAD examples) or depends on undeclared context |
| ` ```cpp needs=reflection ` | Needs a feature not every CI compiler has (`contracts`, `reflection`, `execution`, …) |

`needs=` blocks are checked (blocking) in the GCC job with the features GCC 16
has enabled (`contracts`, `reflection`); the Clang job skips them.

Prefer making a snippet self-contained over tagging it `nocompile`. Run locally:
`python3 tools/check_snippets.py docs/guidelines --cxx g++-15`.

`-fsyntax-only` is more than a parse: on GCC and Clang it catches type errors,
errors in templates that are instantiated, and failed constant evaluation —
the same as `-c`. It misses templates that are never instantiated, link errors
(a declared but undefined function), and anything at run time. The summary
counts skipped blocks by reason; keep the `nocompile` count falling.

## Git / commits

- **Do not add Claude as a commit author or co-author.** Commit messages must not
  include any `Co-Authored-By: Claude` (or similar) trailer. Commits are authored
  solely by the repository owner.

## Issues and pull requests

- Work each issue on its own feature branch (e.g. `docs/<issue>-<slug>`) and
  open a PR that references the issue.
- **Open PRs ready for review, never as drafts.**
- **Close the issue as soon as its PR is opened.** Don't wait for the merge,
  and close it even if the PR says "Part of #n".
- **The closing comment explains the change**, not just the link:

  ```markdown
  Delivered in <PR URL>.

  **What changed**
  - each file/page and what it now says or does

  **Why**
  - which part of the issue each change addresses, and what was verified
    against which primary source (or tested how)

  **Still open**
  - what waits on the deep-research report or a follow-up
  ```

  Post it with `gh issue close <n> -c "$(cat comment.md)"` (or
  `gh issue comment <n> -F comment.md` then close).
- Guidelines issues get their own deep-research prompt in
  `.claude/skills/refresh-guidelines/prompts/issue-<n>-<slug>.md`. **Hand each
  prompt to the user as soon as it's written** (collect them with
  `.claude/skills/refresh-guidelines/scripts/collect_prompts.py`, which fills
  in the date) — never leave a prompt only in a branch. Importing
  that report (with the `refresh-guidelines` skill) is a follow-up — on the
  same branch if the PR is still open, otherwise a new PR.
- No AI attribution in issues or PR descriptions either.
