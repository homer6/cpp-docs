#!/usr/bin/env python3
"""Helpers for the platform workflows in .github/workflows/ (#143-#149).

tools/ci/run.sh and tools/ci/run.ps1 call it; it runs anywhere Python 3.8+
does (Linux, macOS, FreeBSD, Windows, MSYS2, QEMU containers).

    cppdocs_ci.py areas --regex 'containers|ranges' --targets
        CMake targets for the test areas whose name matches the ctest label
        regex (the workflows' `areas` input), one per line
    cppdocs_ci.py areas --regex 'containers|ranges' --paths
        the docs/examples/<area> folders for the same areas (for the snippet
        checker), one per line
    cppdocs_ci.py collect --platform linux-x64-ubuntu-24.04 --compiler gcc-14 \
        --preset gcc --tests ci-out/tests.json --snippets ci-out/snippets.json \
        -o results-linux-x64-ubuntu-24.04-gcc-14.json --summary ci-out/summary.md
        merge the tests JSON (tools/ctest_to_json.py) and the snippets JSON
        (tools/check_snippets.py --json) into one results file and write a
        Markdown summary for $GITHUB_STEP_SUMMARY

The results file (schema "cpp-docs/results/1", kind "run") keeps both
documents whole under "tests" and "snippets" (null when that step was off or
produced nothing) and adds the fields the support matrix (#150) reads:

    {"schema": "cpp-docs/results/1", "kind": "run",
     "platform": "linux-x64-ubuntu-24.04", "arch": "x86_64", "os_version": "...",
     "byte_order": "little", "compiler": "gcc-14", "compiler_version": "14.2.0",
     "preset": "gcc", "inputs": {"areas": "", "tests": true, "snippets": true},
     "steps": {"configure": 0, "build": 0, "tests": 0, "snippets": 0},
     "ok": true, "run": {"workflow": "...", "run_id": "...", "url": "..."},
     "tests": {...}, "snippets": {...}, "generated": "..."}
"""
from __future__ import annotations

import argparse
import collections
import datetime as dt
import json
import os
import pathlib
import platform
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
SCHEMA = "cpp-docs/results/1"


# --- areas ------------------------------------------------------------------

def test_areas() -> list[str]:
    """Area folders under tests/ that CMake turns into examples_<area>_tests."""
    return sorted(p.name for p in (ROOT / "tests").iterdir()
                  if p.is_dir() and p.name != "support" and any(p.rglob("*_test.cpp")))


def cmd_areas(args) -> int:
    rx = re.compile(args.regex)  # ctest -L semantics: a search, not a full match
    picked = [a for a in test_areas() if rx.search(a)]
    if args.targets:
        out = [f"examples_{a.replace('-', '_')}_tests" for a in picked]
        if rx.search("support"):
            out.append("cppdocs_support_tests")
    else:
        out = [f"docs/examples/{a}" for a in picked if (ROOT / "docs" / "examples" / a).is_dir()]
    if not out:
        print(f"cppdocs_ci.py: no area matches {args.regex!r}; areas: {', '.join(test_areas())}",
              file=sys.stderr)
        return 1
    print("\n".join(out))
    return 0


# --- collect ----------------------------------------------------------------

def load(path: str | None):
    if not path:
        return None
    p = pathlib.Path(path)
    if not p.is_file():
        return None
    try:
        return json.loads(p.read_text(encoding="utf-8"))
    except (OSError, ValueError) as e:
        print(f"cppdocs_ci.py: can't read {p}: {e}", file=sys.stderr)
        return None


def first_line(s: str) -> str:
    return (s or "").strip().splitlines()[0] if (s or "").strip() else ""


def bool_arg(s: str) -> bool:
    return str(s).strip().lower() in ("1", "true", "yes", "on")


def md_cell(s) -> str:
    return str(s).replace("|", "\\|").replace("\n", " ")


def reason_counts(skips: list[dict], key: str) -> list[tuple[str, int]]:
    c = collections.Counter(md_cell(s.get(key) or "(no reason given)") for s in skips)
    return sorted(c.items(), key=lambda kv: (-kv[1], kv[0]))


