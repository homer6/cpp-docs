# Working in cpp-docs

This repo is a learning reference for **all of modern C++** — the language and
the standard library — written as example-driven Markdown. The deliverable is
documentation, not a program: every C++ snippet is compile-checked locally
(`tools/check_snippets.py`), and the test suite under `tests/` checks the
pages' claims at run time (see "Running the tests").

We mirror how cppreference organizes the reference. Every library area and
language area on cppreference's main page now has pages (see
`docs/examples/README.md`); new work fills in detail, fixes mistakes, and keeps
up with new standards.
To add a feature, facility or gotcha to a topic, use the `add-to-topic`
skill (`.claude/skills/add-to-topic/`): coverage check, Perplexity and Firecrawl research,
verification on all three toolchains, placement, snippet and test.
To write C++ with this reference (here or in another project), use the
`modern-cpp` skill (`.claude/skills/modern-cpp/`): project constraints, the
right pages, toolchain support, the guidelines, validation.
`make install` links it (and `cpp-guidelines`) into `~/.claude/skills`, upgrading
an older link or copied install; `make update` pulls and re-links; `make
uninstall` removes the links.
Add a skill to the Makefile's `SKILLS` only if it works outside the repo.
Never run `make install` from a worktree: the link would point at a
directory that's deleted after the merge.

## Source of truth

- **Use any good source** — cppreference.com, the standard draft
  (eel.is/c++draft), WG21 papers, compiler and standard-library docs and
  status pages, books, talks — **as long as the claim is verified locally**:
  the snippet compiles with a local toolchain, and where behaviour matters, it
  runs and does what the page says.
- Local toolchains: `g++-15` (Homebrew GCC 15.2),
  `/opt/homebrew/opt/llvm/bin/clang++` (Homebrew Clang 22, use
  `-stdlib=libc++`), and Docker (`docker run --rm --platform linux/arm64 -v "$PWD":/w -w /w gcc:16 …`)
  for GCC 16 features such as contracts and reflection. Always run the image for
  the host's architecture (`linux/arm64` on Apple silicon): under emulation,
  results change (#130). `tools/run_tests.sh` and `probe.py` pass it for you.
- If something can't be verified locally (no local compiler implements it
  yet), say so on the page and tag the snippet `needs=<feature>`.
- **Compiler support beyond the local toolchains** (MSVC, other releases,
  partial support): read the vendors' status pages. The `add-to-topic` skill's
  "Checking compiler support" section lists them (GCC, libstdc++, Clang,
  libc++, MSVC, MS STL, cppreference) and explains how to read a level of
  support. Say where a vendor-only claim comes from.
- **See also** links to what you used: cppreference first, then papers, the
  draft, vendor docs, articles or talks where they add something. Don't cite
  AI summaries; cite the source they came from.
- cppreference blocks WebFetch (HTTP 403). Query it with the `perplexity` MCP
  tools (`search_domain_filter: ["cppreference.com"]`), or read the page itself
  with the Firecrawl CLI (`firecrawl scrape <url> --only-main-content`), which
  also reads papers (`wg21.link`) and finds implementation PRs
  (`firecrawl developer`).
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
  README.md            # master index + progress table
  containers/{sequence,associative,unordered,adaptors,views}/
  strings/  text-processing/  algorithms/  ranges/  iterators/  utilities/
  memory/  metaprogramming/  numerics/  datetime/  io/  concurrency/
  execution/  diagnostics/  language-support/  concepts/
  feature-test-macros/  named-requirements/
  language/{basic-concepts,expressions,statements,declarations-initialization,
            functions,classes,templates,exceptions,coroutines,modules,preprocessor}/
```

Keep `docs/examples/README.md` updated whenever a page is added or finished.
Then regenerate the root README's clickable index — never edit it by hand:
`python3 tools/gen_index.py` (titles come from each page's `# ` heading, area
order and descriptions from `docs/examples/README.md`);
`python3 tools/gen_index.py --check` fails if it's stale. Run the check before
every PR that adds, removes, moves or retitles a page.

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
Links to the relevant cppreference pages first, then the other sources used.
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
`-std=c++26 -fsyntax-only -pedantic-errors`. **Both `docs/guidelines/` and
`docs/examples/` must pass cleanly** — every block compiles, or carries an
honest `nocompile` / `compile-fail` / `needs=` tag. Each block is
compiled after `tools/snippet_prelude.hpp` (common headers and placeholders
like `Widget`, `use()`, `process()`). Tag the fence's info string:

