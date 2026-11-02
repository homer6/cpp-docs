# Testing and CI

> How the reference is checked: the snippet checker (does each block compile?),
> the GoogleTest suite (does the code do what the page says?), the toolchain
> presets, and the manual-only GitHub Actions platform workflows. The results
> per platform and per feature are in [`platform-support.md`](platform-support.md) (one page per toolchain in [`platform-support/`](platform-support/)) and in the
> [README's platform table](../README.md#tested-platforms).

**Verified locally first:** every release passes the full test suite and the
snippet checker on GCC 15 and Clang 22 (macOS, arm64) and GCC 16 (Linux,
Docker); that's the primary proof. The badges in the README are the GitHub
Actions platform workflows, which run **only by hand**, with the owner's
consent, before each release: each shows its last consented run on `main`, not
the current tip. They stay "no status" until the first consented run on `main`
(the runs before the next release are on `dev`). The full platform × compiler ×
architecture matrix, with counts, skip reasons, run links and dates, is in
[`platform-support.md`](platform-support.md).

## Why we test on every platform

The goal is to check **every language and library feature the reference
documents, and every example, on every supported platform, architecture and
compiler**, so the results show how far each toolchain, and the industry as
a whole, supports modern C++. That's why:

- the platform runs that are published use every test area and every
  snippet, never a sample;
- a feature a toolchain lacks shows up as a **skip with a reason** (a
  feature-test macro, a `needs=` tag, or a version gate naming the compiler
  bug or paper), never as a deleted test, so the skip reasons in
  [`platform-support.md`](platform-support.md) read as a support map;
- the workflows cover native Linux, macOS and Windows (MSVC, clang-cl,
  MinGW), FreeBSD VMs and QEMU for other architectures, and grow as new
  toolchains become available.

## Checking the snippets

Run the checker locally:

```bash
python3 tools/check_snippets.py docs/guidelines --cxx g++-15
python3 tools/check_snippets.py docs/examples --cxx g++-15 -q \
  --baseline tools/snippet_baseline/gcc.txt
python3 tools/check_snippets.py docs --cxx /opt/homebrew/opt/llvm/bin/clang++ \
  --flag=-stdlib=libc++ --flag=-fexperimental-library -q \
  --baseline tools/snippet_baseline/clang.txt
```

All three must pass cleanly. The GCC list of known failures
(`tools/snippet_baseline/gcc.txt`) is empty and stays that way; the Clang one
(`tools/snippet_baseline/clang.txt`) lists only libc++ gaps and only shrinks. Fence tags (`body`,
`nocompile`, `compile-fail`, `needs=…`) are explained in [`CLAUDE.md`](../CLAUDE.md).

Other toolchains: `--toolchain msvc` drives `cl /Zs` (syntax only, the prelude
via `/FI`) and `--toolchain clang-cl` drives `clang-cl /Zs`, both from a VS
Developer shell; each has its own baseline (`tools/snippet_baseline/msvc.txt`,
`clang-cl.txt`). Module blocks are checked with GCC and Clang only for now; with
the cl-style toolchains they skip with that reason. `--json FILE` also writes
the results (platform, compiler, counts, failures, skips with reasons) as JSON.

## Running the tests

The snippet checker proves the code **compiles**; the GoogleTest suite under
[`tests/`](../tests/) checks that it **does what the pages say** (printed output,
values in comments, thrown exceptions, type claims). Run it locally with CMake
3.25+ (presets) and Ninja:

```bash
tools/run_tests.sh              # gcc-15: Homebrew g++-15, libstdc++
tools/run_tests.sh clang-22     # Homebrew Clang 22 with libc++
tools/run_tests.sh gcc-16       # GCC 16 in the gcc:16 Docker image
tools/run_tests.sh gcc-15 -L containers   # one area (ctest arguments)
```

`tools/run_tests.sh <preset>` runs `cmake --workflow --preset <preset>`
(configure → build → test into `build/<preset>/`) and then lists each skipped
test with its reason, which ctest itself doesn't show. You can also run the
workflow directly: `cmake --workflow --preset gcc-15`.

- **gcc-15** — `CMAKE_CXX_COMPILER=g++-15`.
- **clang-22** — `/opt/homebrew/opt/llvm/bin/clang++ -stdlib=libc++`, linked
  with `-L/opt/homebrew/opt/llvm/lib/c++ -L/opt/homebrew/opt/llvm/lib/unwind
  -lunwind` plus matching `-rpath`s, so the binaries use Homebrew's libc++
  (whose headers they were compiled against) rather than the older system one
  in `/usr/lib`. It also sets `CMAKE_CXX_STDLIB_MODULES_JSON` to Homebrew's
  `libc++.modules.json`, which `clang++ -print-file-name` doesn't find, so
  CMake can build the `std` module.
- **gcc-16** — runs inside Docker. The `gcc:16` image (arm64-native, Debian
  trixie) has `g++` 16.2 and git but no CMake or Ninja, so the script installs
  them from Debian (CMake 3.31) before running the workflow:

  ```bash
  docker run --rm -v "$PWD":/w -w /w gcc:16 bash -c \
    'apt-get update -qq && apt-get install -y -qq --no-install-recommends cmake ninja-build && cmake --workflow --preset gcc-16'
  ```

  The script re-runs itself in Docker only for `gcc-16`, and not when it's
  already in a container (`/.dockerenv`, `/run/.containerenv`) or
  `CPPDOCS_NO_DOCKER=1` is set (a host whose `g++` is GCC 16).

### Portable presets (any machine, CI)

The presets above are the owner's Mac. These take no paths: the compiler comes
from `CXX` (or `-DCMAKE_CXX_COMPILER=…`), and without either they use the
driver name in the table from `PATH`. They build into `build/<preset>/` with
Ninja, and the configure step fails if the compiler isn't the preset's
toolchain (say AppleClang under `gcc`), so a job can't test the wrong one.

| Preset | Toolchain | Default `CXX` |
|---|---|---|
| `gcc` | GCC + libstdc++ (Linux, macOS, FreeBSD) | `g++` |
| `clang` | Clang + the platform's library (libstdc++ on Linux, libc++ on macOS/FreeBSD) | `clang++` |
| `clang-libcxx` | Clang + libc++ (`-stdlib=libc++` via `CMAKE_CXX_FLAGS_INIT`, so `CXXFLAGS`/`LDFLAGS` still apply) | `clang++` |
| `msvc` | MSVC `cl` + MS STL (Windows) | `cl` |
| `clang-cl` | `clang-cl` + MS STL (Windows) | `clang-cl` |
| `mingw` | MinGW-w64 GCC + libstdc++ (MSYS2) | `g++` |

```bash
CXX=g++-14 tools/run_tests.sh gcc
CXX=clang++-20 tools/run_tests.sh clang-libcxx
CXX=/opt/homebrew/opt/llvm/bin/clang++ \
  LDFLAGS="-L/opt/homebrew/opt/llvm/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++" \
  tools/run_tests.sh clang-libcxx      # Homebrew LLVM on macOS (import std skips:
                                       # configure first with -DCMAKE_CXX_STDLIB_MODULES_JSON=
                                       # /opt/homebrew/opt/llvm/lib/c++/libc++.modules.json)
tools/run_tests.sh --json build/results-gcc.json gcc   # also write JSON results
```

**MSVC and clang-cl** use Ninja too, so they must run where `cl`, `link` and
the Windows SDK are on `PATH`: a *Developer PowerShell / Command Prompt for
VS* (or `vcvarsall.bat x64`; in GitHub Actions, a step that sets up the MSVC
developer environment). From there, `tools/run_tests.ps1 msvc` (PowerShell) or
`tools/run_tests.sh msvc` (Git Bash started from that shell) runs the
workflow. The two presets show up in `cmake --list-presets` only on Windows.
The tests are built with `/std:c++latest /permissive- /EHsc /utf-8
/Zc:__cplusplus /bigobj /W4` (plus `/Zc:preprocessor` for `cl`), and
GoogleTest with `gtest_force_shared_crt` so both use the DLL runtime. `mingw`
runs from an MSYS2 MinGW shell (`tools/run_tests.sh mingw`).

`tools/ctest_to_json.py --build-dir build/<preset>` turns any ctest run
(`LastTest.log`, or a `ctest --output-junit` file with `--junit`) into JSON:
platform, compiler and version, preset, counts, failures and skips with their
reasons, and `results`: every test with its status (passed, failed, skipped).
`tools/check_snippets.py --json` writes the same shape for snippets, with
every block's ID and status in `results`. The text output and counts don't
change.

`tools/collect_results.py` turns those JSON files into
[`docs/platform-support.md`](platform-support.md): give it a GitHub
Actions run ID (it fetches the run's `results-*.json` artifacts with
`gh run download`) or a folder of results, plus `--local <dir>` for local
runs; `--stdout` prints the page instead. It also rewrites the platform
table in the [README](../README.md#tested-platforms) (the block between
`<!-- platforms:start -->` and `<!-- platforms:end -->`; `--no-readme` leaves
it alone, `--check` only reports whether either is stale, `--self-test` runs
its built-in samples). A file that doesn't match the schema fails with the
file and field named. The page and the table change only from runs the owner
consented to.

The report has an entry page and one page per toolchain:

```
docs/platform-support.md        entry page: how to read it, the platform table, the feature
                                support matrix summary (rows = areas, columns = compilers),
                                known gaps and the compiler/platform bugs found
docs/platform-support/
  <slug>.md                     one per platform × arch × compiler (generated), e.g.
                                linux-ubuntu-24.04-x86_64-gcc-13.md, local-macos-arm64-gcc-15.md
  runs/<run-id>/                archived consented runs: results-*.json and run.json
  local/                        archived local results: results-*.json and local.json
  known-bugs.json               compiler and platform bugs found (maintained by hand)
```

A toolchain page (built by `tools/feature_support.py`) has a header (OS or
image, compiler and version, standard library, architecture and byte order,
run link, date, commit, areas covered, whether its snippets are
report-only), totals, then per area every page, every feature (each
GoogleTest case, labelled by the `// "Section": claim` comment above its
`TEST`) and every example section: ✅ / ❌ / ⏭ with the skip reason / — not run
/ ❔ unknown. Rows come from the checkout; a job's cells come from its
`results` lists, or, for older results without them, are inferred only when
the job's counts match (else ❔). In the entry page's summary matrix, jobs
with the same compiler, version, standard library and identical results
share a column; its header links the toolchain page (or the list of them).
The known-gaps section ends with the bugs from
[`platform-support/known-bugs.json`](platform-support/known-bugs.json)
(toolchain, summary, upstream bug, repo issue, and the skip reason that
records it).

The results are archived in the repo, so the report can be rebuilt after
GitHub deletes the artifacts: the first time the collector sees a run ID it
saves each job's `results-*.json` and a `run.json` (URL, date, commit,
inputs, jobs and conclusions, a digest of the counts; no logs) under
[`platform-support/runs/<run-id>/`](platform-support/runs/) and reads the run
from there afterwards; `--local <folder>` copies the local results into
[`platform-support/local/`](platform-support/local/) (with the date and
commit), which later refreshes reuse. Regenerate everything with every
platform's latest consented run in one command:

