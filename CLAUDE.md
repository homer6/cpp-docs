# Working in cpp-docs

This repo is a learning reference for **all of modern C++** — the language and
the standard library — written as example-driven Markdown. There is **no build
system and no compiler step**; the deliverable is documentation, not a program.

We mirror how cppreference organizes the reference and fill it in topic by topic.
**Containers are the current starting point, not the whole scope.** Expect to
grow into strings, algorithms, ranges, utilities, numerics, concurrency, and the
core language over time.

## Source of truth

- **Use any good source** — cppreference.com, the standard draft
  (eel.is/c++draft), WG21 papers, compiler and standard-library docs and
  status pages, books, talks — **as long as the claim is verified locally**:
  the snippet compiles with a local toolchain, and where behaviour matters, it
  runs and does what the page says.
- Local toolchains: `g++-15` (Homebrew GCC 15.2),
  `/opt/homebrew/opt/llvm/bin/clang++` (Homebrew Clang 22, use
  `-stdlib=libc++`), and Docker (`docker run --rm -v "$PWD":/w -w /w gcc:16 …`)
  for GCC 16 features such as contracts and reflection.
- If something can't be verified locally (no local compiler implements it
  yet), say so on the page and tag the snippet `needs=<feature>`.
- **See also** links to what you used: cppreference first, then papers or
  vendor docs where they add something. No unsourced blogs or AI summaries.
- cppreference blocks automated fetches (HTTP 403 from WebFetch); query it with
  the `perplexity` MCP tools (`search_domain_filter: ["cppreference.com"]`) or
  read other sources with Firecrawl.
- `docs/guidelines/` records each page's sources in `docs/guidelines/sources.md`.

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

There is no CI (GitHub Actions is disabled for this repo); run the checks
locally. `tools/check_snippets.py` compiles every ```cpp block with
`-std=c++26 -fsyntax-only -pedantic-errors`. `docs/guidelines/` must pass
cleanly; `docs/examples/` is checked against a list of known failures (see
below). Each block is
compiled after `tools/snippet_prelude.hpp` (common headers and placeholders
like `Widget`, `use()`, `process()`). Tag the fence's info string:

| Fence | Meaning |
|---|---|
| ` ```cpp ` | Must compile at namespace scope |
| ` ```cpp body ` | Statements; checked inside a function body |
| ` ```cpp nocompile ` | Depends on undeclared context, third-party headers, or is wrong but still compiles (UB, dangling) |
| ` ```cpp compile-fail="regex" ` | A BAD example that must **not** compile; the diagnostics must match the regex (plain `compile-fail` accepts any error) |
| ` ```cpp needs=reflection ` | Needs a feature not every compiler has (`contracts`, `reflection`, `execution`, …) |

`needs=` blocks are skipped unless you enable the feature, e.g.
`--enable contracts --enable reflection --flag=-freflection` with GCC 16.

Prefer making a snippet self-contained over tagging it `nocompile`, and
`compile-fail` over `nocompile` for type errors. Run:

```bash
python3 tools/check_snippets.py docs/guidelines --cxx g++-15
python3 tools/check_snippets.py docs/examples --cxx g++-15 -q \
  --baseline tools/snippet_baseline/gcc.txt
```

Each block has a stable ID, `<file>#<heading-slug>/<n>`, shown next to every
failure. `tools/snippet_baseline/gcc.txt` lists the `docs/examples/` blocks
known to fail on GCC; the run fails on a **new** failure, and on a listed
block that now compiles or no longer exists — so when you fix a snippet,
delete its line (or rerun with `--update-baseline`). The list should only
shrink (#20 drives it to empty).

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
- **Close the issue as soon as its PR is opened — automatically, as part of
  shipping the PR.** Opening the PR *is* the signal that the feature is done;
  don't wait for the merge, close it even if the PR says "Part of #n", and
  **never ask the owner whether to close it or leave it for them to close.**
  An epic/tracker issue stays open until all its sub-issues are closed.
- **Subagents delivering an issue:** if the agent couldn't close the issue
  (e.g. a permission check blocked `gh issue close`), the orchestrating
  session closes it with the comment straight away when the agent reports
  back — the owner has authorized closing issues on PR-open for this repo.
  Agents write the closing comment to a file and report its path, so it
  isn't lost.
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

## Syncing and merging

- **Sync a feature branch with `main` by rebasing**, never by merging `main`
  into it: `git fetch && git rebase origin/main`, resolve, then
  `git push --force-with-lease`. Shared index files (`docs/guidelines/README.md`,
  `sources.md`): keep every row from both sides; if two PRs add the same source
  ID, combine them into one row.
- **If you rebased before merging, comment on the PR** saying what conflicted
  and how it was resolved. No comment is needed for a clean merge.
- **Merge PRs with a merge commit only** — never squash or rebase-merge — and
  **delete the feature branch** when merging:
  `gh pr merge <n> --merge --delete-branch`. Then remove its worktree
  (`git worktree remove <path>`) and the local branch.
- Merge several PRs one at a time: merge one, rebase the next onto the new
  `main`, check, merge, repeat.
- **Comment on the issue every time a PR for it ships** — when it's opened
  (the closing comment above) and again when it's merged, plus after any
  follow-up push such as a research import. Say what changed since the last
  comment and why:

  ```markdown
  Merged in <PR URL> (<merge commit SHA>).

  **What changed**
  - what landed since the PR was opened (follow-up commits, research
    import, rebase conflicts and how they were resolved)

  **Why**
  - the reason for each change and what it was verified against

  **Still open**
  - follow-ups, with issue links
  ```
