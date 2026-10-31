#!/usr/bin/env python3
"""Sanity-check the eval suite (not a model runner).

For every case in evals/cases/:
  - the .json metadata has the required fields and a known outcome
  - every rule ID exists in modules/CppCoreGuidelines (the pinned edition)
  - the .cpp compiles with -fsyntax-only (set CXX to choose a compiler)

Usage: python3 evals/check.py            # from the repo root
       CXX=g++-15 python3 evals/check.py
"""
import json
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CASES = ROOT / "evals" / "cases"
GUIDELINES = ROOT / "modules" / "CppCoreGuidelines" / "CppCoreGuidelines.md"
OUTCOMES = {"violation", "compliant", "exception", "constraint", "needs-context"}
REQUIRED = {"id", "outcome", "rules", "context", "expect", "notes"}


def known_rule_ids() -> set[str]:
    if not GUIDELINES.exists():
        sys.exit("modules/CppCoreGuidelines is missing: run `git submodule update --init`")
    text = GUIDELINES.read_text(encoding="utf-8")
    ids = set(re.findall(r'^### <a name="[^"]*"></a>([A-Za-z]+(?:\.[a-z]+)?\.[0-9]+):', text, re.M))
    ids |= set(re.findall(r'</a>((?:Type|Bounds|Lifetime)\.[0-9]+):', text))  # profile rules
    return ids


def main() -> int:
    cxx = os.environ.get("CXX", "c++")
    ids = known_rule_ids()
    failures = 0
    cases = sorted(CASES.glob("*.json"))
    for meta_path in cases:
        meta = json.loads(meta_path.read_text(encoding="utf-8"))
        problems = []
        missing = REQUIRED - meta.keys()
        if missing:
            problems.append(f"missing fields {sorted(missing)}")
        if meta.get("outcome") not in OUTCOMES:
            problems.append(f"unknown outcome {meta.get('outcome')!r}")
        if meta.get("id") != meta_path.stem:
            problems.append("id does not match file name")
        cited = set(meta.get("rules", []))
        for key in ("must_cite_any", "must_not_flag", "must_not_cite_as_violation"):
            cited |= set(meta.get("expect", {}).get(key, []))
        unknown = sorted(r for r in cited if r not in ids)
        if unknown:
            problems.append(f"rule IDs not in the Guidelines: {unknown}")

        cpp = meta_path.with_suffix(".cpp")
        std = meta.get("context", {}).get("std", "c++20")
        cmd = [cxx, f"-std={std}", "-fsyntax-only", str(cpp)]
        if meta.get("context", {}).get("exceptions") is False:
            cmd.insert(1, "-fno-exceptions")
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            problems.append("does not compile:\n" + result.stderr.strip())

        status = "ok  " if not problems else "FAIL"
        print(f"{status} {meta_path.stem}")
        for p in problems:
            print(f"     {p}")
        failures += bool(problems)

    print(f"\n{len(cases) - failures}/{len(cases)} cases passed ({cxx})")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