```bash
tools/collect_results.py <linux-run> <macos-run> <windows-msvc-run> \
  <windows-gnu-run> <freebsd-run> <qemu-run> [--local <folder>]
```

(or one `all.yml` run ID), then commit the entry page, the toolchain pages,
the README and any new `runs/` or `local/` files. The collector removes
toolchain pages it no longer produces, and `--check` covers them all.

GoogleTest v1.18.0 is fetched at configure time with `FetchContent`. Tests
mirror the pages: `docs/examples/<area>/.../<page>.md` is checked by
`tests/<area>/.../<page>_test.cpp`, and each area folder builds into one
executable (`examples_containers_tests`, `examples_text_processing_tests`, …;
`examples_tests` builds them all). Helpers in [`tests/support/`](../tests/support/):
`capture.hpp` captures what `std::cout`, `printf` and `std::print` write, and
`feature.hpp` provides `CPPDOCS_REQUIRE(__cpp_lib_x)`, which skips a test with
the missing feature's name when a toolchain doesn't define the macro.

Named modules: [`tests/language/modules/modules.cmake`](../tests/language/modules/modules.cmake)
builds the module units in `tests/language/modules/units/` with
`FILE_SET CXX_MODULES` (CMake 3.28+ and Ninja) and scans the tests that import
them. `import std` is used where CMake provides it: CMake 4.3 and 4.4 with the
experimental gate the top-level `CMakeLists.txt` sets, or 4.5+ without one.
Elsewhere — the gcc-16 image's CMake 3.31, GCC 15 with CMake 4.5 on macOS —
those tests skip and say why.

## CI (manual)

GitHub Actions workflows here are **manual only**: `workflow_dispatch` is the
only trigger, nothing runs on push, pull request or a schedule, and a run
needs the owner's go-ahead. Every workflow is read-only (`permissions:
contents: read`) with actions pinned to commit SHAs;
`python3 tools/check_workflows.py` checks that. How to run, watch and read a
workflow is in [`.github/workflows/README.md`](../.github/workflows/README.md).
The platform workflows (`linux.yml`, `macos.yml`, `windows-msvc.yml`,
`windows-gnu.yml`, `freebsd.yml`, `qemu.yml`) build and run the suite and the
snippet checker with the portable presets on each platform, through the
scripts in [`tools/ci/`](../tools/ci/), and upload one
`results-<platform>-<compiler>.json` per job. A release needs a green
consented run of each on `dev` first, with every failure fixed (the release
checklist is in [`CLAUDE.md`](../CLAUDE.md)). The local checks above remain the
source of truth.