| Fence | Meaning |
|---|---|
| ` ```cpp ` | Must compile at namespace scope |
| ` ```cpp body ` | Statements; checked inside a function body |
| ` ```cpp nocompile ` | Depends on undeclared context, third-party headers, or is wrong but still compiles (UB, dangling) |
| ` ```cpp compile-fail="regex" ` | A BAD example that must **not** compile; the diagnostics must match the regex (plain `compile-fail` accepts any error) |
| ` ```cpp needs=reflection ` | Needs a feature not every compiler has (`contracts`, `reflection`, `execution`, …) |
| ` ```cpp module ` | A C++20 module unit, or a file that imports modules (`import std;`, `import <vector>;`). Compiled **without** the prelude, with `-c` and modules enabled for the compiler |
| ` ```cpp module=geo ` | One unit of a multi-file module example. All blocks in the same file with the same group name compile in document order in one directory, so later blocks import the BMIs of earlier ones: write the interface (and partitions) first, then implementation units and importers |

`needs=` blocks are skipped unless you enable the feature, e.g.
`--enable contracts --enable reflection --flag=-freflection` with GCC 16.

Module blocks combine with `compile-fail` (that block must fail, so make it an
importer) and `needs=` (a gated block skips its group). The checker enables
modules per compiler (GCC `-fmodules`; Clang `.cppm` + `-fmodule-output`),
builds `std`/`std.compat` once for `import std;`, and builds header units for
`import <header>;`. The private module fragment is
`needs=private-module-fragment` (GCC 15/16 lack it); check module pages with
Clang too: `--cxx /opt/homebrew/opt/llvm/bin/clang++ --flag=-stdlib=libc++
--enable private-module-fragment`.

Prefer making a snippet self-contained over tagging it `nocompile`, and
`compile-fail` over `nocompile` for type errors. For an issue, check only the
pages you changed (`python3 tools/check_snippets.py <pages> --cxx …` with
both compilers). The full runs below are for a release (see "Running the
tests"):

```bash
python3 tools/check_snippets.py docs/guidelines --cxx g++-15
python3 tools/check_snippets.py docs/examples --cxx g++-15 -q \
  --baseline tools/snippet_baseline/gcc.txt
python3 tools/check_snippets.py docs --cxx /opt/homebrew/opt/llvm/bin/clang++ \
  --flag=-stdlib=libc++ --flag=-fexperimental-library -q \
  --baseline tools/snippet_baseline/clang.txt
```

The Clang 22 + libc++ run must pass too. `tools/snippet_baseline/clang.txt`
lists only genuine libc++ gaps (a header or feature libc++ doesn't ship yet):
a snippet that fails on Clang because it's wrong or uses a GCC extension gets
fixed so it compiles on both. Delete a line when libc++ catches up.

Each block has a stable ID, `<file>#<heading-slug>/<n>`, shown next to every
failure. `tools/snippet_baseline/gcc.txt` (the list of known failures) is
**empty** and must stay that way: fix a failing block or tag it honestly —
never add it to the baseline. The `--baseline` run still guards against stale
entries if one is ever added.

`-fsyntax-only` is more than a parse: on GCC and Clang it catches type errors,
errors in templates that are instantiated, and failed constant evaluation —
the same as `-c`. It misses templates that are never instantiated, link errors
(a declared but undefined function), and anything at run time. The summary
counts skipped blocks by reason; keep the `nocompile` count falling.

## Running the tests