def summary(doc: dict) -> str:
    t, s = doc["tests"], doc["snippets"]
    icon = "✅" if doc["ok"] else "❌"
    lines = [
        f"## {icon} {doc['platform']} · {doc['compiler']}",
        "",
        "| | |",
        "|---|---|",
        f"| Platform | `{doc['platform']}` ({doc['os_version']}) |",
        f"| Architecture | `{doc['arch']}` |",
        f"| Byte order | **{doc['byte_order']}-endian** |",
        f"| Compiler | `{doc['compiler']}` {doc['compiler_version']} |",
        f"| Preset | `{doc['preset']}` |",
        f"| Areas | {('`' + doc['inputs']['areas'] + '`') if doc['inputs']['areas'] else 'all'} |",
        f"| Step exit codes | {', '.join(f'{k} {v}' for k, v in doc['steps'].items())} |",
        "",
    ]
    lines += ["### Tests", ""]
    if not doc["inputs"]["tests"]:
        lines += ["Not run (`tests` input is false).", ""]
    elif t is None:
        lines += ["No results: the configure or build step failed; see the logs artifact.", ""]
    else:
        c = t["counts"]
        lines += ["| Total | Passed | Failed | Skipped |", "|---:|---:|---:|---:|",
                  f"| {c['total']} | {c['passed']} | {c['failed']} | {c['skipped']} |", ""]
        if t["failures"]:
            lines += [f"<details><summary>{len(t['failures'])} failing tests</summary>", ""]
            lines += [f"- `{f['name']}`: {md_cell(first_line(f['message']))[:200]}" for f in t["failures"][:100]]
            lines += ["", "</details>", ""]
        if t["skips"]:
            lines += ["| Skip reason | Tests |", "|---|---:|"]
            lines += [f"| {r[:200]} | {n} |" for r, n in reason_counts(t["skips"], "reason")]
            lines += [""]
    lines += ["### Snippets", ""]
    if not doc["inputs"]["snippets"]:
        lines += ["Not run (`snippets` input is false).", ""]
    elif s is None:
        lines += ["No results: the snippet checker didn't write JSON; see the logs artifact.", ""]
    else:
        if doc.get("snippets_mode") == "report-only":
            lines += ["**Report-only:** this toolchain has no reviewed baseline yet, so snippet "
                      "failures are listed but don't fail the job (#159).", ""]
        c = s["counts"]
        b = s.get("baseline")
        lines += ["| Blocks | Checked | Failed | Skipped | Baseline |", "|---:|---:|---:|---:|---|",
                  f"| {c['total']} | {c['checked']} | {c['failed']} | {c['skipped']} | "
                  + (f"`{b['file']}`: {b['known_failing']} known, {b['new']} new, "
                     f"{len(b['fixed'])} fixed, {len(b['stale'])} stale" if b else "none") + " |", ""]
        if s["failures"]:
            lines += [f"<details><summary>{len(s['failures'])} failing blocks</summary>", ""]
            lines += [f"- `{f['id']}`{' (known)' if f.get('known') else ''}: "
                      f"{md_cell(first_line(f['message']))[:200]}" for f in s["failures"][:200]]
            lines += ["", "</details>", ""]
        if s["skips"]:
            lines += ["| Skip reason | Blocks |", "|---|---:|"]
            lines += [f"| {r[:200]} | {n} |" for r, n in reason_counts(s["skips"], "reason")]
            lines += [""]
    return "\n".join(lines) + "\n"


# Snippet runs are enforced only where a reviewed baseline matches the
# toolchain: GCC 15/16 (snippet_baseline/gcc.txt) and Clang/LLVM 22 with libc++
# (clang.txt), off Windows. Everything else is report-only until a baseline is
# seeded from its first consented run and reviewed (#159).
ENFORCED_SNIPPETS = re.compile(r"^(gcc-1[56]|clang-22-libcxx|llvm-22)$")


def snippet_mode(compiler: str, platform_label: str, baseline: str) -> str:
    override = os.environ.get("CPPDOCS_SNIPPETS_MODE", "").strip()
    if override in ("enforced", "report-only"):
        return override
    if baseline and not platform_label.startswith("windows") and ENFORCED_SNIPPETS.match(compiler):
        return "enforced"
    return "report-only"


