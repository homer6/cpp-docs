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
right pages, toolchain support from the measured platform results (the
support notes, toolchain pages, known bugs), the guidelines, validation. Its
eval cases are in `evals/modern-cpp/` (#251); `evals/check.py` validates them.
It's the preferred entry point and calls `cpp-guidelines` itself. Use
`cpp-guidelines` directly only for a Core Guidelines review or audit with rule
IDs, a question about one rule, a focused ownership or lifetime fix, or the
evals (`evals/cases/`, 31 cases; `python3 evals/check.py` validates them and
picks a working compiler unless `CXX` is set, #252).
For security (a review where an attacker controls input, hardening a build,
planning security tests), use the `cpp-security` skill
(`.claude/skills/cpp-security/`), built on `docs/security/` (#312); it calls
`modern-cpp` and `cpp-guidelines` for the writing and rule detail. Its eval cases are in `evals/cpp-security/` (#309), validated by
`evals/check.py` (cited page anchors, CERT and Core Guidelines IDs). When a
security page is verified or changes what it recommends, update the skill's
page table and checklist in the same PR.
`make install` links it (and `cpp-guidelines` and `cpp-security`) into `~/.claude/skills`, upgrading
an older link or copied install; `make update` pulls and re-links; `make
uninstall` removes the links.
To run, read and report the platform runs (consent, every log processed,
findings captured as tests, vendor bugs, re-running what failed, the
archive, regenerating `docs/platform-support.md`), use the
`platform-support` skill (`.claude/skills/platform-support/`).
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
  support. Say where a vendor-only claim comes from. Where a consented
  GitHub Actions run covered that toolchain, cite it
  (`docs/platform-support.md` or the run link) in preference to the status
  page alone.
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

`docs/guidelines/` is about *how to write* C++ well (every Core Guidelines
rule, worked examples, safety, modern idioms, design principles, project
conventions). To refresh it
against upstream or import a new reference, use the `refresh-guidelines` skill
(`.claude/skills/refresh-guidelines/`). Keep `docs/guidelines/sources.md`
updated whenever a guidelines page changes.

The full C++ Core Guidelines are a git submodule at `modules/CppCoreGuidelines`
(pinned to the edition the pages track). Run
`git submodule update --init` after cloning. Third-party sources go under
`modules/`; don't edit them.

**Every Core Guidelines rule is in our docs** (#233), never a selection:
`docs/guidelines/core-guidelines/` is generated from the submodule by
`python3 tools/gen_core_guidelines.py`, one page per official section listing
every rule (the ### headings, the profiles' Type/Bounds/Lifetime rules and
their sub-points, and rules the summaries list with no text yet) with its
official ID, title and link, and the pages of ours that cite it. Never edit it
by hand. It links instead of copying the rule text, because the Guidelines'
licence permits copies for personal or internal business use only. `--check`
fails if a rule is missing or a page is stale (run it whenever the submodule
or a guidelines page changes, since the "ours" links come from our pages);
`--upstream` fails if the pin is behind upstream `master`; `--self-test` runs
its samples. Don't describe `core-guidelines.md` (the worked examples) as the
important rules: it covers the rules someone has written an example for.

**Every rule gets an example of our own and, where its claim can be checked,
a unit test** (#235). Examples go in
`docs/guidelines/core-guidelines/examples/<page>.md` (one page per section,
named like its index page; the conventions and template are in that folder's
`README.md`): a `## <ID>: <official title>` heading per rule in official
order, our own BAD/GOOD code (`compile-fail="regex"` when the compiler
rejects the mistake, `bad` when it compiles but is wrong), or
`*No example: <reason>.*` for a rule that isn't about code. Never copy the
Guidelines' text or examples. Tests go in `tests/guidelines/<page>_test.cpp`
(the `guidelines` area), with a `// <ID>: <claim>` comment above each `TEST`
(`tests/guidelines/README.md`). The generator reads both: each rule in the
index links its example and test, the index README counts what's left, and
`--check` fails on an ID the Guidelines don't have. Regenerate the index in
every PR that adds examples or tests; after a rebase, regenerate it rather than
merging its conflicts.

`modules/perf-report` is the owner's Linux performance-analysis guide
(https://github.com/homer6/perf-report, also perf-report.com: perf, bpftrace,
flame graphs, CPU/memory/disk/network analysis). Link to it for profiling
commands instead of duplicating them.

## Security section

`docs/security/` is about writing C++ that holds up against an attacker
(#286, v0.5.0): threat models and trust boundaries, untrusted input, integer
and size checks, lifetimes across callbacks, fail-closed errors, secrets,
process and file boundaries, resource limits, hardening, verification, supply
chain, deployment and policy. `docs/security/README.md` lists the pages and
each page's status.

- **Security isn't safety.** `docs/guidelines/safety.md` and the Core
  Guidelines cover correctness (no UB, no dangling, no leaks). The security
  pages ask what an attacker can make the code do: which input crosses a trust
  boundary, what it can exhaust, what a failure grants. Link the safety pages
  for the foundations instead of repeating them.
- **Positive framing.** The pages say what makes C++ code secure: C++ written
  to the guidelines, with library hardening, verification and a small trusted
  core. Never adopt the premise that C++ isn't memory-safe (the owner: C++ is
  more memory-safe, when the guidelines are followed, than languages that
  claim memory safety).
- **Page shape:** the one-line summary quote, `## Mental model`, then sections
  of rules labelled **MUST**, **SHOULD** or **MAY**, each with the reason, our
  own compile-checked example where code helps, and its sources; then
  `## See also` and a `Tests:` link. A section on how this repo does it is
  welcome where the repo is a worked example (presets, fuzz targets,
  clang-tidy, pinned inputs).
- **Citations:** Core Guidelines IDs link to our index
  (`../guidelines/core-guidelines/<section>.md`). SEI CERT rules are cited by
  ID and linked to CERT's site, **never copied** (CERT's licence). Vendor docs,
  man pages and papers back the facts; an AI summary is never a source.
- **Verify like the rest of the repo:** every claim about a compiler, library
  or OS is checked locally (GCC 15, Clang 22, GCC 16 or Linux in the `gcc:16`
  image) and states the version it was checked on. An imported claim that
  can't be verified is fixed or removed, and the PR says which.
- **Tests:** one file per page, `tests/security/<page>_test.cpp` (the
  `security` area), starting `// Checks claims made by docs/security/<page>.md.`,
  with the usual `// "Section": claim` comments. Platform-specific claims
  (POSIX, Linux) are gated with `#if` and skip with a reason elsewhere; a test
  that would change the test process's privileges skips when it runs as root.
  The security area also gets `CPPDOCS_FUZZ_DIR` (the `fuzz/` folder) and its
  include path. `tests/security/` and `fuzz/` must pass
  `tools/run_clang_tidy.sh`.
- **When a page is verified or changes what it recommends,** mark it in
  `docs/security/README.md` and update the `cpp-security` skill's page table
  and checklist in the same PR.

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

Run the checks locally; GitHub Actions workflows are manual only and never
replace them (see "GitHub Actions (manual only)"). `tools/check_snippets.py`
compiles every ```cpp block with `-std=c++26 -fsyntax-only -pedantic-errors`. **Both `docs/guidelines/` and
`docs/examples/` must pass cleanly** — every block compiles, or carries an
honest `bad` / `nocompile` / `compile-fail` / `needs=` tag. Each block is
compiled after `tools/snippet_prelude.hpp` (common headers and placeholders
like `Widget`, `use()`, `process()`). Tag the fence's info string:

| Fence | Meaning |
|---|---|
| ` ```cpp ` | Must compile at namespace scope |
| ` ```cpp body ` | Statements; checked inside a function body |
| ` ```cpp nocompile ` | Can't compile here: depends on undeclared context, third-party headers or another platform's syntax |
| ` ```cpp bad ` | A BAD example that compiles but is wrong (UB, dangling, a leak, a race, a missing `const`). It **must** compile, and is counted separately (#262) |
| ` ```cpp bad warns="regex" ` | … and with `-Wall -Wextra` the compiler must warn, matching the regex (GCC/Clang; not checked on MSVC) |
| ` ```cpp run ` | Has a `main`: also built with `-fsanitize=address,undefined` and run; it must exit 0 with no sanitizer report |
| ` ```cpp bad run ` | Built and run the same way, and a sanitizer **must** catch it: proves "this is UB / a memory error" |
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

Prefer making a snippet self-contained over tagging it `nocompile`,
`compile-fail` over `nocompile` for type errors, and `bad` over `nocompile`
for code that compiles but is wrong. Run blocks need the sanitizer runtimes:
Clang 22 has them locally, GCC on Linux has them, Homebrew GCC on macOS
doesn't, and MSVC/clang-cl skip runs; where they're missing the block is still
compile-checked and the run is reported as skipped with the reason
(`--no-run` skips runs on purpose). For an issue, check only the
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

Toolchains: `--toolchain` is `auto` (GCC or Clang, told apart by
`--cxx --version`), `gcc`, `clang`, `msvc` (`cl /Zs`, prelude via `/FI`) or
`clang-cl`. Fence tags mean the same everywhere, except that on `msvc` a
`compile-fail="regex"` block only has to fail (the regexes are GCC/Clang
wording), and module blocks skip with a reason on `msvc` and `clang-cl` (a
separate step). Each toolchain has its own baseline in
`tools/snippet_baseline/` (`gcc.txt`, `clang.txt`, `msvc.txt`, `clang-cl.txt`).
`clang-linux.txt` is `clang.txt` for Linux's libc++ (Ubuntu's LLVM packages,
glibc), which has a few things macOS's doesn't (the time zone database,
`<stdbit.h>`); keep the two in step.
`--json FILE` writes the results as JSON (schema `cpp-docs/results/1`, kind
`snippets`) for the results collector; keep the text output and its counts
unchanged for GCC and Clang.

Each block has a stable ID, `<file>#<heading-slug>/<n>`, shown next to every
failure. `tools/snippet_baseline/gcc.txt` (the list of known failures) is
**empty** and must stay that way: fix a failing block or tag it honestly —
never add it to the baseline. The `--baseline` run still guards against stale
entries if one is ever added.

`-fsyntax-only` is more than a parse: on GCC and Clang it catches type errors,
errors in templates that are instantiated, and failed constant evaluation —
the same as `-c`. It misses templates that are never instantiated, link errors
(a declared but undefined function), and anything at run time. The summary
counts skipped blocks by reason, then the `bad` blocks and the runs; keep the
`nocompile` count falling.

## Running the tests

`tests/` is a GoogleTest suite that turns each page's claims into assertions
(#41). It's the only thing CMake builds. Run it locally; nothing runs on
GitHub automatically.

**Run only the tests that apply to an issue.** The full suite runs only
before a release. A GitHub Actions run (manual, with the owner's consent; see
"GitHub Actions") adds the platforms we can't run here, but never replaces
these local runs.

| When | What to run |
|---|---|
| Working an issue / opening its PR | The test areas the change touched (build the area target, `ctest -L '^<area>$'`) on `gcc-15` and `clang-22`, plus `gcc-16` when the change needs a GCC 16 feature. The snippet checker on the changed pages only, with both compilers. `check_links.py` on the changed files and `gen_index.py --check`. `gen_core_guidelines.py --check` when the change touches `docs/guidelines/` or the submodule. `check_workflows.py` when the PR touches `.github/`. If you edit a baseline file, run that compiler's `--baseline` run so the file stays consistent. |
| Cutting a release | Everything: `tools/run_tests.sh` on all three presets and the two `*-asan` presets, the full `--baseline` snippet runs for `docs/examples` and `docs/guidelines` with both compilers, the link and index checks, and `check_workflows.py`. The counts go in the release notes, next to the consented Actions runs of every platform workflow (see "Release checklist"). |
| A claim about a platform we can't run locally (MSVC, FreeBSD, MinGW…) | The local runs above, then ask the owner for a consented run of that platform's workflow, scoped to the page's area (see "GitHub Actions"). Until it's green, the page cites the vendor's status page and says so. |

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
tools/run_tests.sh clang-22-asan  # the suite under ASan + UBSan (Clang 22 + libc++)
tools/run_tests.sh gcc-16-asan    # the same with GCC 16, in the gcc:16 image
```

**Hardened builds** (#305): `CPPDOCS_HARDEN=ON` (the `gcc-15-hardened`,
`clang-22-hardened` and `gcc-16-hardened` presets, `RelWithDebInfo`) builds the
suite with library hardening for the library in use, stack protection,
`-ftrivial-auto-var-init=zero`, control-flow protection, `_FORTIFY_SOURCE=3`
on glibc, PIE and full RELRO (`/sdl /guard:cf` on cl), each flag probed first;
the flags used go into the results JSON (`compiler.harden_flags`). In CI,
`CPPDOCS_HARDEN=1` in a job's environment does the same and adds `-hardened`
to the compiler label (its own results and toolchain page, no snippets).
Details in `docs/testing.md`.

**Fuzzing** (#307): `fuzz/` holds libFuzzer targets for the security pages'
parsers (`CPPDOCS_FUZZ=ON`, Clang; `tools/ci/fuzz.sh`; the `fuzz` job in
`linux.yml`). A crash found by fuzzing is fixed, and its minimized input is
saved in `fuzz/corpus/<target>/`, which `tests/security/fuzz_corpus_test.cpp`
replays on every toolchain. See `fuzz/README.md`.

**clang-tidy** (#308): `.clang-tidy` (`cppcoreguidelines-*`, `cert-*`,
`bugprone-*`, every finding an error) covers `tests/security/` and `fuzz/`.
Run `tools/run_clang_tidy.sh build/<preset>`; the `clang-tidy` job in
`linux.yml` runs it too. Fix a finding, or keep it with
`// NOLINT(<check>): <why>`; never a bare NOLINT.

**Sanitizers** (#263): `CPPDOCS_SANITIZE` (`address,undefined`, `thread`, or
`address` on MSVC/clang-cl) builds the whole suite with the sanitizer; the
`*-asan` presets set it, and in CI `CPPDOCS_SANITIZE` in a job's environment
makes `tools/ci/run.sh` pass it, set the sanitizer options, add `-asan`/`-tsan`
to the compiler label (its own results and toolchain page) and skip snippets.
A test that can't work under a sanitizer skips with the reason (e.g. ASan's
`operator new` can't throw `bad_alloc`). ThreadSanitizer runs on Linux with
libstdc++ only: packaged libc++ isn't TSan-instrumented, so it reports false
races and crashes in its unwinder. Details in `docs/testing.md`.

The script runs `cmake --workflow --preset <preset>` (configure, build and
test into `build/<preset>/`) and then prints every skipped test with its
reason. `gcc-15`, `clang-22` and `gcc-16` are the owner's local presets. The
portable presets `gcc`, `clang`, `clang-libcxx`, `msvc`, `clang-cl` and
`mingw` (for CI and other machines) take the compiler from `CXX` and set
`CPPDOCS_TOOLCHAIN`, which makes the configure step fail if the compiler is
the wrong toolchain; `docs/testing.md` has the table and the VS Developer
shell setup for `msvc`/`clang-cl`. `tools/run_tests.ps1` is the PowerShell
counterpart; `tools/run_tests.sh --json FILE <preset>` (or
`tools/ctest_to_json.py --build-dir build/<preset>`) writes the results as
JSON (kind `tests`). The presets live in `CMakePresets.json`; `docs/testing.md` explains each one
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
- **Tests must build on MSVC too** (`/std:c++latest /permissive- /EHsc
  /utf-8 /Zc:__cplusplus`, MS STL, LLP64: 4-byte `long`, 2-byte `wchar_t`,
  `long double` = `double`, signed `char`) and on clang-cl and MinGW. No POSIX
  headers or GCC builtins without an `#if`; locale names differ (`de-DE` on
  Windows); a toolchain chain (`#if defined(__clang__) … #elif
  defined(__GNUC__)`) needs a branch for `_MSC_VER` / `_MSVC_STL_VERSION`
  (clang-cl defines both `__clang__` and `_MSC_VER`). Gate what differs with
  `CPPDOCS_REQUIRE`, `#ifdef` or a per-toolchain expectation, never by deleting
  the test.
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

## GitHub Actions (manual only)

**Why we test on every platform** (the owner, #182): *"the whole point is to
test every language feature documented and all examples on all supported
platforms/architectures/compilers .... that way, we see the level of
language support and feature support in the entire industry"*. So:

- **Every feature and every example, everywhere.** The platform runs that
  count (before a release, and for `docs/platform-support.md` and the
  README table) use empty `areas`: every test area and every snippet, on
  every platform × compiler × architecture in the workflows. A one-area run
  only proves the plumbing or a fix; it's never the result we publish.
- **The results are a support map.** A feature a toolchain doesn't have
  shows as a skip with a reason (`CPPDOCS_REQUIRE`, a `needs=` tag, a
  version gate naming the compiler and the bug or paper), never as a deleted
  or hidden test, so the counts and skip reasons say how far each compiler
  and standard library has got. A skip for a compiler bug names the bug.
- **New platforms, compilers and architectures** that are supported and
  can run on GitHub's runners natively or in a KVM-accelerated VM belong in
  the workflows; widening the matrix is part of the goal. **No emulated
  platforms for now** (the owner, #207: *"let's remove support for qemu and
  emulated platforms"*): no QEMU user-mode jobs and no VM under emulation
  (FreeBSD aarch64). Ask the owner before adding one back.
- **Every every-area run is read in full, captured and archived**, with the
  `platform-support` skill, which has the commands and the
  `digest_run.py` log digest (the
  owner, #186: *"these full action runs (and logs) are pretty useful for
  archiving exactly the level of support and for each feature ... we'll need
  to read each of them fully (and capture them in test pass/fail unit tests)
  and report those in the platform_support full report"*). After each one:
  1. **Process every job's log in full, every time**, not just the failed
     ones: build errors, test failures, every skip and its reason, snippet
     failures, and warnings that show a toolchain gap. The raw logs aren't
     kept (the owner: *"we don't need to store the raw log, but we should
     process it each time so we can report any compiler/platform bugs to the
     vendors or fill the report properly"*), so everything worth keeping is
     taken out of them in this step.
  2. **Capture each finding in the suite.** A wrong page claim is fixed
     (an issue and a PR each, the fix loop). A missing feature or a compiler
     bug becomes a skip whose reason names the compiler, the version and the
     paper or bug (`CPPDOCS_REQUIRE`, a version gate, `needs=`), so the
     result is recorded as a test outcome and shows in the report. Never
     leave a finding only in a log or a chat.
  3. **Compiler and platform bugs go to the vendors.** Each one gets its
     own issue here (one bug per issue, titled `vendor: <toolchain> <bug>`,
     never a list in a PR body or a shared issue; the owner, #220) with a
     minimal reproducer, the exact compiler and image
     version, the run link and the vendor's existing bug if there is one
     (search GCC Bugzilla, LLVM GitHub, Developer Community for MSVC, and
     the others first). The report's known-gaps section lists them with
     their links, and `docs/platform-support/known-bugs.json` names that
     issue. Filing a new bug with a vendor is posting outside this
     repo: draft it in the issue and file it only with the owner's yes.
  4. **Re-run only what failed**, on `dev`, after the fixes merge (a new
     dispatch of the failing workflows, since a GitHub re-run reuses the old
     commit), until every combination is green.
  5. **Archive the results, not the logs:** the results JSON and run
     metadata go in the repo under `docs/platform-support/runs/<run-id>/`
     (`tools/collect_results.py`, #184). The report links the run IDs.
     The archive holds only what the report uses (#214): compact JSON, and
     the collector removes a job file a newer run superseded and any run
     folder not named on its command line (git history keeps them). So
     always pass every run the report should show.
  6. **Regenerate `docs/platform-support.md`**, the full report's entry
     page (platform summary, the feature support matrix, known gaps), its
     per-toolchain pages in `docs/platform-support/` (every feature and
     example), and the README's "Tested platforms" table from the archive,
     and commit them in
     a PR. Keep both current: a page, test or toolchain change that alters
     a result means the report is regenerated after the next consented run.

The owner's rule (#141): *"we are not going to have any github actions that
run automatically... manual invocation only"*.

- **Every workflow's only trigger is `workflow_dispatch`.** No `push`,
  `pull_request`, `schedule`, `workflow_run`, `release` or any other event,
  not even for lint. The one addition: a platform workflow may also have
  `workflow_call`, next to `workflow_dispatch`, so the manually dispatched
  `all.yml` can run it (`uses: ./.github/workflows/<file>`). Each workflow sets top-level `permissions: contents:
  read`, no job grants write, and every `uses:` is pinned to a full
  40-character commit SHA with the version in a comment
  (`uses: actions/checkout@<sha>  # v4.2.2`); local `./` actions are fine.
- **Never start a workflow** (`gh workflow run`, the API, a re-run or anything
  else) without the owner's explicit consent, given in the moment and naming
  the workflow, the ref and the inputs. Consent for one run doesn't cover the
  next. Agents write and check workflows locally; only the orchestrating
  session asks the owner for a run.
- **`python3 tools/check_workflows.py`** enforces the rules above. Run it in
  every PR that touches `.github/` and in release checks;
  `--self-test` runs its built-in good and bad samples. It needs no
  third-party packages and exits 0 when there are no workflows.
- **The default-branch rule.** GitHub dispatches a workflow only if its file
  exists on the repo's default branch; then it can run on any ref that has
  the file. `main` is the default branch and has every workflow file
  (since v0.3.0), so runs on `dev` start normally. Make `dev` the default
  only when a new workflow file is on `dev` but not yet on `main` and must
  be started directly (`all.yml` calls the others, so they don't need it),
  and switch back to `main` before the release PR (#140, #282). **Never
  change the default branch, in either direction, without the owner's
  explicit permission for that change in the moment.**
- Running and reading (details in
  [`.github/workflows/README.md`](.github/workflows/README.md)):
  `gh workflow run <file>.yml --ref <branch> -f key=value`, `gh run watch
  <id>`, `gh run view <id> --log-failed`, `gh run download <id> -D <dir>`.
- **When to ask the owner for a run:**
  - **before a release**: required (the release gate, #140). Every platform
    workflow needs a green consented run on `dev` first;
  - **for a claim about a platform we can't run locally** (MSVC, clang-cl,
    MinGW, FreeBSD or other architectures): a page that states
    how that toolchain behaves is better backed by a run than by a vendor
    status page alone.

  Never ask just to refresh the badges or the platform page.
- **How to ask:** the orchestrating session names the workflow, the ref and
  every input, e.g. *"May I run `linux.yml` on `dev` with `areas=^strings$`,
  `tests=true`, `snippets=true`?"*, and waits for a yes. **Start small**: one
  area on one workflow proves the plumbing (toolchain setup, artifacts, the
  summary) cheaply. Then go wide: empty `areas` (every area and snippet) on
  every workflow, usually `all.yml`, each with its own consent. The
  every-area runs are the goal (see "Why we test on every platform").
- **The fix loop:** each failure gets its own issue (the workflow, job, run
  link and failing test or snippet ID) and a PR into `dev` that fixes the
  page, the test or the script, with the local runs in its Tested section.
  After it merges, start a new dispatch on `dev` of only the workflows that
  failed (with the owner's consent; not `gh run rerun`, which reuses the old
  commit). Repeat until that workflow is green.
  A wrong page claim is fixed on the page, not hidden in a baseline.
- **How results feed the repo:**
  - **Pages:** a claim about MSVC, FreeBSD or another non-local toolchain can
    say it was verified there, citing `docs/platform-support.md` or the run
    link, once a consented run passed the test that checks it.
  - **Baselines:** a report-only toolchain becomes enforced in three steps:
    seed `tools/snippet_baseline/<toolchain>.txt` from the run (the failing
    IDs in its `results-*.json`, `snippets.failures[].id`, or
    `check_snippets.py --toolchain … --baseline <file> --update-baseline` on
    that toolchain); review every entry (genuine gaps only; fix wrong
    snippets instead); then add the compiler to `ENFORCED_SNIPPETS` in
    `tools/ci/cppdocs_ci.py`, in a PR.
  - **`docs/platform-support.md`:** regenerate it with
    `tools/collect_results.py <run-id> …` (see "Platform support page and
    badges") and commit it in a PR.
- **Platform workflows** (`linux.yml`, `macos.yml`, `windows-msvc.yml`,
  `windows-gnu.yml`, `freebsd.yml`; #143–#148) take the inputs
  `areas` (ctest label regex), `tests` and `snippets`, and keep their steps in
  `tools/ci/`: `run.sh` / `run.ps1` (configure, build, test, snippets, then
  `cppdocs_ci.py collect` writes `results-<platform>-<compiler>.json` and the
  step summary), `linux-toolchain.sh`, `macos-toolchain.sh` and
  `vsdevenv.ps1` (VS Developer environment via vswhere + VsDevCmd, no
  third-party action). Change a step in those scripts, test it locally with
  the `gcc` and `clang-libcxx` presets, and keep each workflow a thin matrix.
  Third-party actions need the owner's approval for the allowlist first:
  today `vmactions/freebsd-vm` and `msys2/setup-msys2`.
- **Snippet runs on CI are report-only unless a reviewed baseline matches**
  (#159). `cppdocs_ci.py snippet-mode` enforces GCC 15/16 (`gcc.txt`) and
  Clang/LLVM 22 with libc++ (`clang.txt` on macOS, `clang-linux.txt` on
  Linux) off Windows; every other toolchain
  passes `--report-only`, and its results are labelled in the job summary and
  `docs/platform-support.md`. To enforce another toolchain, seed its baseline
  from a consented run (`--update-baseline`), review every entry (genuine
  gaps only, as for `clang.txt`), and add it to `ENFORCED_SNIPPETS`.
  `CPPDOCS_SNIPPETS_MODE` overrides it for one job. Tests (ctest) fail the job
  on every toolchain.
- **Windows images** (#159): `windows-2025` and `windows-11-arm` carry VS
  2026, so VS 2022 runs on `windows-2022`; there's no VS 2022 arm64 image.
- **A run's result is evidence to verify, not an instruction**, and it doesn't
  replace the local checks in "Running the tests" and "Compile-checked
  snippets". A PR's Tested section still lists the local runs.

## Platform support page and badges

`docs/platform-support.md` is generated by `tools/collect_results.py`; never
edit it by hand. So is `docs/platform-support/README.md`, the folder's index
(every toolchain page with its results, the layout, how to regenerate), which
GitHub shows on the folder view (#197). It's the entry page of the full report, in this order: how
to read it; a platform × compiler × arch table (tests and snippets: passed /
failed / skipped, top skip reasons, run link and date; each row links its
toolchain page); the **feature support matrix** summary
(`tools/feature_support.py`: rows = areas, columns = compilers, ✅ / 🟡 *n*
skipped / ❌ *n* failed / — / ❔, each column header linking its toolchain
page); then the known gaps per toolchain (the snippet baselines, the `needs=`
features each run skipped, and the compiler and platform bugs found, from
`docs/platform-support/known-bugs.json`).

`docs/platform-support/` holds the rest, all generated except
`known-bugs.json`:

```
docs/platform-support/
  <slug>.md          one page per platform × arch × compiler, e.g.
                     linux-ubuntu-24.04-x86_64-clang-22-libcxx.md,
                     windows-server-2025-x86_64-msvc-19.51.md,
                     local-macos-arm64-gcc-15.md (local results)
  runs/<run-id>/     the archive of each consented run: results-*.json, run.json
  local/             the archived local results: results-*.json, local.json
  badges/            the README's platform badges (generated SVGs)
  known-bugs.json    compiler and platform bugs found (maintained by hand)
  notes.md           the platform support notes (maintained by hand)
```

**The platform support notes** (`docs/platform-support/notes.md`, #226) are a
short, hand-written reading of the results: the level of support per
compiler, platform and architecture, what decides it, what nothing supports
yet, and the limits of the results. The collector puts them at the top of
the folder README. Every claim comes from the results (counts, skip reasons,
`known-bugs.json`), never from memory or vendor pages, and the file's
`<!-- notes-run: <id> -->` marker names the run they describe. **Update them
with every report refresh from a new full run**: re-read the results, revise
each claim, and move the marker; `collect_results.py --check` fails while the
marker names a different run than the newest one in the report. The check
covers only the marker, so moving it proves nothing about the text: every
claim is re-checked against the results, with a "Notes claims" table in the
PR (see "Release checklist", step 3).

A toolchain page has a header (OS or image, compiler and version, standard
library, architecture and byte order, the run link, date and commit, the
areas covered, snippets enforced or report-only), totals, a table of areas,
then per area (in `<details>`) every page with every feature (each
GoogleTest case, labelled by the `// "Section": claim` comment above its
`TEST`) and every example section (the ```` ```cpp ```` blocks, by their IDs):
✅ / ❌ / ⏭ with the skip reason / — / ❔. Areas the job didn't run are named,
not listed. The slug comes from the OS, arch, compiler family, major version
(MSVC: major.minor) and, for Clang, the standard library, so it stays the
same across runs and links don't churn. The same run writes the README's "Tested platforms" table, the
block between `<!-- platforms:start -->` and `<!-- platforms:end -->`: one
row per platform × compiler × arch with ✅/❌ and counts for tests and
snippets, the local runs first, then each run's link. Never edit that block
by hand either (`gen_index.py` leaves it alone, and the collector leaves the
index alone). **The README's badges are ours, not GitHub's** (#230): the
collector writes one SVG per platform (Linux, macOS, Windows, FreeBSD) and one
for the local runs into `docs/platform-support/badges/`, from the same results
as the report, and the badge row between `<!-- badges:start -->` and
`<!-- badges:end -->`. Each says "✓ N toolchains" (green) when no toolchain
has a failing test or a snippet failure outside its baseline where snippets
are enforced, else "✗ n of N failing" (red), and links the platform support
notes. Report-only snippet failures don't count. GitHub's workflow badges
follow one branch and miss the local runs, so they aren't used. Never edit the
block or the SVGs by hand; `--check` covers both. Right under the badges, a bold link points to the
platform support notes (`docs/platform-support/notes.md`; #226), and under it
a bold link to the full support per platform, compiler and architecture
(`docs/platform-support/README.md`, which indexes every toolchain page; #215).

- **The badges and the page change only from consented runs.** Never start a
  workflow to refresh them (that needs the owner's consent, asked in the
  moment by the orchestrating session); `all.yml` runs the platform
  workflows in one dispatch, with `platforms`, `areas`, `snippets` and
  `tests` inputs.
- **After a consented run**, refresh the page from it, together with the
  release's local results (`run_tests.sh --json` for `gcc-15`, `clang-22`,
  `gcc-16`, and `check_snippets.py --json` for the snippet runs, written as
  `results-<label>.json` / `results-<label>-snippets.json` into one folder):

  ```bash
  tools/collect_results.py <run-id>… --local <folder>   # archives both, writes the report
  tools/collect_results.py <run-id>…                    # reuses docs/platform-support/local/
  tools/collect_results.py <run-id> --stdout            # preview the entry page
  ```

  `--local <folder>` replaces the archived local results in
  `docs/platform-support/local/` (with `local.json`: date and the commit
  archived from); without it the archive is used (`--no-local` leaves local
  results out). Commit the page, the toolchain pages, the README and the new
  `runs/` and `local/` files together.

  The first time it sees a run ID it fetches the run's `results-*.json`
  artifacts with `gh run download` and its URL, date, commit and jobs with
  `gh api` (read-only), and **archives** them in
  `docs/platform-support/runs/<run-id>/` (each job's `results-*.json` and
  `run.json`: id, URL, date, commit, inputs, jobs and conclusions, a digest
  of the counts; never logs). After that it reads the run from the archive,
  so the report can be rebuilt once GitHub's retention has deleted the
  artifacts; commit the archive with the page (`--no-archive` skips writing
  it). It keeps the archive to what the report uses (#214): files are written
  as compact JSON (the local ones too), a job file that a newer run
  superseded is removed from the older run's folder (`run.json` keeps its
  counts), and a run folder not named on the command line is removed. Both
  stay in git history (`git log -- docs/platform-support/runs/<id>`), and the
  archive stays at about one full run (about 9 MB) instead of growing by
  11 MB per run. It exits 1 naming the file and field when a result doesn't match
  schema `cpp-docs/results/1`.
- **The matrix's rows come from the checkout**, not from the results: the
  test names and section comments in `tests/`, and the snippet IDs from
  `check_snippets.py`'s own parser. A job's cells use its per-test and
  per-block `results` list (written by `ctest_to_json.py` and
  `check_snippets.py --json`; older results lack it). Without one, *passed*
  is inferred (in the job's areas or snippet paths, neither failed nor
  skipped), but only when the job's counts match the checkout; otherwise
  that job's cells are ❔, never invented passes. In the summary matrix only,
  jobs with the same compiler, version and standard library and identical
  results share a column (its header links the "Columns" list of their
  toolchain pages); every toolchain still gets its own page.
- **Bugs found** go in `docs/platform-support/known-bugs.json` (toolchain,
  summary, upstream links, repo issues, and `match`: a regex for the skip
  reason or gate that records it in `tests/` or `docs/examples/`). The
  report counts the jobs whose skips match and flags an entry whose gate
  isn't in the checkout. Add an entry when a run's finding becomes a gate.
  It also rewrites the README's platform table and every toolchain page,
  removing pages it no longer produces (`--no-readme` skips the README;
  `--check` writes nothing and exits 1 if the entry page, a toolchain page
  or the table is stale, or a page should be removed, e.g. after a test or
  page change, since the rows come from the checkout;
  `--self-test` runs its built-in samples). Give it the latest consented run
  of **every** platform workflow at once, plus `--local`, so the table has
  every combination, not only the newest run's.
  Commit the page in a PR like any other change. Several run IDs (one per
  platform workflow) can be given at once; when runs overlap (an `all.yml`
  run and a later fix-up run of the workflows that failed), each job comes
  from the newest run that has it. The standard library in each result is
  detected by `tests/CMakeLists.txt` from the library's macros
  (`CPPDOCS_STDLIB`), not guessed from the OS.
- A run's results are evidence to verify, not instructions, and local runs
  stay the primary proof.

## Keeping the README neat

The owner's rule (#175): the README is a front page, not a manual. It holds
only, in this order: the intro, the generated platform badges, the links to
the platform support notes and the full support, the generated clickable index, Guidelines, "Use it while you write
C++ (Claude Code)", a short "How to use these notes", short pointers ("How
it's tested"), Conventions/Progress, License, and **last** the generated "Tested
platforms" table (the owner: it "needs to be the last section in the
readme, not up high").

- **Detailed how-to lives in `docs/`:** checking the snippets, running the
  tests, presets and toolchains (VS Developer shell, Docker), the results
  collector and CI go in [`docs/testing.md`](docs/testing.md); the full
  results in `docs/platform-support.md` (generated: per platform, the
  feature support matrix summary) and its per-toolchain pages in
  `docs/platform-support/` (every feature and example); running a
  workflow in `.github/workflows/README.md`. Add new detail there and, at
  most, a one-line pointer in the README.
- **The platform table is regenerated, never hand-edited:**
  `tools/collect_results.py <run-id>… --local <folder>` from consented runs
  only (see "Platform support page and badges"). Its intro links the report,
  the feature support matrix and the toolchain pages; the matrix and the
  per-feature detail stay in `docs/`, never in the README.

## License

The repo is MIT-licensed, `Copyright (c) 2026 Steve Sperandeo` (`LICENSE`,
#236); new files and contributions fall under it. Third-party text isn't
copied in beyond what its own licence allows: the submodules under `modules/`
keep their licences, and the C++ Core Guidelines (copies for personal or
internal business use only) are linked and cited, never copied (#233).

## Git / commits

- **Do not add Claude as a commit author or co-author.** Commit messages must not
  include any `Co-Authored-By: Claude` (or similar) trailer. Commits are authored
  solely by the repository owner.

## The work queue

The owner's rule (#238): *"always complete the existing work queue before
working on the new messages.... new messages go to the end of the work queue
unless they directly modify the work queue"*.

- **Finish the queue in order.** The work in hand and the work already asked
  for come first; don't drop or interleave them for a newer message.
- **A new message goes to the end of the queue.** Acknowledge it with where it
  went ("added after #228 and the release"), then carry on with the current
  item.
- **The exception: a message that changes the queue itself** takes effect at
  once: it reorders, cancels or adds to queued work, or changes how the
  queue is worked (like this rule). A question about the work in hand is
  answered right away, without starting new work.
- **Report the queue** when a session ends or the owner asks: what's done,
  what's in progress, what's next.

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
- **Issues worked serially may stack** (v0.5.0's security PRs): each issue is
  worked on its own feature branch in its own worktree
  (`~/dev/homer6/cpp-docs-worktrees/issue-<n>`, branch `<area>/<n>-<slug>`),
  and each PR's base is the previous issue's branch; the first PR's base is
  `dev`. GitHub retargets the next PR when one merges with `--delete-branch`.
  When `dev` moves, rebase the whole stack onto it in one pass
  (`git rebase --update-refs origin/dev` from the top branch, then
  `git push --force-with-lease` for each branch), never by merging `dev` in.
- **A separate release branch** (like `dev-0.5`) only when the owner asks for
  one. v0.5.0 started on `dev-0.5`; the owner dropped it and retargeted the
  stack to `dev` before review.
- **The only PR into `main` is the release PR** (`dev` → `main`), opened when
  the owner names a version. Its merge commit is tagged `vX.Y.Z` and gets a
  GitHub release; the notes live in `docs/releases/vX.Y.Z.md`. Follow the
  "Release checklist" below.
- **Open PRs ready for review, never as drafts.**
- **Every PR keeps `README.md` and `CLAUDE.md` current.**
  - `README.md`: regenerate the clickable index (`python3 tools/gen_index.py`)
    whenever a page is added, moved or retitled. Edit the prose when the change
    touches it: a new area, tool, skill, check or workflow (the detail goes in
    `docs/testing.md`, the README gets a pointer at most; see "Keeping the
    README neat").
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

## Release checklist

A release (the owner names the version) needs all of these, in order:

1. **Local runs** (still required): everything in the "Cutting a release"
   row of "Running the tests", with the counts.
2. **Green consented Actions runs of every platform workflow** (`linux`,
   `macos`, `windows-msvc`, `windows-gnu`, `freebsd`) on the `dev`
   commit being released, each started only with the owner's consent for
   that run. Every failure is fixed through the fix loop in "GitHub
   Actions" (an issue and a PR into `dev` each), then re-run green. A fix
   that lands on `dev` after a run means re-running the workflows it can
   affect.
3. **The notes** (`docs/releases/vX.Y.Z.md`) carry the local counts and a
   per-platform table of the runs: the run links and the passed / failed /
   skipped counts, taken from `tools/collect_results.py <run-id>… --local
   <folder> --summary --stdout`. Commit the regenerated
   `docs/platform-support.md` with them, and **regenerate the platform
   support notes** (`docs/platform-support/notes.md`) from the release run:
   every claim re-read against its results, the marker on that run (#226).
   The release notes link them. **Re-derive the notes, never patch them**
   (#350: v0.4.0 shipped with stale claims because only the numbers known
   to have changed were updated):
   - Go through the notes sentence by sentence. Every count, range, version,
     "only X has Y", "nothing has Y", "X lacks Y" and per-platform statement
     is checked against the release run's results JSON and the local results
     (per-test status, skip reasons, `known-bugs.json`), including the claims
     whose numbers didn't change. Ranges include the local results.
   - What the results can't show (a skip caused by the runner's CMake, not
     the compiler) is said that way, not claimed for the compiler.
   - The release PR carries a **"Notes claims" table**: each claim, the
     evidence (job, test or skip reason, count) and kept / fixed / removed.
   - `collect_results.py --check` only checks the notes' run marker. A green
     check says nothing about whether the sentences are true.
   - Never describe the notes as re-derived, regenerated or verified unless
     every claim was checked. Say exactly what was checked.
4. **The default branch is `main`** before opening the release PR, so the
   front page points at releases. If it was moved to `dev` for a new
   workflow, ask the owner's explicit permission to switch it back (see
   "GitHub Actions").
5. **The owner's explicit approval** of the release. Then open the release
   PR (`dev` → `main`); once it's merged with the owner's verdict, tag the
   merge commit `vX.Y.Z` and publish the GitHub release.

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
