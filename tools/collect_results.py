#!/usr/bin/env python3
"""Build docs/platform-support.md from test and snippet results.

Reads the JSON results that `tools/run_tests.sh --json` (via
tools/ctest_to_json.py, kind "tests") and `tools/check_snippets.py --json`
(kind "snippets") write, schema "cpp-docs/results/1", and renders a
platform x compiler x arch table: passed, failed and skipped counts, the top
skip reasons, the run link and date, plus the known gaps per toolchain (the
snippet baselines, the needs= features each toolchain skipped).

Sources (positional) are GitHub Actions results; each is either

  * a run ID: the artifacts are fetched with `gh run download <id>` and the
    run's URL, date, workflow and jobs with `gh api`; or
  * a local folder (searched recursively) of results-*.json files, e.g. an
    unpacked `gh run download`.

`--local DIR` adds results from local runs (the owner's machine, Docker) in
their own table, labelled as local. Never start a workflow to refresh this
page: it's built from runs the owner already consented to.

    tools/collect_results.py 1234567890 --local build/results
    tools/collect_results.py --local build/results --stdout
    tools/collect_results.py path/to/downloaded-run --run-url URL --date 2026-11-01

File names: each CI job uploads results-<platform>-<compiler>.json. A job (or
a local run) that writes both kinds may suffix them, results-<label>-tests.json
and results-<label>-snippets.json; both land on the same row. A file may also
hold a JSON list of result documents. The row label is the file name without
"results-" and that suffix.

Exit status: 0 when the page was written, 1 when a result file doesn't match
the schema (the message names the file and the field), 2 on a usage error or
when there are no results at all.
"""
from __future__ import annotations

import argparse
import collections
import datetime as dt
import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
PAGE = ROOT / "docs" / "platform-support.md"
BASELINES = ROOT / "tools" / "snippet_baseline"
REPO = "homer6/cpp-docs"
SCHEMA = "cpp-docs/results/1"
TOP_SKIPS = 3

# The platform workflows (the shared contract in #140), in badge order.
WORKFLOWS = [
    ("linux.yml", "Linux (x86_64 and arm64)",
     "ubuntu-24.04/26.04 x64 and arm: GCC 13-16, Clang 18-22 with libstdc++ and libc++"),
    ("macos.yml", "macOS",
     "macos-15/26 arm64, macos-15-intel/26-intel: Apple Clang, Homebrew GCC and LLVM"),
    ("windows-msvc.yml", "Windows (MSVC)",
     "windows-2025, windows-2025-vs2026, windows-11-arm, windows-11-vs2026-arm: MSVC"),
    ("windows-gnu.yml", "Windows (clang-cl, MinGW)",
     "windows-2025: clang-cl, MSYS2 UCRT64 GCC and Clang"),
    ("freebsd.yml", "FreeBSD",
     "FreeBSD 14.x/15.x VM, x86_64 (and aarch64 if supported): base Clang/libc++, pkg GCC"),
    ("qemu.yml", "Other architectures (QEMU)", "riscv64, s390x (big-endian), ppc64le under QEMU"),
]
NOT_RUN = "not run yet: first run after the next release, with the owner's consent"

# Fields every result document must have, by kind: (path, type).
COMMON = [("schema", str), ("kind", str), ("platform", dict), ("platform.system", str),
          ("platform.version", str), ("platform.arch", str), ("compiler", dict),
          ("compiler.id", str), ("compiler.version", str), ("counts", dict),
          ("counts.total", int), ("counts.failed", int), ("counts.skipped", int),
          ("failures", list), ("skips", list), ("generated", str)]
BY_KIND = {
    "tests": [("preset", str), ("counts.passed", int)],
    "snippets": [("toolchain", str), ("counts.checked", int), ("baseline", (dict, type(None)))],
}


class SchemaError(Exception):
    pass


def field(doc: dict, path: str):
    cur = doc
    for part in path.split("."):
        if not isinstance(cur, dict) or part not in cur:
            raise KeyError(path)
        cur = cur[part]
    return cur


