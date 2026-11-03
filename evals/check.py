#!/usr/bin/env python3
"""Sanity-check the eval suite (not a model runner).

For every case in evals/cases/ (the cpp-guidelines suite):
  - the .json metadata has the required fields and a known outcome
  - every rule ID exists in modules/CppCoreGuidelines (the pinned edition)
  - the .cpp compiles with -fsyntax-only (set CXX to choose a compiler)

For every case in evals/modern-cpp/ (the modern-cpp suite):
  - the .json has the required fields, and every page it cites exists
  - the reference answer (.cpp) compiles with -fsyntax-only at the case's std

Usage: python3 evals/check.py            # from the repo root
       CXX=g++-15 python3 evals/check.py

Without CXX it picks the first of g++-16, g++-15, Homebrew clang++ (libc++),
g++, clang++ and c++ that compiles a C++20 probe using std::jthread: Apple's
default c++ ships a libc++ without it, which would fail cases for the wrong
reason.
"""
import json
import os
import shutil
import tempfile
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CASES = ROOT / "evals" / "cases"
GUIDELINES = ROOT / "modules" / "CppCoreGuidelines" / "CppCoreGuidelines.md"
OUTCOMES = {"violation", "compliant", "exception", "constraint", "needs-context"}
REQUIRED = {"id", "outcome", "rules", "context", "expect", "notes"}
MODERN = ROOT / "evals" / "modern-cpp"
MODERN_REQUIRED = {"id", "task", "context", "expect", "notes"}
MODERN_EXPECT = {"must_not_use", "must_use_any", "must_cite_any", "must_say"}


def known_rule_ids() -> set[str]:
    if not GUIDELINES.exists():
        sys.exit("modules/CppCoreGuidelines is missing: run `git submodule update --init`")
    text = GUIDELINES.read_text(encoding="utf-8")
    ids = set(re.findall(r'^### <a name="[^"]*"></a>([A-Za-z]+(?:\.[a-z]+)?\.[0-9]+):', text, re.M))
    ids |= set(re.findall(r'</a>((?:Type|Bounds|Lifetime)\.[0-9]+):', text))  # profile rules
    return ids


def compile_ok(cxx: str, cpp: Path, context: dict) -> str:
    cmd = [*cxx.split(), f"-std={context.get('std', 'c++20')}", "-fsyntax-only", str(cpp)]
    if context.get("exceptions") is False:
        cmd.insert(1, "-fno-exceptions")
    result = subprocess.run(cmd, capture_output=True, text=True)
    return "" if result.returncode == 0 else result.stderr.strip()


def check_modern(cxx: str) -> tuple[int, int]:
    cases = sorted(MODERN.glob("*.json")) if MODERN.exists() else []
    failures = 0
    for meta_path in cases:
        meta = json.loads(meta_path.read_text(encoding="utf-8"))
        problems = []
        missing = MODERN_REQUIRED - meta.keys()
        if missing:
            problems.append(f"missing fields {sorted(missing)}")
        if meta.get("id") != meta_path.stem:
            problems.append("id does not match file name")
        expect = meta.get("expect", {})
        if MODERN_EXPECT - expect.keys():
            problems.append(f"expect is missing {sorted(MODERN_EXPECT - expect.keys())}")
        if not meta.get("context", {}).get("compilers"):
            problems.append("context.compilers is empty")
        for page in expect.get("must_cite_any", []):
            if not (ROOT / page).exists():
                problems.append(f"cited page doesn't exist: {page}")
        if err := compile_ok(cxx, meta_path.with_suffix(".cpp"), meta.get("context", {})):
            problems.append("reference answer does not compile:\n" + err)
        print(f"{'ok  ' if not problems else 'FAIL'} modern-cpp/{meta_path.stem}")
        for p in problems:
            print(f"     {p}")
        failures += bool(problems)
    return len(cases), failures

CANDIDATES = ["g++-16", "g++-15", "/opt/homebrew/opt/llvm/bin/clang++ -stdlib=libc++", "g++", "clang++", "c++"]
PROBE = "#include <thread>\nint main() { std::jthread t([] {}); }\n"


def pick_compiler() -> str:
    if os.environ.get("CXX"):
        return os.environ["CXX"]
    with tempfile.TemporaryDirectory() as d:
        probe = Path(d) / "probe.cpp"
        probe.write_text(PROBE)
        for cand in CANDIDATES:
            exe, *flags = cand.split()
            if not shutil.which(exe):
                continue
            r = subprocess.run([exe, *flags, "-std=c++20", "-fsyntax-only", str(probe)], capture_output=True)
            if r.returncode == 0:
                return cand
    return "c++"


def main() -> int:
    cxx = pick_compiler()
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
        cmd = [*cxx.split(), f"-std={std}", "-fsyntax-only", str(cpp)]
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
    n, bad = check_modern(cxx)
    print(f"{n - bad}/{n} modern-cpp cases passed ({cxx})")
    return 1 if failures or bad else 0


if __name__ == "__main__":
    sys.exit(main())
