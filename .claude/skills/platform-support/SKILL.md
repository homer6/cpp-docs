---
name: platform-support
description: Keep cpp-docs' platform support report current - plan and ask for the manual GitHub Actions platform runs, process every job's log in full, capture each finding in the test suite (a fix, or a skip naming the compiler, version and bug), draft vendor bug reports, re-run only what failed until everything is green, archive the results JSON, and regenerate docs/platform-support.md, its per-toolchain files and the README's Tested platforms table. Use when the owner says "run the platforms", "re-run what failed", "read the run", "update the platform support", "refresh the support matrix", "which compilers support X", after any consented every-area run finishes, or before a release.
---

# Keep the platform support report current

The goal (the owner, #182): **every documented feature and every example,
tested on every supported platform, architecture and compiler**, so the
report shows how far each toolchain, and the industry, supports modern C++.
The report is measured, never copied from vendor status pages.

What it produces:

| Output | What | How it changes |
|---|---|---|
| `docs/platform-support.md` | The entry page (the README links here): how to read it, the platform × compiler × arch table, the summary matrix (areas × toolchains), known gaps, and the compiler and platform bugs found | generated |
| `docs/platform-support/<platform>-<arch>-<compiler>.md` | One file per toolchain, local ones too: every area → page → feature with ✅ / ❌ / ⏭ (reason) / —, and the examples per section | generated |
| `docs/platform-support/runs/<run-id>/` | The archive: each run's `results-*.json` and its metadata (no logs) | written by the collector |
| `docs/platform-support/local/` | The local results used (GCC 15, Clang 22, GCC 16) | written by you, step 7 |
| `docs/platform-support/known-bugs.json` | Compiler and platform bugs: upstream link, repo issue, affected toolchains | edited by hand, validated |
| README "Tested platforms" | The last README section, between `<!-- platforms:start -->` and `<!-- platforms:end -->` | generated |

Never edit a generated file by hand. Repo rules (issue first, `dev`, no merge
without the owner's verdict, no AI attribution, the GitHub Actions rules) are
in `CLAUDE.md`: this skill follows them and doesn't repeat them all.

## 1. Plan the run and ask for consent

Every workflow is `workflow_dispatch` only, and **every run needs the owner's
consent in the moment**, naming the workflow, the ref and every input.

| Situation | Ask for |
|---|---|
| New plumbing (a new workflow, toolchain or script) | One workflow, one area: `areas=^strings$` |
| The report, a release, or a claim about a platform | `all.yml` on `dev`, `platforms=all`, `areas=` (empty: every area and snippet), `tests=true`, `snippets=true` |
| After fixes (step 5) | Only the workflows that failed, every area, as a **new dispatch** on `dev` |

> May I run `all.yml` on `dev` with `platforms=all`, `areas=` (every area),
> `tests=true`, `snippets=true`?

The owner may give standing consent for a loop (for example "after the
fixes, only re-run items that failed"). Follow it as given, and nothing
wider. If the permission check blocks a dispatch, stop and tell the owner;
never work around it.

```bash
gh workflow run all.yml -R homer6/cpp-docs --ref dev -f platforms=all -f areas= -f tests=true -f snippets=true
gh run list -R homer6/cpp-docs --workflow all.yml --limit 1      # the run ID
```

Watch it in the background (`gh run watch <id> --interval 120`, run in the
background so you're notified). QEMU and FreeBSD can take hours.

## 2. Gather everything the run produced

```bash
S=<scratchpad>
python3 .claude/skills/platform-support/scripts/digest_run.py <run-id> --out $S/digest-<run-id>.md
gh run download <run-id> -R homer6/cpp-docs -D $S/run-<run-id>     # results-*.json per job
gh run view <run-id> -R homer6/cpp-docs --json jobs --jq '.jobs[]|"\(.conclusion) \(.name)"'
```

- `digest_run.py` reads **every job's log**, not only the failed ones, and
  lists per job and across jobs: compiler and linker errors, crashes and
  timeouts, failed build targets, failed steps, and warnings by flag or code.
  It's read-only and writes to the scratchpad. **The raw logs are never
  stored** (the owner's rule), so whatever matters must come out now.
- The `results-*.json` files carry the test failures, every skip with its
  reason, the snippet failures and (since #184) every test's status.

A job that couldn't build an area has no test results for it; the digest's
"Failed targets" says which. The report shows those cells as "—", never as a
pass.

## 3. Process every finding

Go through the digest and every job's failures and skips. Each finding ends
in exactly one of these, **never only in a chat or a log**:

| Finding | Outcome |
|---|---|
| The page is wrong (the feature behaves differently than the page says) | Fix the page and its test: an issue and a PR (the fix loop in `CLAUDE.md`) |
| The test isn't portable (a GCC/Clang extension, a POSIX header, LP64, a locale name) | Fix the test so it checks the same claim portably: an issue and a PR |
| The toolchain lacks the feature | A skip with a reason: `CPPDOCS_REQUIRE(__cpp_lib_x)` / `CPPDOCS_REQUIRE_VERSION`, or a `needs=` tag on the snippet. The page notes the gap if it's worth knowing |
| A compiler or library bug (valid code rejected, wrong result) | A version gate (`_MSC_VER < 1950`, `__GNUC__ < 16`, `defined(_MSC_VER) && defined(__clang__)` for clang-cl) that skips with a reason naming the compiler, the version and the bug; the run-time half of the test keeps running where it can. Then step 4 |
| A CI or toolchain-setup problem (a missing SDK, a wrong flag, a step that stops early) | Fix the script in `tools/ci/` or the workflow: an issue and a PR |
| A warning that shows a gap or a bug | Treat it like the rows above; plain noise needs nothing |

Group findings that share a cause into one issue and one PR. Each PR runs the
affected areas locally on `gcc-15` and `clang-22` (and `gcc-16` when it
matters), plus the snippet checker on changed pages, and says in "Not
verified locally" which runner will confirm it.

Gate patterns used so far (copy them):

```cpp
// Preprocessor-only feature checks: __has_cpp_attribute only works in #if ([cpp.cond]).
#if __has_cpp_attribute(nodiscard) < 201907L
#error "[[nodiscard]] with a reason not recognized"
#endif

// A compiler bug: skip the compile-time half, keep the run-time check.
TEST(ConstantEvaluationRules, PolymorphismAtCompileTime) {
    EXPECT_TRUE(check_polymorphism());
#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER < 1950
    GTEST_SKIP() << "VS 2022 (MSVC 19.44) rejects a virtual call through a base reference in "
                    "constant evaluation (C2131); VS 2026 (19.5x) accepts it. Run-time result checked";
#else
    static_assert(check_polymorphism());
#endif
}
```

## 4. Compiler and platform bugs: vendors and `known-bugs.json`

For each bug:

1. **Search upstream first**: GCC Bugzilla (gcc.gnu.org/bugzilla), LLVM
   (github.com/llvm/llvm-project/issues), Microsoft Developer Community
   (developercommunity.visualstudio.com) for MSVC and the MS STL
   (github.com/microsoft/STL/issues), Apple Feedback for Apple Clang, the
   FreeBSD and MSYS2 trackers. The Perplexity MCP and `firecrawl search` find
   them quickly.
2. **An issue here** with a minimal reproducer (a few lines that compile on
   one toolchain and not the other), the exact compiler and image version,
   the run link, the job, and the upstream bug if one exists.
3. **Add it to `docs/platform-support/known-bugs.json`**: upstream link (or
   none yet), the repo issue, the affected toolchains and versions, and the
   test or snippet IDs whose skip reason names it.
4. **A new upstream report is posting outside this repo**: draft it in the
   repo issue and file it only after the owner says yes. When it's filed,
   add the link to the issue and to `known-bugs.json`.
5. When a later run shows the bug fixed in a newer version, narrow the
   version gate, and keep the entry with the fixed version.

## 5. Re-run only what failed

After the fix PRs merge (each with the owner's verdict), start **a new
dispatch** of only the workflows that had failures, on `dev`, every area.
Don't use `gh run rerun`: a GitHub re-run reuses the original commit and
wouldn't pick up the fixes.

```bash
gh workflow run all.yml -R homer6/cpp-docs --ref dev -f platforms=windows-msvc,macos -f areas= -f tests=true -f snippets=true
```

Check the `platforms` input's accepted values in `.github/workflows/all.yml`;
a single workflow file (`windows-msvc.yml`) can be dispatched directly too.
Process each new run from step 2. Repeat until every job of every workflow is
green.

## 6. Archive and regenerate

When the runs are in, regenerate from the latest consented run of **every**
platform workflow at once, plus the local results:

```bash
tools/collect_results.py <linux-id> <macos-id> <windows-msvc-id> <windows-gnu-id> <freebsd-id> <qemu-id> \
  --local docs/platform-support/local
tools/collect_results.py <ids…> --local docs/platform-support/local --check   # nothing stale
tools/collect_results.py --self-test
```

The collector archives each run under `docs/platform-support/runs/<run-id>/`
the first time and reads the archive after that, so the report can be rebuilt
after GitHub's retention runs out. It writes the entry page, every
per-toolchain file (removing files for toolchains no longer present) and the
README table. Check `tools/collect_results.py --help` for the current flags.

## 7. Local results

The local toolchains are part of the report. Before regenerating for a
release (or when the suite changed), refresh them into the archive:

```bash
tools/run_tests.sh --json docs/platform-support/local/results-gcc-15.json gcc-15
tools/run_tests.sh --json docs/platform-support/local/results-clang-22.json clang-22
tools/run_tests.sh --json docs/platform-support/local/results-gcc-16.json gcc-16
python3 tools/check_snippets.py docs --cxx g++-15 -q --json docs/platform-support/local/results-gcc-15-snippets.json
python3 tools/check_snippets.py docs --cxx /opt/homebrew/opt/llvm/bin/clang++ --flag=-stdlib=libc++ \
  --flag=-fexperimental-library -q --json docs/platform-support/local/results-clang-22-snippets.json
```

Follow the layout `collect_results.py` documents for `--local` if it differs.

## 8. The PR

One PR into `dev` with the regenerated entry page, the per-toolchain files,
the new `runs/` folders, `local/` and the README block. The body lists:

- the run links and each workflow's job counts (green / total);
- what changed since the last report: features that moved from ⏭ or ❌ to ✅,
  new skips, new bugs (with links);
- the fix PRs this report depends on;
- "README / CLAUDE.md": the README table is regenerated; CLAUDE.md changes
  only if the process changed.

Then comment on the release tracker (#140 or its successor) with the
per-workflow results.

## Keeping this skill current

When the process changes (a new workflow, a new output file, a new collector
flag, a new kind of finding), update this skill, the "Why we test on every
platform" and "Platform support page and badges" sections of `CLAUDE.md`,
and `docs/testing.md` in the same PR.