def validate(doc, where: str) -> dict:
    if not isinstance(doc, dict):
        raise SchemaError(f"{where}: expected a JSON object, found {type(doc).__name__}")
    if doc.get("schema") != SCHEMA:
        raise SchemaError(f"{where}: schema is {doc.get('schema')!r}, expected {SCHEMA!r} "
                          "(written by tools/ctest_to_json.py or tools/check_snippets.py --json)")
    kind = doc.get("kind")
    if kind not in BY_KIND:
        raise SchemaError(f"{where}: kind is {kind!r}, expected one of {sorted(BY_KIND)}")
    for path, typ in COMMON + BY_KIND[kind]:
        try:
            val = field(doc, path)
        except KeyError:
            raise SchemaError(f"{where}: missing field {path!r} (kind {kind!r})") from None
        if not isinstance(val, typ) or (typ is int and isinstance(val, bool)):
            raise SchemaError(f"{where}: field {path!r} is {type(val).__name__}, expected "
                              f"{getattr(typ, '__name__', 'object or null')}")
    for i, s in enumerate(doc["skips"]):
        if not isinstance(s, dict) or not isinstance(s.get("reason"), str):
            raise SchemaError(f"{where}: skips[{i}] has no string 'reason'")
    return doc


# --- loading ---------------------------------------------------------------

def label_of(path: pathlib.Path) -> str:
    stem = path.stem
    stem = stem[len("results-"):] if stem.startswith("results-") else stem
    return re.sub(r"-(tests|snippets)$", "", stem)