`tests/` is a GoogleTest suite that turns each page's claims into assertions
(#41). It's the only thing CMake builds. There's no CI; run it locally.

**Run only the tests that apply to an issue.** The full suite runs only
before a release.

| When | What to run |
|---|---|
| Working an issue / opening its PR | The test areas the change touched (build the area target, `ctest -L '^<area>$'`) on `gcc-15` and `clang-22`, plus `gcc-16` when the change needs a GCC 16 feature. The snippet checker on the changed pages only, with both compilers. `check_links.py` on the changed files and `gen_index.py --check`. If you edit a baseline file, run that compiler's `--baseline` run so the file stays consistent. |
| Cutting a release | Everything: `tools/run_tests.sh` on all three presets, the full `--baseline` snippet runs for `docs/examples` and `docs/guidelines` with both compilers, and the link and index checks. The counts go in the release notes. |

One area, without building the rest of the suite:

```bash
cmake --preset gcc-15 && cmake --build --preset gcc-15 --target examples_<area>_tests
ctest --preset gcc-15 -L '^<area>$'
```

The area is the top folder under `tests/`; hyphens become underscores in the
target name. The full-suite commands:

```bash
tools/run_tests.sh              # gcc-15 (default)
tools/run_tests.sh clang-22     # Homebrew Clang 22 + libc++
tools/run_tests.sh gcc-16       # GCC 16 via the gcc:16 Docker image
tools/run_tests.sh gcc-15 -L containers   # one area; args after the preset go to ctest
```

The script runs `cmake --workflow --preset <preset>` (configure, build and
test into `build/<preset>/`) and then prints every skipped test with its
reason. The presets live in `CMakePresets.json`; README.md explains each one
and the Docker recipe (the `gcc:16` image needs `apt-get install cmake
ninja-build`, which the script does for you).

Writing tests:

- **One file per page, mirroring the path:**
  `docs/examples/containers/sequence/vector.md` →
  `tests/containers/sequence/vector_test.cpp`. A folder whose only page is its
  `README.md` uses `readme_test.cpp`. Every `*_test.cpp` under `tests/<area>/`
  is picked up automatically into that area's executable
  (`examples_<area>_tests`, hyphens become underscores); re-run the configure
  step after adding a file. Suite names must be unique within an area.
- Start the file with `// Checks claims made by docs/examples/<path>.md.` and
  put a one-line comment above each `TEST` naming the section it checks.
- **Assert the claim, not just that it runs:** `EXPECT_EQ` on printed output
  (`cppdocs::capture_stdout([]{ ... })` from `support/capture.hpp` captures
  `std::cout`, `printf` and `std::print`), values the page states in comments,
  `static_assert` for type and trait claims, `EXPECT_THROW` for documented
  exceptions.
- **Missing features skip, never vanish:** include `<version>` and
  `support/feature.hpp`, call `CPPDOCS_REQUIRE(__cpp_lib_x)` (or
  `CPPDOCS_REQUIRE_VERSION(__cpp_lib_x, 202306L)`) first, and wrap the code that
  needs the feature in `#ifdef __cpp_lib_x`. Implementation-defined results get
  `GTEST_SKIP() << "reason"` or a per-toolchain expectation.
- **A failing test means the page is wrong:** fix the page in the same PR and
  list it in the PR description.
- **Module tests:** module units live in `tests/language/modules/units/` and
  are listed in `tests/language/modules/modules.cmake` (providers in the
  `FILE_SET CXX_MODULES`, implementation units as ordinary sources); a test
  file that imports them goes in that file's `_mod_tests` list so CMake scans
  it. Guard imports with `#ifdef CPPDOCS_NAMED_MODULES` and start each test
  with `CPPDOCS_REQUIRE_NAMED_MODULES()` (`language/modules/named_modules.hpp`).
- The affected test areas must pass on `gcc-15` and `clang-22` (or skip with a
  reason) before a PR. The PR's Tested section names what was run.

## Git / commits

- **Do not add Claude as a commit author or co-author.** Commit messages must not
  include any `Co-Authored-By: Claude` (or similar) trailer. Commits are authored
  solely by the repository owner.

## Issues and pull requests

- **Create a GitHub issue first, for all work** — before starting, however
  small: pages, fixes, tooling, `CLAUDE.md`/README edits, and changes pushed
  directly to a branch at the owner's request. Nothing lands without an issue
  reference (`Closes #n` in the PR, or `Closes #n` in a direct commit). Create
  issues with `gh issue create -R homer6/cpp-docs --assignee @me`.
- **Branches:** `dev` is the integration branch; `main` holds releases only.
  Work each issue on its own feature branch from `origin/dev` (e.g.
  `docs/<issue>-<slug>`) and open a PR into `dev` that references the issue:
  `gh pr create -R homer6/cpp-docs --base dev ...`. Always write `--base dev`
  out: GitHub's default branch is `main`, so a PR without it targets `main`.
- **The only PR into `main` is the release PR** (`dev` → `main`), opened when
  the owner names a version. Its merge commit is tagged `vX.Y.Z` and gets a
  GitHub release; the notes live in `docs/releases/vX.Y.Z.md`.
- **Open PRs ready for review, never as drafts.**
- **Every PR keeps `README.md` and `CLAUDE.md` current.**
  - `README.md`: regenerate the clickable index (`python3 tools/gen_index.py`)
    whenever a page is added, moved or retitled. Edit the prose when the change
    touches it: a new area, tool, skill, check or workflow.
  - `CLAUDE.md`: update it when the change adds or alters a convention, a fence
    tag, a tool, a skill or a workflow step.

  The PR description says which of the two were updated, or why neither
  needed it. `python3 tools/gen_index.py --check` must pass.
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

- **Sync a feature branch with `dev` by rebasing**, never by merging `dev`
  into it: `git fetch && git rebase origin/dev`, resolve, then
  `git push --force-with-lease`. Shared index files (`docs/guidelines/README.md`,
  `sources.md`): keep every row from both sides; if two PRs add the same source
  ID, combine them into one row.
- **If you rebased before merging, comment on the PR** saying what conflicted
  and how it was resolved. No comment is needed for a clean merge.
- **Never merge a PR without the owner's explicit approval for that PR** — a
  verdict such as "merge #n", or "merge them all" naming that batch. This
  includes `CLAUDE.md`/README PRs opened at the owner's request: being asked
  to make a change means open the PR (and close its issue), not merge it.
  Approval for one batch doesn't carry over to PRs opened later. Review goes
  one PR at a time (`/review-prs`).
- **Merge PRs with a merge commit only** — never squash or rebase-merge — and
  **delete the feature branch** when merging:
  `gh pr merge <n> --merge --delete-branch`. Never `--delete-branch` on the
  release PR: its head is `dev`. Then remove its worktree
  (`git worktree remove <path>`) and the local branch.
- Merge several PRs one at a time: merge one, rebase the next onto the new
  `dev`, check, merge, repeat.
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
