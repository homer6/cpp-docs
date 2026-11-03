#!/usr/bin/env python3
"""Select which jobs a workflow run starts (#376).

Each platform workflow's jobs are listed in .github/ci/<workflow>.json, one
entry per job with its display name ("GCC 16 · ubuntu-24.04 (gcc:16 image)").
The workflow's plan job runs this script with the run's `jobs` input, a
regular expression searched for in each name (Python `re.search`; empty
selects every job), and every job's matrix is its selected entries. A job
with no selected entry doesn't start. So a fix-up run starts only the jobs
that failed, on the fixed commit, and nothing that already passed.

Preview a selection before asking to start a run:

    tools/ci/plan.py                                  # every job, per workflow
    tools/ci/plan.py --jobs 'Clang 2[23] libcxx'      # what this pattern selects
    tools/ci/plan.py linux.yml --jobs 'GCC 1[56]'     # one workflow

In a workflow (the plan job):

    tools/ci/plan.py linux.yml --jobs "$JOBS" --github-output

writes, per job id, `<id>` = the matrix ({"include": [...]}) and `<id>_on` =
true/false to $GITHUB_OUTPUT ("-" in an id becomes "_"), and the selection to
the step summary. It fails (exit 2) on a pattern that isn't a valid regular
expression, so a typo doesn't silently start nothing.

    tools/ci/plan.py --check       # every list: readable, names unique
    tools/ci/plan.py --self-test
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LISTS = ROOT / ".github" / "ci"


def load(path: Path) -> dict[str, list[dict]]:
    """A workflow's job list: {job id: [entry, ...]}, each entry with a "name"."""
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as e:
        raise ValueError(f"{path.name}: not readable JSON ({e})") from None
    jobs = doc.get("jobs") if isinstance(doc, dict) else None
    if not isinstance(jobs, dict) or not jobs:
        raise ValueError(f'{path.name}: needs a non-empty "jobs" map of job id -> entries')
    return jobs


def problems(path: Path) -> list[str]:
    """What's wrong with a job list, if anything."""
    try:
        jobs = load(path)
    except ValueError as e:
        return [str(e)]
    out, seen = [], set()
    for job, entries in jobs.items():
        if not re.fullmatch(r"[a-z0-9][a-z0-9-]*", job):
            out.append(f"{path.name}: job id {job!r} isn't a workflow job id")
        if not isinstance(entries, list) or not entries:
            out.append(f"{path.name}: {job}: needs a non-empty list of entries")
            continue
        for e in entries:
            name = e.get("name") if isinstance(e, dict) else None
            if not isinstance(name, str) or not name.strip():
                out.append(f"{path.name}: {job}: an entry has no name")
            elif name in seen:
                out.append(f"{path.name}: {job}: duplicate name {name!r}")
            else:
                seen.add(name)
    return out


def compile_pattern(pattern: str) -> re.Pattern | None:
    if not pattern.strip():
        return None
    try:
        return re.compile(pattern)
    except re.error as e:
        raise ValueError(f"jobs: {pattern!r} isn't a valid regular expression ({e})") from None


def select(jobs: dict[str, list[dict]], rx: re.Pattern | None) -> dict[str, list[dict]]:
    return {job: [e for e in entries if rx is None or rx.search(e["name"])]
            for job, entries in jobs.items()}


def github_output(chosen: dict[str, list[dict]], out, summary, workflow: str, pattern: str) -> None:
    for job, entries in chosen.items():
        key = job.replace("-", "_")
        out.write(f"{key}={json.dumps({'include': entries}, separators=(',', ':'), ensure_ascii=False)}\n")
        out.write(f"{key}_on={'true' if entries else 'false'}\n")
    if summary is not None:
        names = [e["name"] for entries in chosen.values() for e in entries]
        summary.write(f"### Jobs selected ({workflow})\n\n")
        summary.write(f"Pattern: `{pattern}`\n\n" if pattern.strip() else "Pattern: (every job)\n\n")
        summary.write("".join(f"- {n}\n" for n in names) if names else "None: no job name matches.\n")
        summary.write("\n")


