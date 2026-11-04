# GitHub Actions workflows (manual only)

The owner's rule (#141): *"we are not going to have any github actions that
run automatically... manual invocation only"*.

Every workflow in this folder therefore:

- has **`workflow_dispatch` as its only trigger**. There are no `push`,
  `pull_request`, `schedule`, `workflow_run`, `release` or other event
  triggers, not even for a lint-only job. A platform workflow may also have
  `workflow_call` next to it, so the
  manually dispatched `all.yml` can run it; `workflow_call` alone is rejected.
- sets top-level **`permissions: contents: read`**, and no job grants write;
- **pins every action to a full 40-character commit SHA**, with the version in
  a comment, GitHub's own `actions/*` included:
  `uses: actions/checkout@<sha>  # v4.2.2`. Local `./` actions are fine.

`tools/check_workflows.py` enforces all three. Run it before every PR that
touches `.github/`, and in release checks:

```bash
python3 tools/check_workflows.py              # every .github/workflows/*.yml / *.yaml
python3 tools/check_workflows.py --self-test  # the checker's own good/bad samples
```

It exits 0 when every workflow passes (or there are none) and prints one line
per problem otherwise. It needs no third-party Python packages.

## Who starts a workflow

Only someone with the owner's explicit consent, given in the moment for that
run: which workflow, which ref, which inputs. Agents never start one on their
own. Start small (one workflow, one area, e.g. `-f areas='^strings$'`) to
prove the plumbing, then go wide. When to ask, the fix loop for failures and
how results feed the pages, baselines and `docs/platform-support.md` are in
`CLAUDE.md` ("GitHub Actions").

## Running a workflow