def cmd_snippet_mode(args) -> int:
    print(snippet_mode(args.compiler, args.platform, args.baseline))
    return 0


def cmd_collect(args) -> int:
    tests, snippets = load(args.tests), load(args.snippets)
    steps = {}
    for item in args.step:
        k, _, v = item.partition("=")
        steps[k] = int(v) if v.lstrip("-").isdigit() else v
    # Compiler version: the one CMake detected, else the snippet checker's
    # `--version` line.
    version = ""
    if tests and tests.get("compiler", {}).get("version"):
        version = tests["compiler"]["version"]
    elif snippets:
        version = first_line(snippets.get("compiler", {}).get("version", ""))
    plat = (tests or snippets or {}).get("platform") or {}
    run_tests, run_snippets = bool_arg(args.run_tests), bool_arg(args.run_snippets)
    ok = all(v == 0 for v in steps.values() if isinstance(v, int))
    if run_tests and (tests is None or tests["counts"]["failed"]):
        ok = False
    mode = args.snippets_mode
    if run_snippets and (snippets is None or (mode == "enforced" and not snippets.get("ok"))):
        ok = False
    server = os.environ.get("GITHUB_SERVER_URL", "")
    repo, run_id = os.environ.get("GITHUB_REPOSITORY", ""), os.environ.get("GITHUB_RUN_ID", "")
    doc = {
        "schema": SCHEMA,
        "kind": "run",
        "platform": args.platform,
        "arch": plat.get("arch") or platform.machine(),
        "os_version": args.os_version or " ".join(
            x for x in (plat.get("system") or platform.system(), plat.get("version") or platform.release()) if x),
        "byte_order": sys.byteorder,
        "compiler": args.compiler,
        "compiler_version": version,
        "preset": args.preset,
        "inputs": {"areas": args.areas, "tests": run_tests, "snippets": run_snippets},
        "snippets_mode": mode,
        "steps": steps,
        "ok": ok,
        "run": {"workflow": os.environ.get("GITHUB_WORKFLOW", ""), "job": os.environ.get("GITHUB_JOB", ""),
                "run_id": run_id, "sha": os.environ.get("GITHUB_SHA", ""),
                "url": f"{server}/{repo}/actions/runs/{run_id}" if server and repo and run_id else ""},
        "tests": tests,
        "snippets": snippets,
        "generated": dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    }
    out = pathlib.Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    md = summary(doc)
    if args.summary:
        p = pathlib.Path(args.summary)
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(md, encoding="utf-8")
    print(f"wrote {out}: ok={ok}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    a = sub.add_parser("areas", help="targets or docs folders for an areas regex")
    a.add_argument("--regex", required=True)
    g = a.add_mutually_exclusive_group(required=True)
    g.add_argument("--targets", action="store_true")
    g.add_argument("--paths", action="store_true")
    a.set_defaults(func=cmd_areas)
    c = sub.add_parser("collect", help="merge results and write the summary")
    c.add_argument("--platform", required=True)
    c.add_argument("--compiler", required=True)
    c.add_argument("--preset", required=True)
    c.add_argument("--os-version", default="")
    c.add_argument("--areas", default="")
    c.add_argument("--run-tests", default="true")
    c.add_argument("--run-snippets", default="true")
    c.add_argument("--tests")
    c.add_argument("--snippets")
    c.add_argument("--step", action="append", default=[], help="NAME=exit-code (repeatable)")
    c.add_argument("-o", "--output", required=True)
    c.add_argument("--summary")
    c.add_argument("--snippets-mode", choices=["enforced", "report-only"], default="enforced")
    c.set_defaults(func=cmd_collect)
    m = sub.add_parser("snippet-mode", help="print enforced or report-only for this toolchain (#159)")
    m.add_argument("--compiler", required=True)
    m.add_argument("--platform", required=True)
    m.add_argument("--baseline", default="")
    m.set_defaults(func=cmd_snippet_mode)
    args = ap.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