def load_dir(folder: pathlib.Path) -> dict[str, dict[str, dict]]:
    """{label: {kind: doc}} for every results-*.json under folder."""
    rows: dict[str, dict[str, dict]] = {}
    files = sorted(folder.rglob("results-*.json"))
    for f in files:
        try:
            data = json.loads(f.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as e:
            raise SchemaError(f"{f}: not readable JSON ({e})") from None
        docs = data if isinstance(data, list) else [data]
        for i, d in enumerate(docs):
            doc = validate(d, f"{f}" + (f"[{i}]" if isinstance(data, list) else ""))
            row = rows.setdefault(label_of(f), {})
            if doc["kind"] in row:
                raise SchemaError(f"{f}: a second {doc['kind']!r} result for row {label_of(f)!r}; "
                                  "give each job's file a unique name")
            row[doc["kind"]] = doc
    return rows


def gh_json(args: list[str]):
    r = subprocess.run(["gh", *args], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"collect_results.py: gh {' '.join(args)} failed: {r.stderr.strip()}")
    return json.loads(r.stdout)


def load_run(run_id: str, keep: pathlib.Path | None) -> dict:
    """Download a run's artifacts and metadata (never starts or re-runs anything)."""
    meta = gh_json(["api", f"repos/{REPO}/actions/runs/{run_id}"])
    jobs = gh_json(["api", "--paginate", "--slurp", f"repos/{REPO}/actions/runs/{run_id}/jobs?per_page=100"])
    jobs = [j for page in jobs for j in page.get("jobs", [])]
    dest = keep or pathlib.Path(tempfile.mkdtemp(prefix=f"run-{run_id}-"))
    dest.mkdir(parents=True, exist_ok=True)
    r = subprocess.run(["gh", "run", "download", run_id, "-R", REPO, "-D", str(dest),
                        "--pattern", "results-*"], capture_output=True, text=True)
    if r.returncode and "no valid artifacts" not in r.stderr:
        sys.exit(f"collect_results.py: gh run download {run_id} failed: {r.stderr.strip()}")
    return {"rows": load_dir(dest), "url": meta.get("html_url", ""),
            "date": (meta.get("run_started_at") or meta.get("created_at") or "")[:10],
            "workflow": pathlib.PurePosixPath(meta.get("path", "")).name,
            "branch": meta.get("head_branch", ""), "sha": (meta.get("head_sha") or "")[:7],
            "conclusion": meta.get("conclusion") or meta.get("status", ""),
            "jobs": [(j.get("name", ""), j.get("conclusion") or j.get("status", "")) for j in jobs]}


def load_folder(folder: pathlib.Path, url: str, date: str) -> dict:
    rows = load_dir(folder)
    if not date:
        stamps = [d["generated"] for r in rows.values() for d in r.values()]
        date = max(stamps)[:10] if stamps else ""
    return {"rows": rows, "url": url, "date": date, "workflow": "", "branch": "", "sha": "",
            "conclusion": "", "jobs": []}


# --- rendering -------------------------------------------------------------

def family(doc: dict) -> str:
    c = doc["compiler"]
    cid = c["id"].lower()
    if doc["kind"] == "snippets":
        if cid == "clang" and "apple" in c["version"].lower():
            return "Apple Clang"
        return {"gcc": "GCC", "clang": "Clang", "msvc": "MSVC", "clang-cl": "clang-cl"}.get(cid, c["id"])
    if c.get("frontend") == "MSVC" and cid == "clang":
        return "clang-cl"
    return {"gnu": "GCC", "clang": "Clang", "appleclang": "Apple Clang", "msvc": "MSVC"}.get(cid, c["id"])


def version_of(doc: dict) -> str:
    v = doc["compiler"]["version"]
    if doc["kind"] == "tests":
        return v
    m = re.search(r"\b(\d+\.\d+(?:\.\d+)*)\b", v.split(")")[-1]) or re.search(r"\b(\d+\.\d+(?:\.\d+)*)", v)
    return m[1] if m else v


def os_of(doc: dict) -> str:
    p = doc["platform"]
    name = {"Darwin": "macOS (Darwin {})", "Linux": "Linux {}", "Windows": "Windows {}",
            "FreeBSD": "FreeBSD {}"}.get(p["system"], p["system"] + " {}")
    return name.format(p["version"]).strip()


def arch_of(doc: dict) -> str:
    a = doc["platform"]["arch"]
    return {"aarch64": "arm64", "ARM64": "arm64", "AMD64": "x86_64", "amd64": "x86_64"}.get(a, a)


def short_reason(reason: str, limit: int = 60) -> str:
    reason = re.sub(r"\s+", " ", reason).strip().replace("|", "\\|")
    return reason if len(reason) <= limit else reason[:limit - 1].rstrip() + "…"


def top_skips(row: dict[str, dict]) -> str:
    # nocompile snippets skip on every toolchain alike, so they aren't a toolchain's gap.
    counts = collections.Counter(short_reason(s["reason"]) for d in row.values() for s in d["skips"]
                                 if s["reason"] != "nocompile")
    return "<br>".join(f"{n}× {r}" for r, n in counts.most_common(TOP_SKIPS)) or "-"


def tests_cell(doc: dict | None) -> str:
    if not doc:
        return "-"
    c = doc["counts"]
    return f"{c['passed']} / {c['failed']} / {c['skipped']}"


def snippets_cell(doc: dict | None) -> str:
    if not doc:
        return "-"
    c = doc["counts"]
    known = (doc["baseline"] or {}).get("known_failing", 0)
    cell = f"{c['checked'] - c['failed']} / {c['failed']} / {c['skipped']}"
    return cell + (f" ({known} in baseline)" if known else "")


def describe(row: dict[str, dict]) -> tuple[str, str, str, str]:
    """(OS, arch, compiler, preset/toolchain) for a row, preferring the tests document."""
    main = row.get("tests") or row["snippets"]
    comp = f"{family(main)} {version_of(main)}"
    stdlib = main["compiler"].get("stdlib") or ""
    if stdlib and stdlib != "unknown":
        comp += f", {stdlib}"
    preset = row["tests"]["preset"] if "tests" in row else row["snippets"]["toolchain"]
    return os_of(main), arch_of(main), comp, f"`{preset}`"


def matrix(rows: dict[str, dict[str, dict]], link: str) -> list[str]:
    out = ["| Platform | OS | Arch | Compiler | Preset | Tests passed / failed / skipped "
           "| Snippets passed / failed / skipped | Top skip reasons | Run |",
           "|---|---|---|---|---|---|---|---|---|"]
    for label in sorted(rows):
        row = rows[label]
        os_, arch, comp, preset = describe(row)
        out.append(f"| {label} | {os_} | {arch} | {comp} | {preset} | {tests_cell(row.get('tests'))} "
                   f"| {snippets_cell(row.get('snippets'))} | {top_skips(row)} | {link} |")
    return out


def baseline_entries(name: str) -> list[str]:
    f = BASELINES / f"{name}.txt"
    if not f.is_file():
        return []
    return [l.strip() for l in f.read_text(encoding="utf-8").splitlines() if l.strip() and not l.startswith("#")]


def gated_features(doc: dict) -> collections.Counter:
    c: collections.Counter = collections.Counter()
    for s in doc["skips"]:
        if s["reason"].startswith("needs ") and not s["reason"].startswith("needs:"):
            for feat in s["reason"][6:].split(", "):
                c[feat] += 1
    return c


def gaps(all_rows: list[tuple[str, dict[str, dict]]]) -> list[str]:
    out = ["## Known gaps per toolchain", "",
           "Snippet baselines (`tools/snippet_baseline/`) list blocks known to fail on a toolchain; "
           "they only shrink, and the GCC one stays empty. A `needs=<feature>` block is skipped unless "
           "the run enables that feature (`--enable <feature>`); a test whose feature-test macro is "
           "missing skips with the macro's name (see the skip reasons above).", ""]
    seen: dict[str, dict] = {}
    for _, row in all_rows:
        doc = row.get("snippets")
        if doc:
            seen.setdefault(f"{family(doc)} {version_of(doc).split('.')[0]}", doc)  # first run wins
    feats = {k: gated_features(d) for k, d in seen.items()}
    common = set.intersection(*(set(f) for f in feats.values())) if feats else set()
    out += ["| Toolchain | Baseline | Baselined blocks | `needs=` enabled | `needs=` blocks skipped "
            "| Skipped here but not on every toolchain |", "|---|---|---|---|---|---|"]
    for key, doc in seen.items():
        tc = doc["toolchain"]
        extra = ", ".join(f"`{f}` ({n})" for f, n in sorted(feats[key].items()) if f not in common) or "-"
        enabled = ", ".join(f"`{e}`" for e in doc["enabled"]) or "-"
        out.append(f"| {key} | [`{tc}.txt`](../tools/snippet_baseline/{tc}.txt) | "
                   f"{len(baseline_entries(tc))} | {enabled} | {sum(feats[key].values())} | {extra} |")
    for tc in ("gcc", "clang", "msvc", "clang-cl"):
        if not any(d["toolchain"] == tc for d in seen.values()):
            out.append(f"| {tc} (no snippet run in these results) | "
                       f"[`{tc}.txt`](../tools/snippet_baseline/{tc}.txt) | {len(baseline_entries(tc))} "
                       "| - | - | - |")
    if common:
        out += ["", "`needs=` features skipped on every toolchain above (none of these runs enabled "
                "them; they wait for library or compiler support): " + ", ".join(f"`{f}`" for f in sorted(common)) + "."]
    listed = [(tc, baseline_entries(tc)) for tc in ("gcc", "clang", "msvc", "clang-cl")]
    if any(e for _, e in listed):
        out += ["", "<details><summary>Baselined blocks</summary>", ""]
        for tc, entries in listed:
            if entries:
                out += [f"**{tc}** ({len(entries)}):", ""] + [f"- `{e}`" for e in entries] + [""]
        out.append("</details>")
    return out


def render(runs: list[dict], local: dict | None, today: str) -> str:
    consented = [r for r in runs if r["rows"] or r["jobs"]]
    # Which platform workflows each run covered: the run's own workflow file, or for all.yml
    # the caller jobs named after the files ("linux.yml / <job>"), skipped ones left out.
    known = {w for w, _, _ in WORKFLOWS}
    ran: dict[str, tuple[str, str, str]] = {}
    for r in consented:
        names = {r["workflow"]} & known
        for job, concl in r["jobs"]:
            head = job.split(" / ")[0].strip()
            if head in known and concl != "skipped":
                names.add(head)
        for w in names:
            ran[w] = (r["date"], r["url"], r["conclusion"])
    last = max([r["date"] for r in consented if r["rows"] and r["date"]] + [d for d, _, _ in ran.values() if d],
               default="")
    out = ["# Platform support", "",
           "<!-- Generated by tools/collect_results.py; don't edit by hand. "
           "Refresh it only from runs the owner consented to (see CLAUDE.md). -->", "",
           "> Which platforms, compilers and architectures the test suite and the snippet checker "
           "have run on, with passed, failed and skipped counts.", "",
           "## How to read this", "",
           "- **Workflows run only by hand.** Every GitHub Actions workflow here is `workflow_dispatch` "
           "only, and runs only with the owner's consent, at a release. The badges in the README and "
           "the tables below show the **last consented run**, not the current tip of any branch.",
           f"- **Last consented run:** {last or 'none yet'}.",
           "- **Local runs are the primary proof.** The owner's machine (GCC 15, Clang 22) and the "
           "GCC 16 Docker image are checked for every release; their results are labelled *local*.",
           "- **Skipped** means a test or snippet didn't run on that toolchain, for the reason given: "
           "a missing library feature (`CPPDOCS_REQUIRE`, `needs=`), a `nocompile` snippet, or an "
           "implementation-defined result. A skip isn't a pass or a failure; it's a known gap.",
           "- **Tests** are the GoogleTest suite (`tests/`); **snippets** are the compile-checked "
           "blocks in `docs/` (`tools/check_snippets.py`). Snippet *passed* = checked − failed; "
           "baselined failures are counted as failed and noted. The top skip reasons leave out "
           "`nocompile` snippets, which skip on every toolchain alike.", "",
           f"Generated {today}.", ""]

    out += ["## GitHub Actions", ""]
    out += ["| Workflow | Platforms | Last consented run |", "|---|---|---|"]
    for wf, title, what in WORKFLOWS:
        badge = (f"[{title}](https://github.com/{REPO}/actions/workflows/{wf}) (`{wf}`)")
        if wf in ran:
            date, url, concl = ran[wf]
            cell = f"[{date}]({url})" + (f" ({concl})" if concl else "")
        else:
            cell = NOT_RUN
        out.append(f"| {badge} | {what} | {cell} |")
    out.append("")
    action_rows = [(label, row, r) for r in consented for label, row in r["rows"].items()]
    if action_rows:
        for r in consented:
            if r["rows"]:
                link = f"[{r['date'] or 'run'}]({r['url']})" if r["url"] else (r["date"] or "-")
                where = " · ".join(x for x in (r["workflow"], r["branch"], r["sha"]) if x)
                if where:
                    out += [f"Run {link}: {where}.", ""]
                out += matrix(r["rows"], link) + [""]
    else:
        out += ["No consented Actions run has produced results yet.", ""]

    out += ["## Local runs", ""]
    if local and local["rows"]:
        out += ["Run on the owner's machine (macOS, Apple silicon) and in the `gcc:16` Docker image "
                f"for the host's architecture, {local['date']}. These are **local runs**, not GitHub "
                "Actions.", ""]
        out += matrix(local["rows"], "local") + [""]
    else:
        out += ["No local results in this refresh; the release notes in "
                "[`docs/releases/`](releases/README.md) carry the local counts.", ""]

    all_rows = [(l, row) for l, row, _ in action_rows] + \
        ([(l, row) for l, row in local["rows"].items()] if local else [])
    out += gaps(all_rows) + [""]
    return "\n".join(out)


def summary(runs: list[dict], local: dict | None) -> str:
    """The matrix and the gaps only, for a workflow run's summary page."""
    out = ["## Platform matrix", "",
           "Tests and snippets: passed / failed / skipped. Refresh docs/platform-support.md "
           "locally with `tools/collect_results.py <run-id>`.", ""]
    rows = []
    for r in runs + ([local] if local else []):
        if r["rows"]:
            link = f"[{r['date'] or 'run'}]({r['url']})" if r["url"] else (r["date"] or "local")
            out += matrix(r["rows"], link) + [""]
            rows += list(r["rows"].items())
    return "\n".join(out + gaps(rows) + [""])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("sources", nargs="*", help="GitHub Actions run IDs or folders of results-*.json")
    ap.add_argument("--local", type=pathlib.Path, help="folder of results-*.json from local runs")
    ap.add_argument("--run-url", default="", help="run link for a folder source (default: none)")
    ap.add_argument("--date", default="", help="run date for a folder source (default: newest 'generated')")
    ap.add_argument("--workflow", default="", help="workflow file for a folder source, e.g. linux.yml")
    ap.add_argument("--keep", type=pathlib.Path, help="download run artifacts here instead of a temp dir")
    ap.add_argument("-o", "--output", type=pathlib.Path, default=PAGE, help=f"default: {PAGE.relative_to(ROOT)}")
    ap.add_argument("--summary", action="store_true",
                    help="only the results tables and gaps (for a run's $GITHUB_STEP_SUMMARY)")
    ap.add_argument("--stdout", action="store_true", help="print the page instead of writing it")
    args = ap.parse_args()
    if not args.sources and not args.local:
        ap.error("give a run ID, a results folder, or --local DIR")
    if args.local and not args.local.is_dir():
        ap.error(f"--local: not a folder: {args.local}")

    try:
        runs = []
        for s in args.sources:
            p = pathlib.Path(s)
            if p.is_dir():
                runs.append(load_folder(p, args.run_url, args.date) | {"workflow": args.workflow})
            elif s.isdigit():
                runs.append(load_run(s, args.keep / s if args.keep else None))
            else:
                ap.error(f"{s}: neither a folder nor a run ID")
        local = load_folder(args.local, "", "") if args.local else None
    except SchemaError as e:
        print(f"collect_results.py: schema mismatch: {e}", file=sys.stderr)
        return 1
    if not any(r["rows"] for r in runs) and not (local and local["rows"]) and not any(r["jobs"] for r in runs):
        print("collect_results.py: no results-*.json found in the sources", file=sys.stderr)
        return 2

    if args.summary:
        page = summary(runs, local)
    else:
        page = render(runs, local, os.environ.get("CPPDOCS_DATE") or dt.date.today().isoformat())
    if args.stdout:
        sys.stdout.write(page)
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(page, encoding="utf-8")
        n = sum(len(r["rows"]) for r in runs) + (len(local["rows"]) if local else 0)
        print(f"wrote {args.output}: {n} rows")
    return 0


if __name__ == "__main__":
    sys.exit(main())