A workflow file has to exist on the repo's **default branch** before it can
be dispatched at all. Once it's there, you can run it against any branch
that has the file. The default is normally `main`; the owner has switched it
to `dev` for now (#140), because no workflow file is on `main` yet, and it
goes back to `main` before the next release PR (CLAUDE.md, "Release
checklist"). Meanwhile, runs happen on `dev`:

```bash
gh workflow list -R homer6/cpp-docs
gh workflow run <file>.yml -R homer6/cpp-docs --ref <branch> -f key=value
```

`-f key=value` sets one `workflow_dispatch` input; repeat it for more.

## Starting only some jobs

Every platform workflow and `all.yml` take a `jobs` input: a regular
expression searched for in each job's name (Python `re.search`, empty = every
job; #376). Only the jobs whose names match start. A matrix entry that
doesn't match isn't in the run at all, and a single job that doesn't match
shows as skipped, never as passed. Use it after a fix: start **only the jobs
that failed**, on the fixed commit, not every job of their workflows.
`gh run rerun` can't do this, because a re-run reuses the run's old commit.

The jobs are listed in `.github/ci/<workflow>.json`, one entry per job with
its display name and its matrix values. Each workflow's `plan` job runs
`tools/ci/plan.py`, which keeps the matching entries, and each job's matrix
is what it kept. Preview a pattern locally before asking to start the run:

```bash
tools/ci/plan.py                                    # every job of every workflow
tools/ci/plan.py --jobs 'FreeBSD .* gcc$'           # which jobs a pattern selects
tools/ci/plan.py linux.yml --jobs 'GCC 1[56] '      # one workflow
tools/ci/plan.py --check                            # the lists are readable, names unique
```

Then start the run with the same pattern:

```bash
gh workflow run linux.yml -R homer6/cpp-docs --ref dev -f jobs='^GCC 1[56] '
gh workflow run all.yml -R homer6/cpp-docs --ref dev -f platforms=windows-msvc,windows-gnu,freebsd -f jobs='MSVC|MSYS2|clang-cl|FreeBSD .* gcc$'
```

With `all.yml`, the pattern is matched in every selected platform: narrow it
with `platforms` too. Anchor the pattern when a name is a prefix of another:
`GCC 16 · ubuntu-24.04 \(gcc:16 image\)$` is only the plain job, not its
`ASan + UBSan` and `hardened` variants. A pattern that isn't a valid regular
expression fails the `plan` job, so a typo doesn't silently start nothing.
The plan job's summary lists the jobs it selected.

To add, rename or remove a job, edit the workflow and its job list together.
`tools/check_workflows.py` fails when they disagree: every job except `plan`
must be in the list, need `plan` and check its `<id>_on` output; a matrix
job's matrix is `${{ fromJSON(needs.plan.outputs.<id>) }}` and its name
`${{ matrix.name }}`; a single job's name is its one entry's name.

The report keeps each job from the newest run that ran it, so give
`tools/collect_results.py` both runs: `<full-run-id> <fix-up-run-id>`. The
fix-up run replaces only the jobs it ran.

## Watching and reading a run

```bash
gh run list -R homer6/cpp-docs --workflow <file>.yml   # find the run ID
gh run watch <run-id> -R homer6/cpp-docs                # follow it live
gh run view <run-id> -R homer6/cpp-docs                 # summary and jobs
gh run view <run-id> -R homer6/cpp-docs --log-failed    # logs of the failed steps
gh run download <run-id> -R homer6/cpp-docs -D <dir>    # artifacts it uploaded
```

A run's result is evidence to verify, like any other output. It doesn't
replace the local checks in `CLAUDE.md` ("Running the tests").

## Platform workflows

| File | Runners | Compilers |
|---|---|---|
| `linux.yml` | ubuntu-24.04, ubuntu-24.04-arm, ubuntu-26.04 (preview), ubuntu-26.04-arm (preview) | GCC 13–16 (official `gcc:N` images, by digest), Clang 18–23 (apt.llvm.org) with libstdc++ and libc++; GCC 15 and Clang 22 from Ubuntu 26.04; GCC 16 and Clang 22 (libc++) also sanitized and hardened (#263, #305); a time-boxed libFuzzer job for `fuzz/` (#307); a clang-tidy job over `tests/security/` and `fuzz/` (#308) |
| `macos.yml` | macos-15, macos-26 (arm64), macos-15-intel, macos-26-intel | Apple Clang (the runner's Xcode), Homebrew `gcc`, Homebrew `llvm` (its own libc++), also with ASan+UBSan and hardened (#305) |
| `windows-msvc.yml` | windows-2022 (VS 2022), windows-2025-vs2026, windows-11-arm (VS 2026) | MSVC `cl`, `msvc` preset, `run_tests.ps1`, `check_snippets.py --toolchain msvc`; a hardened MSVC job on windows-2025-vs2026 (#305) |
| `windows-gnu.yml` | windows-2025-vs2026 | clang-cl (VS's LLVM, `clang-cl` preset); MSYS2 UCRT64 GCC and Clang (libstdc++), CLANG64 Clang (libc++) |
| `freebsd.yml` | ubuntu-24.04 + a FreeBSD VM (`vmactions/freebsd-vm`) | 14.5 and 15.1 x86_64: base Clang (libc++) and pkg GCC |

**No emulated platforms for now** (the owner, #207): QEMU user-mode runs
(riscv64, s390x, ppc64le) and FreeBSD aarch64 under QEMU were removed. Every
job runs natively or in a KVM-accelerated VM.

Each takes four inputs, from `workflow_dispatch` or from `all.yml` through
`workflow_call`:

- `areas`: a ctest label regex (empty = all areas). Only the matching
  `examples_<area>_tests` targets are built (on Windows, `run_tests.ps1`
  builds all and runs the matching labels), and only their
  `docs/examples/<area>` folders are snippet-checked. Use it to prove
  plumbing or a fix cheaply; published runs use every area.
- `tests` and `snippets`: booleans, default true.
- `jobs`: a regex for the job names to start (empty = every job; #376). See
  [Starting only some jobs](#starting-only-some-jobs).

Every job runs `tools/ci/run.sh` (or `tools/ci/run.ps1` with a VS Developer
environment from `tools/ci/vsdevenv.ps1`), after a toolchain script
(`tools/ci/linux-toolchain.sh`, `tools/ci/macos-toolchain.sh`) sets `CXX`,
the preset, the snippet baseline and the labels. It uploads an artifact named
`results-<platform>-<compiler>` holding `results-<platform>-<compiler>.json`
(schema `cpp-docs/results/1`, kind `run`: `platform`, `arch`, `os_version`,
`byte_order`, `compiler`, `compiler_version`, `preset`, plus the whole
`tests` and `snippets` documents from `ctest_to_json.py` and
`check_snippets.py --json`) and its logs (`ci-out/`), and writes a step
summary: counts, failures and skips by reason. Matrices use
`fail-fast: false`, so one platform's failure doesn't cancel the others.
The same scripts run locally, e.g.

```bash
CPPDOCS_PLATFORM=local CPPDOCS_COMPILER=gcc-15 CPPDOCS_PRESET=gcc CXX=g++-15 \
  CPPDOCS_BASELINE=tools/snippet_baseline/gcc.txt tools/ci/run.sh
```
