# Releases

Each release has a notes page here, a `vX.Y.Z` tag on its `dev` → `main`
merge commit, and a GitHub release. `CMakeLists.txt` holds the current
version; the `release_notes_exist` test fails if its notes page is missing.

Before a release (the full list is "Release checklist" in
[`CLAUDE.md`](../../CLAUDE.md)): the local runs pass, every platform
workflow has a green run on `dev`, started with the owner's consent, the
default branch is back on `main`, and the owner has approved the release.
The notes give the local counts and a per-platform table of the runs (run
links and counts, from `tools/collect_results.py --summary`).

| Version | Date | Notes |
|---|---|---|
| v0.2.1 | 2026-11-01 | [Skills release](v0.2.1.md): `make install`/`update`, both skills usable from any project, `/modern-cpp` as the entry point |
| v0.2.0 | 2026-11-01 | [Coverage release](v0.2.0.md): 7 new pages, C++23/26 gaps across 13 areas, constexpr and reflection, the `add-to-topic` and `modern-cpp` skills |
| v0.1.0 | 2026-10-31 | [First release](v0.1.0.md): 147 reference pages, 12 guidelines pages, test suite and snippet checker |