def workflows(names: list[str]) -> list[Path]:
    if not names:
        return sorted(LISTS.glob("*.json"))
    paths = []
    for n in names:
        p = LISTS / (Path(n).stem + ".json")
        if not p.is_file():
            raise ValueError(f"{n}: no job list at {p.relative_to(ROOT)}")
        paths.append(p)
    return paths


def self_test() -> int:
    failures = []

    def expect(what: str, ok: bool) -> None:
        if not ok:
            failures.append(what)

    with tempfile.TemporaryDirectory() as tmp:
        good = Path(tmp, "w.json")
        good.write_text(json.dumps({"jobs": {
            "gcc": [{"name": "GCC 15 · x", "gcc": "15"}, {"name": "GCC 16 · x", "gcc": "16"}],
            "tidy": [{"name": "clang-tidy · x"}]}}))
        expect("good list passes", problems(good) == [])
        jobs = load(good)
        chosen = select(jobs, compile_pattern("GCC 16"))
        expect("pattern selects one matrix entry", [e["gcc"] for e in chosen["gcc"]] == ["16"])
        expect("unmatched single job is off", chosen["tidy"] == [])
        expect("empty pattern selects all", select(jobs, compile_pattern(" "))["tidy"] != [])
        out, summ = [], []

        class W:
            def __init__(self, sink): self.sink = sink
            def write(self, s): self.sink.append(s)
        github_output(chosen, W(out), W(summ), "w.yml", "GCC 16")
        text = "".join(out)
        expect("matrix output", 'gcc={"include":[{"name":"GCC 16 · x","gcc":"16"}]}' in text)
        expect("on/off outputs", "gcc_on=true" in text and "tidy_on=false" in text)
        expect("summary lists names", "- GCC 16 · x" in "".join(summ))
        try:
            compile_pattern("GCC (16")
            expect("bad regex rejected", False)
        except ValueError:
            pass
        dup = Path(tmp, "d.json")
        dup.write_text(json.dumps({"jobs": {"a": [{"name": "x"}], "b": [{"name": "x"}, {"gcc": "1"}]}}))
        msgs = problems(dup)
        expect("duplicate name reported", any("duplicate" in m for m in msgs))
        expect("nameless entry reported", any("no name" in m for m in msgs))
        Path(tmp, "e.json").write_text("{")
        expect("unreadable JSON reported", problems(Path(tmp, "e.json")) != [])
    for f in failures:
        print(f"self-test FAILED: {f}", file=sys.stderr)
    print("self-test: ok" if not failures else f"self-test: {len(failures)} failed")
    return 1 if failures else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("workflows", nargs="*", help="workflow files, e.g. linux.yml (default: all)")
    ap.add_argument("--jobs", default="", help="regular expression searched for in job names (empty: all)")
    ap.add_argument("--github-output", action="store_true",
                    help="write the matrices to $GITHUB_OUTPUT (one workflow)")
    ap.add_argument("--check", action="store_true", help="check every job list")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    try:
        paths = workflows(args.workflows)
        if args.check:
            msgs = [m for p in paths for m in problems(p)]
            for m in msgs:
                print(m)
            return 1 if msgs else 0
        rx = compile_pattern(args.jobs)
        if args.github_output:
            if len(paths) != 1:
                ap.error("--github-output needs exactly one workflow")
            chosen = select(load(paths[0]), rx)
            summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
            with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as out:
                if summary_path:
                    with open(summary_path, "a", encoding="utf-8") as summ:
                        github_output(chosen, out, summ, paths[0].stem + ".yml", args.jobs)
                else:
                    github_output(chosen, out, None, paths[0].stem + ".yml", args.jobs)
            return 0
        total = 0
        for p in paths:
            names = [e["name"] for entries in select(load(p), rx).values() for e in entries]
            total += len(names)
            if names or not args.jobs.strip():
                print(f"{p.stem}.yml ({len(names)})")
                print("".join(f"  {n}\n" for n in names), end="")
        print(f"{total} job(s) selected")
        return 0
    except (ValueError, KeyError) as e:
        print(f"plan.py: {e}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
