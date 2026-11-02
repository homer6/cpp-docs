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

The page also has the feature support matrix (tools/feature_support.py): one
row per area, one column per compiler, ✅ / 🟡 n / ❌ n. Each toolchain (platform
x arch x compiler) gets its own page, docs/platform-support/<slug>.md, with
every documented feature (GoogleTest case) and example (```cpp block): ✅ / ❌
/ ⏭ reason / —. Rows come from this checkout. Pages it no longer produces are
removed; `--check` covers them all.

Local results: `--local DIR` copies DIR's results-*.json into
docs/platform-support/local/ (with local.json: date and commit) and reads them
from there; without `--local`, that archive is used (`--no-local` leaves the
local results out).

Archive: a run's results are kept in the repo under
docs/platform-support/runs/<run-id>/ (its results-*.json, one per job, and
run.json: id, URL, date, commit, inputs, jobs and conclusions, a digest of
the counts), so the page can be rebuilt after GitHub's artifact retention runs
out. A run ID that's archived is read from there; one that isn't is
downloaded once and archived (`--no-archive` downloads without archiving;
`--check` never writes). The archive holds only what the report uses (#214):
the files are compact JSON, a job file a newer run superseded is removed from
the older run's folder (its run.json keeps the counts), and a run folder not
named on the command line is removed; git history keeps them all. So give
every run the report should show.

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

import feature_support

ROOT = pathlib.Path(__file__).resolve().parent.parent
PAGE = ROOT / "docs" / "platform-support.md"
ARCHIVE = ROOT / "docs" / "platform-support" / "runs"
BUGS = ROOT / "docs" / "platform-support" / "known-bugs.json"
TOOLCHAINS = ROOT / "docs" / "platform-support"            # one <slug>.md per toolchain
LOCAL_ARCHIVE = ROOT / "docs" / "platform-support" / "local"
# Hand-written summary of the results (#226), shown at the top of the folder README.
NOTES = ROOT / "docs" / "platform-support" / "notes.md"
NOTES_RUN = re.compile(r"<!--\s*notes-run:\s*(\d+)\s*-->")
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
     "windows-2022 (VS 2022), windows-2025-vs2026 and windows-11-arm (VS 2026): MSVC"),
    ("windows-gnu.yml", "Windows (clang-cl, MinGW)",
     "windows-2025-vs2026: clang-cl, MSYS2 UCRT64 GCC and Clang (libstdc++), CLANG64 Clang (libc++)"),
    ("freebsd.yml", "FreeBSD",
     "FreeBSD 14.5 and 15.1 VMs, x86_64: base Clang/libc++, pkg GCC"),
]
NOT_RUN = "not run yet (each run needs the owner's consent)"

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


def unpack(d, where: str) -> list[dict]:
    """The tests/snippets documents in one result.

    A platform workflow job (tools/ci/cppdocs_ci.py collect) writes one
    document of kind "run" with the tests and snippets documents nested
    inside (#160); local runs write the two kinds directly. A run's snippet
    mode ("enforced" or "report-only", #159) is carried onto its snippets
    document as "mode".
    """
    if isinstance(d, dict) and d.get("kind") == "run":
        if d.get("schema") != SCHEMA:
            raise SchemaError(f"{where}: schema is {d.get('schema')!r}, expected {SCHEMA!r}")
        out = [{"kind": "run", **{k: v for k, v in d.items() if k not in ("tests", "snippets", "kind")}}]
        for sub in ("tests", "snippets"):
            if d.get(sub) is not None:
                doc = validate(d[sub], f"{where} ({sub})")
                if sub == "snippets":
                    doc = {**doc, "mode": d.get("snippets_mode", "enforced")}
                out.append(doc)
        if len(out) == 1:
            raise SchemaError(f"{where}: a 'run' result with neither tests nor snippets "
                              "(the job's configure or build step probably failed)")
        return out
    return [validate(d, where)]


def docs(row: dict[str, dict]) -> list[dict]:
    """A row's tests and snippets documents (not the job's "run" metadata)."""
    return [row[k] for k in ("tests", "snippets") if k in row]


def load_dir(folder: pathlib.Path) -> dict[str, dict[str, dict]]:
    """{label: {kind: doc}} for every results-*.json under folder.

    kind is "tests", "snippets" and, for a platform workflow job, "run": the
    job's own fields (os_version, inputs, steps, ok, run link), without the
    nested documents.
    """
    rows: dict[str, dict[str, dict]] = {}
    files = sorted(folder.rglob("results-*.json"))
    for f in files:
        try:
            data = json.loads(f.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as e:
            raise SchemaError(f"{f}: not readable JSON ({e})") from None
        docs = data if isinstance(data, list) else [data]
        for i, d in enumerate(docs):
            where = f"{f}" + (f"[{i}]" if isinstance(data, list) else "")
            row = rows.setdefault(label_of(f), {})
            for doc in unpack(d, where):
                if doc["kind"] in row:
                    raise SchemaError(f"{f}: a second {doc['kind']!r} result for row {label_of(f)!r}; "
                                      "give each job's file a unique name")
                row[doc["kind"]] = doc
    for row in rows.values():
        fix_stdlib(row)
    return rows


# MSYS2's Clang environments (CLANG64, CLANGARM64) build with libc++.
MSYS2_LIBCXX = re.compile(r"-CLANG(64|ARM64)$")


def fix_stdlib(row: dict[str, dict]) -> None:
    """Correct the standard library of results written before tests/CMakeLists.txt
    detected it from the library's macros (#190): ctest_to_json.py guessed
    libstdc++ for every Clang on Windows, but MSYS2 CLANG64 uses libc++."""
    platform = (row.get("run") or {}).get("platform", "")
    if not MSYS2_LIBCXX.search(platform):
        return
    for doc in docs(row):
        comp = doc.get("compiler")
        if isinstance(comp, dict) and comp.get("id") == "Clang" and comp.get("stdlib") == "libstdc++":
            doc["compiler"] = {**comp, "stdlib": "libc++"}


def gh_json(args: list[str]):
    r = subprocess.run(["gh", *args], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"collect_results.py: gh {' '.join(args)} failed: {r.stderr.strip()}")
    return json.loads(r.stdout)


def run_record(run_id: str, meta: dict, jobs: list[dict], rows: dict[str, dict[str, dict]]) -> dict:
    """The run's metadata for docs/platform-support/runs/<id>/run.json."""
    inputs = []
    for row in rows.values():
        i = (row.get("run") or {}).get("inputs")
        if i is not None and i not in inputs:
            inputs.append(i)
    digest = {}
    for label, row in sorted(rows.items()):
        digest[label] = {k: row[k]["counts"] for k in ("tests", "snippets") if k in row}
        if "run" in row:
            digest[label]["ok"] = row["run"].get("ok")
    return {"schema": SCHEMA, "kind": "archived-run", "id": int(run_id) if run_id.isdigit() else run_id,
            "url": meta.get("html_url", ""),
            "date": (meta.get("run_started_at") or meta.get("created_at") or "")[:10],
            "started": meta.get("run_started_at") or meta.get("created_at") or "",
            "workflow": pathlib.PurePosixPath(meta.get("path", "")).name,
            "branch": meta.get("head_branch", ""), "sha": meta.get("head_sha") or "",
            "event": meta.get("event", ""),
            "conclusion": meta.get("conclusion") or meta.get("status", ""),
            "inputs": inputs,
            "jobs": [{"name": j.get("name", ""), "conclusion": j.get("conclusion") or j.get("status", "")}
                     for j in jobs],
            "digest": digest}


def from_record(rec: dict, rows: dict[str, dict[str, dict]]) -> dict:
    return {"rows": rows, "url": rec.get("url", ""), "date": rec.get("date", ""),
            "workflow": rec.get("workflow", ""), "branch": rec.get("branch", ""),
            "sha": (rec.get("sha") or "")[:7], "conclusion": rec.get("conclusion", ""),
            "jobs": [(j.get("name", ""), j.get("conclusion", "")) for j in rec.get("jobs", [])]}


def run_number(r: dict) -> int:
    tail = (r.get("url") or "").rstrip("/").rsplit("/", 1)[-1]
    return int(tail) if tail.isdigit() else 0


def supersede(runs: list[dict]) -> None:
    """Keep each job only from the newest run that has it (#190).

    The latest run of every platform workflow is what the report shows, but an
    all.yml run covers several workflows, and a later fix-up run may re-run only
    some of them. So a row (results label) seen in a newer run is dropped from
    older ones, and so are an older run's caller jobs ("linux.yml / <job>") for a
    workflow a newer run covered."""
    order = sorted(runs, key=run_number, reverse=True)
    seen_rows: set[str] = set()
    seen_wf: set[str] = set()
    for r in order:
        if not run_number(r):
            continue
        covered = {j.split(" / ")[0].strip() for j, c in r["jobs"] if c != "skipped" and " / " in j}
        if r.get("workflow") and r["workflow"] != "all.yml":
            covered.add(r["workflow"])
        r["rows"] = {k: v for k, v in r["rows"].items() if k not in seen_rows}
        r["jobs"] = [(j, c) for j, c in r["jobs"]
                     if not (" / " in j and j.split(" / ")[0].strip() in seen_wf)]
        seen_rows |= set(r["rows"])
        seen_wf |= covered


def load_run(run_id: str, keep: pathlib.Path | None, archive: pathlib.Path | None = ARCHIVE,
             save: bool = True) -> dict:
    """A run's results: from the archive, else downloaded (never starts or re-runs anything)."""
    folder = archive / run_id if archive else None
    if folder and (folder / "run.json").is_file():
        try:
            rec = json.loads((folder / "run.json").read_text(encoding="utf-8"))
        except json.JSONDecodeError as e:
            raise SchemaError(f"{folder / 'run.json'}: not readable JSON ({e})") from None
        return from_record(rec, load_dir(folder))
    meta = gh_json(["api", f"repos/{REPO}/actions/runs/{run_id}"])
    jobs = gh_json(["api", "--paginate", "--slurp", f"repos/{REPO}/actions/runs/{run_id}/jobs?per_page=100"])
    jobs = [j for page in jobs for j in page.get("jobs", [])]
    dest = keep or pathlib.Path(tempfile.mkdtemp(prefix=f"run-{run_id}-"))
    dest.mkdir(parents=True, exist_ok=True)
    r = subprocess.run(["gh", "run", "download", run_id, "-R", REPO, "-D", str(dest),
                        "--pattern", "results-*"], capture_output=True, text=True)
    if r.returncode and "no valid artifacts" not in r.stderr:
        sys.exit(f"collect_results.py: gh run download {run_id} failed: {r.stderr.strip()}")
    rows = load_dir(dest)
    rec = run_record(run_id, meta, jobs, rows)
    if folder and save:
        archive_run(folder, dest, rec)
    return from_record(rec, rows)


def archive_run(folder: pathlib.Path, downloaded: pathlib.Path, rec: dict) -> None:
    """Copy each job's results-*.json (not the logs or the ci-out/ copies) and write run.json."""
    folder.mkdir(parents=True, exist_ok=True)
    for f in sorted(downloaded.rglob("results-*.json")):
        data = json.loads(f.read_text(encoding="utf-8"))
        write_compact(folder / f.name, data)
    (folder / "run.json").write_text(json.dumps(rec, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"archived run {rec['id']} in {folder.relative_to(ROOT) if ROOT in folder.parents else folder}")


def write_compact(path: pathlib.Path, data) -> bool:
    """Write an archived results file as one line of JSON (#214); True if it changed."""
    text = json.dumps(data, separators=(",", ":"), ensure_ascii=False) + "\n"
    if path.is_file() and path.read_text(encoding="utf-8") == text:
        return False
    path.write_text(text, encoding="utf-8")
    return True


def prune_archive(archive: pathlib.Path, runs: list[dict], keep: set[str]) -> list[str]:
    """Keep only what the report uses (#214): each archived run's job files for the
    rows that survived supersede(), and only the run folders named in keep. A
    removed file or folder stays in git history; run.json keeps the counts of
    every job. The kept files are rewritten compactly. Returns what was removed."""
    removed: list[str] = []
    if not archive.is_dir():
        return removed
    for r in runs:
        rid = str(run_number(r))
        folder = archive / rid
        if rid not in keep or not (folder / "run.json").is_file():
            continue
        for f in sorted(folder.glob("results-*.json")):
            if label_of(f) in r["rows"]:
                write_compact(f, json.loads(f.read_text(encoding="utf-8")))
            else:
                f.unlink()
                removed.append(f"{rid}/{f.name}")
    for folder in sorted(archive.iterdir()):
        if folder.is_dir() and folder.name.isdigit() and folder.name not in keep \
                and (folder / "run.json").is_file():
            for f in sorted(folder.iterdir()):
                f.unlink()
            folder.rmdir()
            removed.append(f"{folder.name}/")
    return removed


def load_folder(folder: pathlib.Path, url: str, date: str) -> dict:
    rows = load_dir(folder)
    if not date:
        stamps = [d["generated"] for r in rows.values() for d in docs(r)]
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
    counts = collections.Counter(short_reason(s["reason"]) for d in docs(row) for s in d["skips"]
                                 if s["reason"] != "nocompile")
    return "<br>".join(f"{n}× {r}" for r, n in counts.most_common(TOP_SKIPS)) or "-"


def missing_tests(row: dict[str, dict]) -> str:
    """Why a job that was asked to run the tests has no tests result, or ""."""
    run = row.get("run")
    if "tests" in row or not run or not (run.get("inputs") or {}).get("tests"):
        return ""
    code = (run.get("steps") or {}).get("tests")
    return f"no results (tests step exit {code})" if code not in (None, 0) else "no results"


def tests_cell(doc: dict | None, row: dict[str, dict] | None = None) -> str:
    if not doc:
        why = missing_tests(row or {})
        return f"failed: {why}" if why else "-"
    c = doc["counts"]
    return f"{c['passed']} / {c['failed']} / {c['skipped']}"


def snippets_cell(doc: dict | None) -> str:
    if not doc:
        return "-"
    c = doc["counts"]
    known = (doc["baseline"] or {}).get("known_failing", 0)
    cell = f"{c['checked'] - c['failed']} / {c['failed']} / {c['skipped']}"
    if doc.get("mode") == "report-only":
        return cell + " (report-only)"
    return cell + (f" ({known} in baseline)" if known else "")


def describe(row: dict[str, dict]) -> tuple[str, str, str, str]:
    """(OS, arch, compiler, preset/toolchain) for a row, preferring the tests document."""
    main = row.get("tests") or row["snippets"]
    comp = f"{family(main)} {version_of(main)}"
    stdlib = main["compiler"].get("stdlib") or ""
    if stdlib and stdlib != "unknown":
        comp += f", {stdlib}"
    preset = row["tests"]["preset"] if "tests" in row else row["snippets"]["toolchain"]
    return os_label(row), arch_of(main), comp, f"`{preset}`"


def matrix(rows: dict[str, dict[str, dict]], link: str) -> list[str]:
    out = ["| Platform | OS | Arch | Compiler | Preset | Tests passed / failed / skipped "
           "| Snippets passed / failed / skipped | Top skip reasons | Run |",
           "|---|---|---|---|---|---|---|---|---|"]
    for label in sorted(rows):
        row = rows[label]
        os_, arch, comp, preset = describe(row)
        name = f"[{label}]({row['page']})" if row.get("page") else label
        out.append(f"| {name} | {os_} | {arch} | {comp} | {preset} | {tests_cell(row.get('tests'), row)} "
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


def load_bugs(path: pathlib.Path = BUGS) -> list[dict]:
    """The hand-maintained list of compiler and platform bugs the runs found (known-bugs.json).

    Each entry: toolchain, summary, upstream ([{label, url}], empty until a vendor bug is
    found or filed), issues (repo issue numbers) and match (a regex for the skip reason,
    gate or note in tests/ or docs/examples/ that records it).
    """
    if not path.is_file():
        return []
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        raise SchemaError(f"{path}: not readable JSON ({e})") from None
    if not isinstance(data, list):
        raise SchemaError(f"{path}: expected a JSON list")
    for i, b in enumerate(data):
        where = f"{path}[{i}]"
        for key, typ in (("toolchain", str), ("summary", str), ("upstream", list), ("issues", list), ("match", str)):
            if not isinstance(b, dict) or not isinstance(b.get(key), typ):
                raise SchemaError(f"{where}: field {key!r} missing or not {typ.__name__}")
        if any(not isinstance(u, dict) or not isinstance(u.get("label"), str) or
               not str(u.get("url", "")).startswith("https://") for u in b["upstream"]):
            raise SchemaError(f"{where}: each upstream entry needs a label and an https:// url")
        if any(not isinstance(n, int) for n in b["issues"]):
            raise SchemaError(f"{where}: issues must be issue numbers")
        try:
            re.compile(b["match"])
        except re.error as e:
            raise SchemaError(f"{where}: match isn't a regex ({e})") from None
    return data


def recorded_in(rx: re.Pattern, root: pathlib.Path = ROOT) -> bool:
    """Whether a bug's gate or note is in tests/ or docs/examples/ in this checkout."""
    for folder, pat in (("tests", "*.cpp"), ("tests", "*.hpp"), ("tests", "*.cmake"),
                        ("tests", "CMakeLists.txt"), ("docs/examples", "*.md")):
        for f in (root / folder).rglob(pat):
            if rx.search(f.read_text(encoding="utf-8", errors="replace")):
                return True
    return False


def bugs_table(bugs: list[dict], all_rows: list[tuple[str, dict[str, dict]]]) -> list[str]:
    out = ["### Compiler and platform bugs found", "",
           "Bugs the runs turned up, each recorded in the suite as a skip or gate that names it, "
           "with the vendor's bug and this repo's issue. The list is "
           "[`docs/platform-support/known-bugs.json`](platform-support/known-bugs.json), maintained "
           "by hand; *seen* counts the jobs in these results whose skip reasons name it.", ""]
    if not bugs:
        return out + ["None recorded yet.", ""]
    out += ["| Toolchain | Bug | Upstream | Issue | Seen |", "|---|---|---|---|---|"]
    for b in bugs:
        rx = re.compile(b["match"])
        seen = [label for label, row in all_rows
                if any(rx.search(s["reason"]) for d in docs(row) for s in d["skips"])]
        up = ", ".join(f"[{u['label']}]({u['url']})" for u in b["upstream"]) or \
            "none found yet ([#185](https://github.com/homer6/cpp-docs/issues/185))"
        issues = ", ".join(f"[#{n}](https://github.com/{REPO}/issues/{n})" for n in b["issues"])
        note = "" if recorded_in(rx) else " *(its gate isn't in this checkout)*"
        out.append(f"| {b['toolchain']} | {b['summary']}{note} | {up} | {issues} | "
                   f"{len(seen)} job(s)" + (f": {', '.join(seen[:4])}{'…' if len(seen) > 4 else ''}" if seen else "")
                   + " |")
    return out + [""]


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
    return out + [""] + bugs_table(load_bugs(), all_rows)


def render(runs: list[dict], local: dict | None, today: str, matrix_rows: bool = True) -> str:
    if matrix_rows:
        ensure_pages(runs, local)
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
            # An all.yml run's conclusion covers every workflow it ran; use this
            # workflow's own jobs where the run has them (#190).
            mine = [c for job, c in r["jobs"] if job.split(" / ")[0].strip() == w and c != "skipped"]
            concl = ("failure" if any(c != "success" for c in mine) else "success") if mine else r["conclusion"]
            ran[w] = (r["date"], r["url"], concl)
    last = max([r["date"] for r in consented if r["rows"] and r["date"]] + [d for d, _, _ in ran.values() if d],
               default="")
    out = ["# Platform support", "",
           "<!-- Generated by tools/collect_results.py; don't edit by hand. "
           "Refresh it only from runs the owner consented to (see CLAUDE.md). -->", "",
           "> Which platforms, compilers and architectures the test suite and the snippet checker "
           "have run on, with passed, failed and skipped counts.", "",
           "The goal is every documented feature and every example on every supported platform, "
           "architecture and compiler, so the counts and skip reasons show how far each toolchain "
           "supports modern C++ (see [testing.md](testing.md#why-we-test-on-every-platform)).", "",
           "## How to read this", "",
           "- **Workflows run only by hand.** Every GitHub Actions workflow here is `workflow_dispatch` "
           "only, and runs only with the owner's consent, given for each run: before a release (on "
           "`dev`), or to back a claim about a platform we can't run locally. The tables below show "
           "the **last consented run** of each workflow, not the current tip of any branch; the README "
           "badges summarise these results and the local runs (✓ when every toolchain passes).",
           f"- **Last consented run:** {last or 'none yet'}.",
           "- **Report-only snippets.** Snippet runs are enforced against a reviewed baseline only for "
           "GCC 15/16 and Clang/LLVM 22 with libc++ (off Windows). Every other toolchain's snippet "
           "results are marked *(report-only)*: listed here, but they don't fail the run until that "
           "toolchain's baseline is seeded from a consented run and reviewed (#159).",
           "- **Local runs are the primary proof.** The owner's machine (GCC 15, Clang 22) and the "
           "GCC 16 Docker image are checked for every release; their results are labelled *local*.",
           "- **Skipped** means a test or snippet didn't run on that toolchain, for the reason given: "
           "a missing library feature (`CPPDOCS_REQUIRE`, `needs=`), a `nocompile` snippet, or an "
           "implementation-defined result. A skip isn't a pass or a failure; it's a known gap.",
           "- **Tests** are the GoogleTest suite (`tests/`); **snippets** are the compile-checked "
           "blocks in `docs/` (`tools/check_snippets.py`). Snippet *passed* = checked − failed; "
           "baselined failures are counted as failed and noted. The top skip reasons leave out "
           "`nocompile` snippets, which skip on every toolchain alike.",
           "- **Per feature:** the [feature support matrix](#feature-support-matrix) below summarizes "
           "every area per toolchain; **each toolchain has its own page** in "
           "[`platform-support/`](platform-support/) (linked from every table row and column) with "
           "every area, page, feature and example. The [bugs found](#compiler-and-platform-bugs-found) "
           "close this page.",
           "- **Archived:** the results and metadata of the runs this page uses are kept in "
           "[`platform-support/runs/<run-id>/`](platform-support/runs/) and the local results in "
           "[`platform-support/local/`](platform-support/local/), so this page and the toolchain pages "
           "can be rebuilt after GitHub's artifact retention runs out.", "",
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
                scope = areas_of(r["rows"])
                where = " · ".join(x for x in (r["workflow"], r["branch"], r["sha"], scope) if x)
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
    if matrix_rows:
        out += feature_support.summary(jobs_of(runs, local), universe()).rstrip("\n").split("\n") + [""]
    out += gaps(all_rows) + [""]
    return "\n".join(out)


def areas_of(rows: dict[str, dict[str, dict]]) -> str:
    """The test areas a run's jobs covered ("areas `^strings$`", "all areas"), or "" if unknown."""
    found = {(row["run"].get("inputs") or {}).get("areas") or "" for row in rows.values() if "run" in row}
    if not found:
        return ""
    return "all areas" if found == {""} else "areas " + ", ".join(f"`{a or 'all'}`" for a in sorted(found))


# --- the README's platform table --------------------------------------------

README = ROOT / "README.md"
README_START, README_END = "<!-- platforms:start -->", "<!-- platforms:end -->"


def short_version(v: str) -> str:
    m = re.match(r"\d+(?:\.\d+){0,2}", v)
    return m[0] if m else v


def os_label(row: dict[str, dict]) -> str:
    """The OS or runner image a row ran on, short: "Ubuntu 24.04", "macOS 15", "Windows Server 2025"."""
    run = row.get("run") or {}
    v, plat = run.get("os_version") or "", run.get("platform") or ""
    if m := re.search(r"MSYS2 (\w+)", v):
        w = re.match(r"windows-(\d+)", plat)
        base = ("Windows Server " if w and len(w[1]) == 4 else "Windows ") + w[1] if w else "Windows"
        return f"{base}, MSYS2 {m[1]}"
    for pat, fmt in ((r"Ubuntu (\d+\.\d+)", "Ubuntu {}"), (r"macOS (\d+)", "macOS {}"),
                     (r"FreeBSD (\d+\.\d+)", "FreeBSD {}"), (r"Microsoft Windows (Server \d+|\d+)", "Windows {}"),
                     (r"Debian(?: GNU/Linux)? (\d+)", "Debian {}")):
        if m := re.match(pat, v):
            return fmt.format(m[1]) + (" (QEMU user-mode)" if "QEMU" in v else "")
    if v:
        return v.split(",")[0]
    main = row.get("tests") or row["snippets"]  # a local run: no job metadata
    p = main["platform"]
    if p["system"] == "Linux" and "linuxkit" in p["version"]:
        return "Linux (Docker)"
    return os_of(main)


def compiler_label(row: dict[str, dict]) -> str:
    main = row.get("tests") or row["snippets"]
    fam = family(main)
    out = f"{fam} {short_version(version_of(main))}"
    stdlib = main["compiler"].get("stdlib") or ""
    if fam == "Clang" and stdlib and stdlib != "unknown":
        out += f", {stdlib}"
    return out


def readme_tests(row: dict[str, dict]) -> str:
    doc = row.get("tests")
    if not doc:
        why = missing_tests(row)
        return f"❌ {why}" if why else "-"
    c = doc["counts"]
    if c["failed"]:
        return f"❌ {c['failed']} failed ({c['passed']}/{c['total']})"
    return f"✅ {c['passed']}/{c['total']}" + (f" ({c['skipped']} skipped)" if c["skipped"] else "")


def readme_snippets(row: dict[str, dict]) -> str:
    doc = row.get("snippets")
    if not doc:
        return "-"
    c = doc["counts"]
    new = doc["baseline"]["new"] if doc["baseline"] is not None else c["failed"]
    notes = []
    if c["failed"] - new:
        notes.append(f"{c['failed'] - new} baselined")
    if c["skipped"]:
        notes.append(f"{c['skipped']} skipped")
    if doc.get("mode") == "report-only":
        notes.append("report-only")
    cell = f"{'✅' if new == 0 else '❌'} {c['checked'] - c['failed']}/{c['checked']}"
    return cell + (f" ({', '.join(notes)})" if notes else "")


def readme_block(runs: list[dict], local: dict | None) -> str:
    """The README's platform table: one row per platform x compiler x arch, newest results."""
    order = {w: i for i, (w, _, _) in enumerate(WORKFLOWS)}
    ci = sorted((r for r in runs if r["rows"]), key=lambda r: order.get(r["workflow"], len(order)))
    entries = []  # (row, run cell)
    if local and local["rows"]:
        for label in sorted(local["rows"]):
            entries.append((local["rows"][label], f"local, {local['date']}" if local["date"] else "local"))
    for r in ci:
        for label in sorted(r["rows"]):
            row = r["rows"][label]
            url = (row.get("run") or {}).get("run", {}).get("url") or r["url"]
            entries.append((row, f"[{r['date'] or 'run'}]({url})" if url else (r["date"] or "-")))
    scopes = {areas_of(r["rows"]) for r in ci} - {""}
    per_row_scope = len(scopes) > 1
    dates = [r["date"] for r in ci if r["date"]] + ([local["date"]] if local and local["rows"] and local["date"] else [])
    out = ["<!-- Generated by tools/collect_results.py from consented runs and local results; "
           "don't edit by hand (see CLAUDE.md). -->", ""]
    if not entries:
        return "\n".join(out + ["No results yet. Every feature and example per compiler: "
                                "[`docs/platform-support.md`](docs/platform-support.md#feature-support-matrix)."])
    out.append(f"{len(entries)} platform × compiler × architecture combinations, results as of "
               f"{max(dates) if dates else 'unknown'}. **Tests**: GoogleTest cases passed / total; "
               "**snippets**: compile-checked blocks passed / checked. *report-only*: that toolchain's "
               "snippet results don't fail its run yet (no reviewed baseline). Skip reasons, known gaps "
               "and full counts: [`docs/platform-support.md`](docs/platform-support.md), with the "
               "[feature support matrix](docs/platform-support.md#feature-support-matrix) and one page per "
               "toolchain (every feature and example) in [`docs/platform-support/`](docs/platform-support/).")
    if scopes and not per_row_scope:
        out[-1] += (f" The GitHub Actions runs here cover {next(iter(scopes))} (the `areas` input)"
                    + ("; the local runs cover the whole suite." if local and local["rows"] else "."))
    out += ["", "| Platform | Compiler | Arch | Tests | Snippets |" + (" Scope |" if per_row_scope else "") + " Run |",
            "|---|---|---|---|---|" + ("---|" if per_row_scope else "") + "---|"]
    for row, run_cell in entries:
        scope = ""
        if per_row_scope:
            scope = (areas_of({"r": row}) or "all areas") + " | "
        out.append(f"| {os_label(row)} | {compiler_label(row)} | {arch_of(row.get('tests') or row['snippets'])} "
                   f"| {readme_tests(row)} | {readme_snippets(row)} | {scope}{run_cell} |")
    return "\n".join(out)


# --- the README's platform badges (#230) ---------------------------------------
# Our own badges, from the report's results (consented runs and local runs), not
# GitHub's workflow badges: those follow one branch and miss the local runs.
BADGES_START, BADGES_END = "<!-- badges:start -->", "<!-- badges:end -->"
BADGE_DIR = ROOT / "docs" / "platform-support" / "badges"
BADGE_GROUPS = [("linux", "Linux"), ("macos", "macOS"), ("windows", "Windows"), ("freebsd", "FreeBSD"),
                ("local", "Local")]
GREEN, RED, GREY = "#2e7d32", "#c62828", "#555"


def row_ok(row: dict[str, dict]) -> bool:
    """No failing test, and no snippet failure outside the baseline where snippets are enforced."""
    tests = row.get("tests")
    if tests and tests["counts"]["failed"]:
        return False
    if not tests and missing_tests(row):
        return False
    snip = row.get("snippets")
    if snip and snip.get("mode") != "report-only":
        new = snip["baseline"]["new"] if snip.get("baseline") is not None else snip["counts"]["failed"]
        if new:
            return False
    return True


def badge_counts(runs: list[dict], local: dict | None) -> dict[str, tuple[int, int]]:
    """{group: (toolchains, failing)} for each badge group with results."""
    out: dict[str, list[int]] = {}
    groups = [(r["rows"], False) for r in runs] + ([(local["rows"], True)] if local else [])
    for rows, is_local in groups:
        for label, row in rows.items():
            g = "local" if is_local else label.split("-", 1)[0]
            if g not in dict(BADGE_GROUPS):
                continue
            c = out.setdefault(g, [0, 0])
            c[0] += 1
            c[1] += 0 if row_ok(row) else 1
    return {g: (c[0], c[1]) for g, c in out.items()}


def badge_svg(label: str, message: str, color: str) -> str:
    """A flat badge: grey label, coloured message (widths estimated, 11px Verdana)."""
    def width(t: str) -> int:
        return int(sum(7.5 if ch.isupper() or ch in "✓✗mwMW" else 6.6 for ch in t)) + 14
    lw, mw = width(label), width(message)
    w = lw + mw
    message = message.replace("✓", "&#10003;").replace("✗", "&#10007;")  # ASCII-safe SVG
    title = f"{label}: {message}"
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="20" role="img" aria-label="{title}">'
            f'<title>{title}</title>'
            f'<linearGradient id="s" x2="0" y2="100%"><stop offset="0" stop-color="#bbb" stop-opacity=".1"/>'
            f'<stop offset="1" stop-opacity=".1"/></linearGradient>'
            f'<clipPath id="r"><rect width="{w}" height="20" rx="3" fill="#fff"/></clipPath>'
            f'<g clip-path="url(#r)"><rect width="{lw}" height="20" fill="{GREY}"/>'
            f'<rect x="{lw}" width="{mw}" height="20" fill="{color}"/>'
            f'<rect width="{w}" height="20" fill="url(#s)"/></g>'
            f'<g fill="#fff" text-anchor="middle" font-family="Verdana,Geneva,DejaVu Sans,sans-serif" '
            f'font-size="11">'
            f'<text x="{lw / 2:.1f}" y="15" fill="#010101" fill-opacity=".3">{label}</text>'
            f'<text x="{lw / 2:.1f}" y="14">{label}</text>'
            f'<text x="{lw + mw / 2:.1f}" y="15" fill="#010101" fill-opacity=".3">{message}</text>'
            f'<text x="{lw + mw / 2:.1f}" y="14">{message}</text></g></svg>\n')


def badges(runs: list[dict], local: dict | None) -> dict[str, str]:
    """{file name: SVG} for docs/platform-support/badges/."""
    out = {}
    for g, (n, bad) in badge_counts(runs, local).items():
        name = dict(BADGE_GROUPS)[g]
        tc = "toolchain" if n == 1 else "toolchains"
        msg, color = (f"✓ {n} {tc}", GREEN) if not bad else (f"✗ {bad} of {n} failing", RED)
        out[f"{g}.svg"] = badge_svg(name, msg, color)
    return out


def badges_block(runs: list[dict], local: dict | None) -> str:
    """The README's badge row: one badge per platform, linking the platform support notes."""
    counts = badge_counts(runs, local)
    out = ["<!-- Generated by tools/collect_results.py from the report's results (consented runs and "
           "local runs); don't edit by hand (see CLAUDE.md). -->"]
    for g, name in BADGE_GROUPS:
        if g not in counts:
            continue
        n, bad = counts[g]
        alt = f"{name}: {n - bad} of {n} toolchains passing"
        out.append(f"[![{alt}](docs/platform-support/badges/{g}.svg)](docs/platform-support/notes.md)")
    return "\n".join(out)


def sync_badges(folder: pathlib.Path, svgs: dict[str, str], write: bool) -> list[str]:
    stale = []
    for name, text in svgs.items():
        f = folder / name
        if not f.is_file() or f.read_text(encoding="utf-8") != text:
            stale.append(f"badges/{name}")
            if write:
                folder.mkdir(parents=True, exist_ok=True)
                f.write_text(text, encoding="utf-8")
    for f in sorted(folder.glob("*.svg")) if folder.is_dir() else []:
        if f.name not in svgs:
            stale.append(f"badges/{f.name} (no longer produced)")
            if write:
                f.unlink()
    return stale


def update_block(text: str, start: str, end: str, block: str) -> str:
    pattern = re.compile(re.escape(start) + r".*?" + re.escape(end), re.S)
    if not pattern.search(text):
        raise SchemaError(f"README.md is missing the {start} / {end} markers")
    return pattern.sub(lambda _: f"{start}\n{block}\n{end}", text)


def update_readme(text: str, block: str) -> str:
    pattern = re.compile(re.escape(README_START) + r".*?" + re.escape(README_END), re.S)
    if not pattern.search(text):
        raise SchemaError(f"README.md is missing the {README_START} / {README_END} markers")
    return pattern.sub(lambda _: f"{README_START}\n{block}\n{README_END}", text)


# --- the feature matrix and the per-toolchain pages (tools/feature_support.py) ----

_UNIVERSE = None


def universe() -> "feature_support.Universe":
    global _UNIVERSE
    if _UNIVERSE is None:
        _UNIVERSE = feature_support.Universe(ROOT)
    return _UNIVERSE


def toolchain_slug(row: dict[str, dict], local: bool) -> str:
    """A stable file name for one platform x arch x compiler: lowercase, no patch versions,
    the same across runs (linux-ubuntu-24.04-x86_64-clang-22-libcxx, local-macos-arm64-gcc-15)."""
    main = row.get("tests") or row["snippets"]
    os_ = os_label(row).lower()
    if local:
        system = main["platform"]["system"].lower()
        os_part = "local-" + {"darwin": "macos"}.get(system, system) + ("-docker" if "docker" in os_ else "")
    else:
        os_part = os_.replace("(qemu user-mode)", "qemu")
        if re.match(r"(ubuntu|debian|fedora|alpine|arch)\b", os_part):
            os_part = "linux-" + os_part
    fam = family(main)
    parts = short_version(version_of(main)).split(".")
    comp = fam.lower().replace(" ", "-") + "-" + (".".join(parts[:2]) if fam == "MSVC" else parts[0])
    stdlib = main["compiler"].get("stdlib") or ""
    if fam == "Clang" and stdlib in ("libc++", "libstdc++"):
        comp += "-" + stdlib.replace("+", "x")
    return re.sub(r"[^a-z0-9._]+", "-", f"{os_part}-{arch_of(main).lower()}-{comp}").strip("-")


def assign_pages(runs: list[dict], local: dict | None) -> None:
    """Give every row its toolchain page ("page": platform-support/<slug>.md), unique names."""
    seen: set[str] = set()
    for r, is_local in [(r, False) for r in runs] + ([(local, True)] if local else []):
        for label in sorted(r["rows"]):
            slug = base = toolchain_slug(r["rows"][label], is_local)
            n = 2
            while slug in seen:
                slug, n = f"{base}-{n}", n + 1
            seen.add(slug)
            r["rows"][label]["page"] = f"platform-support/{slug}.md"


def job_of(row: dict[str, dict], r: dict, local: bool) -> dict:
    """One CI job or local run, for feature_support (summary columns and its own page)."""
    main = row.get("tests") or row["snippets"]
    fam = family(main)
    parts = short_version(version_of(main)).split(".")
    major = ".".join(parts[:2]) if fam == "MSVC" else parts[0]
    stdlib = main["compiler"].get("stdlib") or ""
    snip = row.get("snippets")
    run = row.get("run") or {}
    title = f"{compiler_label(row)} on {os_label(row)}, {arch_of(main)}" + (" (local)" if local else "")
    if local:
        run_cell = f"local run, {r['date']}" + (f", commit `{r['sha']}`" if r.get("sha") else "")
    else:
        url = (run.get("run") or {}).get("url") or r["url"]
        what = " · ".join(x for x in (r["workflow"], r["date"]) if x) or "run"
        run_cell = (f"[{what}]({url})" if url else what) + (f", commit `{r['sha']}`" if r["sha"] else "")
    areas = (run.get("inputs") or {}).get("areas")
    facts = [("OS / image", esc_cell(run.get("os_version") or os_of(main))),
             ("Compiler", esc_cell(f"{fam} {version_of(main)}") + (f" (preset `{row['tests']['preset']}`)"
                                                                  if "tests" in row else "")),
             ("Standard library", esc_cell(stdlib or "unknown")),
             ("Architecture", arch_of(main) + (f", {run['byte_order']}-endian" if run.get("byte_order") else "")),
             ("Run", run_cell),
             ("Areas", ("all" if not areas else f"`{areas}`") if run else "all (local run)"),
             ("Tests", f"{main['counts']['total']} run" if "tests" in row else "not run"),
             ("Snippets", (f"{snip['counts']['total']} blocks in {', '.join(f'`{x}`' for x in snip.get('paths') or ['docs'])}, "
                           + ("**report-only**" if snip.get("mode") == "report-only" else "enforced"))
              if snip else "not run")]
    return {"family": fam, "version": major, "stdlib": "" if stdlib == "unknown" else stdlib,
            "report_only": bool(snip and snip.get("mode") == "report-only"),
            "compiler": compiler_label(row), "local": local, "title": title, "facts": facts,
            "file": row["page"], "where": f"{os_label(row)} · {arch_of(main)}",
            "tests": row.get("tests"), "snippets": snip, "run": row.get("run")}


def esc_cell(text: str) -> str:
    return text.replace("|", "\\|")


def ensure_pages(runs: list[dict], local: dict | None) -> None:
    rows = [row for r in runs + ([local] if local else []) for row in r["rows"].values()]
    if any("page" not in row for row in rows):
        assign_pages(runs, local)


def jobs_of(runs: list[dict], local: dict | None) -> list[dict]:
    ensure_pages(runs, local)
    jobs = [job_of(r["rows"][label], r, False) for r in runs for label in sorted(r["rows"])]
    if local and local["rows"]:
        jobs += [job_of(local["rows"][label], local, True) for label in sorted(local["rows"])]
    return jobs


def toolchain_pages(runs: list[dict], local: dict | None) -> dict[str, str]:
    """{file name: Markdown} for docs/platform-support/: <slug>.md per toolchain, plus README.md."""
    u = universe()
    pages = {pathlib.PurePosixPath(j["file"]).name: feature_support.toolchain_page(j, u)
             for j in jobs_of(runs, local)}
    pages["README.md"] = folder_readme(runs, local)
    return pages


def notes_body(notes: pathlib.Path = NOTES) -> list[str]:
    """The platform support notes without their title and run marker, headings
    one level down, for the top of the folder README (#226)."""
    if not notes.is_file():
        return []
    lines = notes.read_text(encoding="utf-8").rstrip("\n").split("\n")
    out = []
    for line in lines:
        if line.startswith("# ") or NOTES_RUN.fullmatch(line.strip()):
            continue
        out.append("#" + line if line.startswith("## ") else line)
    while out and not out[0].strip():
        out.pop(0)
    return out


def notes_stale(runs: list[dict], notes: pathlib.Path = NOTES) -> str:
    """Why the notes don't match the report's newest run, or "" (#226)."""
    newest = max((run_number(r) for r in runs), default=0)
    if not newest:
        return ""
    if not notes.is_file():
        return f"{notes.name} is missing"
    m = NOTES_RUN.search(notes.read_text(encoding="utf-8"))
    if not m:
        return f"{notes.name} has no <!-- notes-run: <id> --> marker"
    if int(m.group(1)) != newest:
        return (f"{notes.name} describes run {m.group(1)}, but the newest run in the report is {newest}: "
                "re-read the results and update the notes and their marker")
    return ""


def folder_readme(runs: list[dict], local: dict | None) -> str:
    """docs/platform-support/README.md: what the folder holds and an index of the toolchain pages."""
    ensure_pages(runs, local)
    order = {w: i for i, (w, _, _) in enumerate(WORKFLOWS)}
    entries = []  # (row, run cell)
    if local and local["rows"]:
        for label in sorted(local["rows"]):
            entries.append((local["rows"][label], f"local, {local['date']}" if local["date"] else "local"))
    for r in sorted((r for r in runs if r["rows"]), key=lambda r: order.get(r["workflow"], len(order))):
        for label in sorted(r["rows"]):
            row = r["rows"][label]
            url = (row.get("run") or {}).get("run", {}).get("url") or r["url"]
            what = " · ".join(x for x in (r["workflow"], r["date"]) if x) or "run"
            entries.append((row, f"[{what}]({url})" if url else what))
    run_ids = sorted({r["url"].rstrip("/").rsplit("/", 1)[-1] for r in runs if r.get("url")})
    out = ["# Platform support: toolchain pages", "",
           "<!-- Generated by tools/collect_results.py; don't edit by hand (see CLAUDE.md). -->", "",
           "> One page per platform × compiler × architecture, each listing every documented feature "
           "and example with ✅ passed, ❌ failed, ⏭ skipped (with the reason) or — not run. The "
           "summary, the feature support matrix and the compiler and platform bugs found are on the "
           "entry page, [`../platform-support.md`](../platform-support.md).", "",
           "The goal is every documented feature and every example on every supported platform, "
           "architecture and compiler, so the results show how far each toolchain supports modern C++. "
           "GitHub Actions runs are manual, each with the owner's consent; the local runs are the "
           "owner's machine and the `gcc:16` Docker image.", ""]
    body = notes_body()
    if body:
        out += ["## Platform support notes", "",
                "*From [`notes.md`](notes.md), written by hand from the results and updated with every "
                "full run.*", ""] + body + [""]
    out += ["## Toolchains", "",
           f"{len(entries)} pages. **Tests**: GoogleTest cases passed / total; **snippets**: "
           "compile-checked blocks passed / checked (*report-only*: no reviewed baseline yet, so "
           "snippet failures don't fail that run).", "",
           "| Page | Platform | Compiler | Arch | Tests | Snippets | Run |",
           "|---|---|---|---|---|---|---|"]
    for row, run_cell in entries:
        name = pathlib.PurePosixPath(row["page"]).name
        out.append(f"| [{name.removesuffix('.md')}]({name}) | {os_label(row)} | {compiler_label(row)} "
                   f"| {arch_of(row.get('tests') or row['snippets'])} | {readme_tests(row)} "
                   f"| {readme_snippets(row)} | {run_cell} |")
    out += ["", "## What's in this folder", "",
            "| Path | What | Edited by |", "|---|---|---|",
            "| `<platform>-<arch>-<compiler>.md` | One toolchain's results (names stay the same across runs) "
            "| generated |",
            "| `local-<platform>-<arch>-<compiler>.md` | The same for a local run | generated |",
            "| [`runs/<run-id>/`](runs/) | The `results-*.json` this report uses and each run's `run.json` "
            "(date, commit, inputs, jobs, counts); never logs. Older files are in git history. The report is rebuilt from here after GitHub's retention runs out"
            + (f" (runs: {', '.join(run_ids)})" if run_ids else "") + " | the collector |",
            "| [`local/`](local/) | The local results used, with `local.json` (date, commit) | you, then the collector |",
            "| [`known-bugs.json`](known-bugs.json) | Compiler and platform bugs: upstream link, repo issue, "
            "affected toolchains | by hand, validated |",
            "| [`notes.md`](notes.md) | The platform support notes shown above: a summary of the results "
            "per compiler, platform and architecture, with the run they describe | by hand, checked "
            "against the newest run |",
            "| `README.md` | This index | generated |",
            "", "## Regenerating", "",
            "After a consented run, follow the [`platform-support`](../../.claude/skills/platform-support/SKILL.md) "
            "skill: process every log, capture the findings in the tests, then", "",
            "```bash", "tools/collect_results.py <run-id>…          # the latest run of every platform workflow",
            "tools/collect_results.py <run-id>… --check  # nothing stale", "```", "",
            "It rewrites this folder, the entry page and the README's \"Tested platforms\" table. "
            "Details: [`docs/testing.md`](../testing.md).", ""]
    return "\n".join(out)


def sync_pages(folder: pathlib.Path, pages: dict[str, str], write: bool) -> list[str]:
    """Write the toolchain pages and remove ones no longer produced; return what was stale."""
    stale = []
    for name, text in pages.items():
        f = folder / name
        if not f.is_file() or f.read_text(encoding="utf-8") != text:
            stale.append(name)
            if write:
                folder.mkdir(parents=True, exist_ok=True)
                f.write_text(text, encoding="utf-8")
    for f in sorted(folder.glob("*.md")) if folder.is_dir() else []:
        if f.name not in pages and f.name != NOTES.name:  # notes.md is hand-written
            stale.append(f"{f.name} (no longer produced)")
            if write:
                f.unlink()
    return stale


def archive_local(src: pathlib.Path, dest: pathlib.Path) -> None:
    """Replace the archived local results with src's results-*.json, plus local.json."""
    dest.mkdir(parents=True, exist_ok=True)
    for f in dest.glob("results-*.json"):
        f.unlink()
    stamps = []
    for f in sorted(src.rglob("results-*.json")):
        data = json.loads(f.read_text(encoding="utf-8"))
        stamps += [d.get("generated", "") for d in (data if isinstance(data, list) else [data])]
        write_compact(dest / f.name, data)
    r = subprocess.run(["git", "-C", str(ROOT), "rev-parse", "--short", "HEAD"], capture_output=True, text=True)
    meta = {"schema": SCHEMA, "kind": "archived-local", "date": max(stamps, default="")[:10],
            "commit": r.stdout.strip() if r.returncode == 0 else "",
            "note": "Local runs (the owner's machine and the gcc:16 Docker image); commit is the checkout "
                    "the results were archived from."}
    (dest / "local.json").write_text(json.dumps(meta, indent=1) + "\n", encoding="utf-8")


def load_local(folder: pathlib.Path) -> dict:
    loc = load_folder(folder, "", "")
    meta = folder / "local.json"
    if meta.is_file():
        m = json.loads(meta.read_text(encoding="utf-8"))
        loc["date"], loc["sha"] = m.get("date") or loc["date"], m.get("commit", "")
    return loc


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
    ap.add_argument("--local", type=pathlib.Path,
                    help="folder of results-*.json from local runs; archived in "
                         f"{LOCAL_ARCHIVE.relative_to(ROOT)} (default: that archive)")
    ap.add_argument("--no-local", action="store_true", help="leave the local results out")
    ap.add_argument("--run-url", default="", help="run link for a folder source (default: none)")
    ap.add_argument("--date", default="", help="run date for a folder source (default: newest 'generated')")
    ap.add_argument("--workflow", default="", help="workflow file for a folder source, e.g. linux.yml")
    ap.add_argument("--keep", type=pathlib.Path, help="download run artifacts here instead of a temp dir")
    ap.add_argument("-o", "--output", type=pathlib.Path, default=PAGE, help=f"default: {PAGE.relative_to(ROOT)}")
    ap.add_argument("--summary", action="store_true",
                    help="only the results tables and gaps (for a run's $GITHUB_STEP_SUMMARY)")
    ap.add_argument("--stdout", action="store_true", help="print the page instead of writing it")
    ap.add_argument("--readme", type=pathlib.Path, default=README,
                    help=f"README whose {README_START} block gets the platform table "
                         f"(default: {README.relative_to(ROOT)})")
    ap.add_argument("--no-readme", action="store_true", help="write the page only, leave the README alone")
    ap.add_argument("--archive", type=pathlib.Path, default=ARCHIVE,
                    help=f"archived runs, one folder per run ID (default: {ARCHIVE.relative_to(ROOT)})")
    ap.add_argument("--no-archive", action="store_true",
                    help="download runs that aren't archived without archiving them")
    ap.add_argument("--no-matrix", action="store_true", help="leave the feature support matrix out of the page")
    ap.add_argument("--check", action="store_true",
                    help="write nothing; exit 1 if the pages or the README table differ from these results")
    ap.add_argument("--self-test", action="store_true", help="run the built-in samples and exit")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    if not args.local and not args.no_local and (LOCAL_ARCHIVE / "local.json").is_file():
        args.local = LOCAL_ARCHIVE
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
                runs.append(load_run(s, args.keep / s if args.keep else None, args.archive,
                                     save=not (args.no_archive or args.check)))
            else:
                ap.error(f"{s}: neither a folder nor a run ID")
        supersede(runs)
        if not (args.no_archive or args.check or args.summary or args.stdout):
            ids = {s for s in args.sources if s.isdigit() and not pathlib.Path(s).is_dir()}
            if ids:
                gone = prune_archive(args.archive, runs, ids)
                if gone:
                    print(f"pruned the archive (still in git history): {', '.join(gone)}")
        local = None
        if args.local and not args.no_local:
            if args.local.resolve() != LOCAL_ARCHIVE.resolve() and not args.check and not args.summary \
                    and not args.stdout and not args.no_archive:
                archive_local(args.local, LOCAL_ARCHIVE)
                print(f"archived local results in {LOCAL_ARCHIVE.relative_to(ROOT)}")
                args.local = LOCAL_ARCHIVE
            elif args.local.resolve() == LOCAL_ARCHIVE.resolve() and not (args.check or args.summary
                                                                          or args.stdout or args.no_archive):
                for f in sorted(LOCAL_ARCHIVE.glob("results-*.json")):  # written there by run_tests.sh
                    write_compact(f, json.loads(f.read_text(encoding="utf-8")))
            local = load_local(args.local)
    except SchemaError as e:
        print(f"collect_results.py: schema mismatch: {e}", file=sys.stderr)
        return 1
    if not any(r["rows"] for r in runs) and not (local and local["rows"]) and not any(r["jobs"] for r in runs):
        print("collect_results.py: no results-*.json found in the sources", file=sys.stderr)
        return 2

    if not args.summary and not args.no_matrix:
        assign_pages(runs, local)
    try:
        if args.summary:
            page = summary(runs, local)
        else:
            # The newest result's date, not today's, so --check stays stable on later days.
            newest = max([str(r.get("date") or "")[:10] for r in runs]
                         + [str((local or {}).get("date") or "")[:10]])
            page = render(runs, local, os.environ.get("CPPDOCS_DATE") or newest or dt.date.today().isoformat(),
                          matrix_rows=not args.no_matrix)
    except SchemaError as e:
        print(f"collect_results.py: schema mismatch: {e}", file=sys.stderr)
        return 1
    if args.stdout:
        sys.stdout.write(page)
        return 0
    readme = None
    if not args.summary and not args.no_readme:
        try:
            current = args.readme.read_text(encoding="utf-8")
            updated = update_readme(current, readme_block(runs, local))
            updated = update_block(updated, BADGES_START, BADGES_END, badges_block(runs, local))
            readme = (current, updated)
        except (OSError, SchemaError) as e:
            print(f"collect_results.py: {e}", file=sys.stderr)
            return 2
    pages = toolchain_pages(runs, local) if not args.summary and not args.no_matrix else None
    folder = args.output.parent / "platform-support"
    why = notes_stale(runs) if pages is not None else ""
    if args.check:
        stale = sync_pages(folder, pages, write=False) if pages is not None else []
        if why:
            stale.append(why)
        if args.output.is_file() and args.output.read_text(encoding="utf-8") != page:
            stale.append(str(args.output))
        if readme and readme[0] != readme[1]:
            stale.append(f"{args.readme} ({README_START} or {BADGES_START} block)")
        if readme:
            stale += sync_badges(BADGE_DIR, badges(runs, local), write=False)
        print("out of date: " + ", ".join(stale) if stale else "up to date")
        return 1 if stale else 0
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(page, encoding="utf-8")
    n = sum(len(r["rows"]) for r in runs) + (len(local["rows"]) if local else 0)
    print(f"wrote {args.output}: {n} rows")
    if pages is not None:
        changed = sync_pages(folder, pages, write=True)
        print(f"wrote {len(pages)} toolchain pages in {folder} ({len(changed)} changed or removed)")
        if why:
            print(f"warning: {why}", file=sys.stderr)
    if readme:
        if readme[0] != readme[1]:
            args.readme.write_text(readme[1], encoding="utf-8")
        print(f"{'updated' if readme[0] != readme[1] else 'unchanged'}: {args.readme} platform table and badges")
        changed = sync_badges(BADGE_DIR, badges(runs, local), write=True)
        print(f"badges: {len(changed)} changed or removed in {BADGE_DIR.relative_to(ROOT)}")
    return 0


def self_test() -> int:
    """Render the README table and the page from built-in samples and check the cells."""
    def tests(passed, failed=0, skipped=0, cid="GNU", ver="16.2.0", stdlib="libstdc++", system="Linux"):
        return {"schema": SCHEMA, "kind": "tests", "preset": "gcc", "toolchain": "gcc",
                "platform": {"system": system, "version": "7.0", "arch": "x86_64"},
                "compiler": {"id": cid, "version": ver, "stdlib": stdlib},
                "counts": {"total": passed + failed + skipped, "passed": passed, "failed": failed,
                           "skipped": skipped},
                "failures": [], "skips": [{"reason": "needs x"}] * skipped, "generated": "2026-11-01T00:00:00Z"}

    def snippets(checked, failed=0, new=None, tc="gcc", cid="gcc", ver="g++ (GCC) 16.2.0"):
        base = None if new is None else {"file": f"tools/snippet_baseline/{tc}.txt", "known_failing": failed - new,
                                         "new": new, "fixed": [], "stale": []}
        return {"schema": SCHEMA, "kind": "snippets", "toolchain": tc, "enabled": [],
                "platform": {"system": "Linux", "version": "7.0", "arch": "x86_64"},
                "compiler": {"id": cid, "version": ver},
                "counts": {"total": checked, "checked": checked, "failed": failed, "skipped": 0},
                "failures": [], "skips": [], "baseline": base, "generated": "2026-11-01T00:00:00Z"}

    def job(os_version, t, sn, mode="enforced", steps=None, platform="linux-x86_64-ubuntu-24.04"):
        return {"schema": SCHEMA, "kind": "run", "platform": platform, "os_version": os_version,
                "inputs": {"areas": "^strings$", "tests": True, "snippets": True}, "snippets_mode": mode,
                "steps": steps or {"tests": 0}, "ok": True,
                "run": {"url": "https://example.invalid/runs/1"}, "tests": t, "snippets": sn}

    samples = {
        "results-linux-x86_64-ubuntu-24.04-gcc-16.json":
            job("Ubuntu 24.04.5 LTS x86_64", tests(30), snippets(12, 0, 0)),
        "results-linux-x86_64-ubuntu-24.04-clang-22.json":
            job("Ubuntu 24.04.5 LTS x86_64", tests(28, 2, cid="Clang", ver="22.1.8", stdlib="libc++"),
                snippets(12, 12, tc="clang", cid="clang", ver="Ubuntu clang version 22.1.8"), mode="report-only"),
        "results-windows-2025-x64-msvc.json":
            job("Microsoft Windows Server 2025 Datacenter 10.0.26100", None,
                snippets(12, 0, 0, tc="msvc", cid="msvc", ver="Microsoft (R) C/C++ Optimizing Compiler Version "
                         "19.51.36260 for x64"), mode="report-only", steps={"tests": 2, "snippets": 0},
                platform="windows-2025-x64"),
        "results-windows-2025-x64-UCRT64-gcc-16.json":
            job("MINGW64_NT-10.0-26100 3.6.12, MSYS2 UCRT64", tests(30), snippets(12, 0, 0),
                platform="windows-2025-x64-UCRT64"),
    }
    local_samples = {"results-macos-arm64-clang-22.json": tests(969, 0, 128, cid="Clang", ver="22.1.8",
                                                                stdlib="libc++", system="Darwin"),
                     "results-macos-arm64-clang-22-snippets.json": snippets(691, 19, 0, tc="clang", cid="clang",
                                                                            ver="Homebrew clang version 22.1.8")}
    failures = []

    def expect(what, cond):
        if not cond:
            failures.append(what)

    with tempfile.TemporaryDirectory() as tmp:
        ci_dir, local_dir = pathlib.Path(tmp, "ci"), pathlib.Path(tmp, "local")
        for d, items in ((ci_dir, samples), (local_dir, local_samples)):
            d.mkdir()
            for name, doc in items.items():
                (d / name).write_text(json.dumps(doc), encoding="utf-8")
        run = load_folder(ci_dir, "https://example.invalid/runs/1", "2026-11-02") | {"workflow": "linux.yml"}
        local = load_folder(local_dir, "", "")
        block = readme_block([run], local)
        lines = block.splitlines()
        expect("five rows", sum(1 for l in lines if l.startswith("| ") and not l.startswith("| Platform")) == 5)
        expect("local row first", lines.index(next(l for l in lines if "local, 2026-11-01" in l))
               < lines.index(next(l for l in lines if "example.invalid" in l)))
        expect("green row", "| Ubuntu 24.04 | GCC 16.2.0 | x86_64 | ✅ 30/30 | ✅ 12/12 |" in block)
        expect("failed tests", "❌ 2 failed (28/30)" in block)
        expect("report-only failures", "❌ 0/12 (report-only)" in block)
        expect("clang stdlib", "Clang 22.1.8, libc++" in block)
        expect("missing tests", "| Windows Server 2025 | MSVC 19.51.36260 | x86_64 | ❌ no results "
                                "(tests step exit 2) | ✅ 12/12 (report-only) |" in block)
        expect("msys2", "Windows Server 2025, MSYS2 UCRT64" in block)
        expect("baselined", "✅ 672/691 (19 baselined)" in block)
        expect("skips", "✅ 969/1097 (128 skipped)" in block)
        expect("scope", "cover areas `^strings$`" in block)
        readme = ("# t\n<!-- index:library:start -->\nidx\n<!-- index:library:end -->\n"
                  f"{README_START}\nold\n{README_END}\ntail\n")
        new = update_readme(readme, block)
        expect("block replaced", "old" not in new and block in new)
        expect("index untouched", "<!-- index:library:start -->\nidx\n<!-- index:library:end -->" in new)
        expect("idempotent", update_readme(new, block) == new)
        try:
            update_readme("# no markers\n", block)
            expect("missing markers rejected", False)
        except SchemaError:
            pass
        page = render([run], local, "2026-11-02")
        expect("page tests failure", "failed: no results (tests step exit 2)" in page)
        expect("page scope", "areas `^strings$`" in page)
        expect("readme links the matrix", "(docs/platform-support.md#feature-support-matrix)" in block)
        jobs = jobs_of([run], None)
        slugs = sorted(pathlib.PurePosixPath(r["page"]).name for r in run["rows"].values()) + \
            sorted(pathlib.PurePosixPath(r["page"]).name for r in local["rows"].values())
        expect(f"toolchain file names ({slugs})", slugs == [
            "linux-ubuntu-24.04-x86_64-clang-22-libcxx.md", "linux-ubuntu-24.04-x86_64-gcc-16.md",
            "windows-server-2025-msys2-ucrt64-x86_64-gcc-16.md", "windows-server-2025-x86_64-msvc-19.51.md",
            "local-macos-x86_64-clang-22-libcxx.md"])
        expect("platform table links the toolchain page",
               "| [linux-x86_64-ubuntu-24.04-gcc-16](platform-support/linux-ubuntu-24.04-x86_64-gcc-16.md) |" in page)
        expect("no per-feature detail on the entry page", "<details><summary><code>" not in page
               and "| Feature |" not in page)
        pages = toolchain_pages([run], local)
        expect("one page per toolchain, plus the folder README", len(pages) == 6 and "README.md" in pages)
        idx = pages["README.md"]
        expect("folder README indexes every toolchain page",
               all(f"({name})" in idx for name in pages if name != "README.md")
               and "(../platform-support.md)" in idx and "| [`known-bugs.json`](known-bugs.json) |" in idx)
        g16 = pages["linux-ubuntu-24.04-x86_64-gcc-16.md"]
        expect("toolchain page header", g16.startswith("# GCC 16.2.0 on Ubuntu 24.04, x86_64")
               and "| Areas | `^strings$` |" in g16 and "(../platform-support.md)" in g16)
        expect("report-only noted", "**report-only**" in pages["windows-server-2025-x86_64-msvc-19.51.md"])
        folder = pathlib.Path(tmp, "pages")
        folder.mkdir()
        (folder / "old-toolchain.md").write_text("x", encoding="utf-8")
        changed = sync_pages(folder, pages, write=True)
        expect("stale page removed", not (folder / "old-toolchain.md").exists() and len(changed) == 7)
        expect("pages up to date", sync_pages(folder, pages, write=False) == [])
        larch = pathlib.Path(tmp, "local-archive")
        archive_local(local_dir, larch)
        back_local = load_local(larch)
        expect("local archive", sorted(back_local["rows"]) == sorted(local["rows"])
               and back_local["date"] == "2026-11-01" and (larch / "local.json").is_file())
        msvc = next(j for j in jobs if j["family"] == "MSVC")
        expect("job: MSVC major.minor", msvc["version"] == "19.51" and msvc["report_only"])
        clang = next(j for j in jobs if j["family"] == "Clang")
        expect("job: clang stdlib", clang["version"] == "22" and clang["stdlib"] == "libc++")
        msys = {"run": {"platform": "windows-2025-x64-CLANG64"},
                "tests": {"compiler": {"id": "Clang", "stdlib": "libstdc++"}}}
        ucrt = {"run": {"platform": "windows-2025-x64-UCRT64"},
                "tests": {"compiler": {"id": "Clang", "stdlib": "libstdc++"}}}
        fix_stdlib(msys)
        fix_stdlib(ucrt)
        expect("CLANG64 is libc++ (#190)", msys["tests"]["compiler"]["stdlib"] == "libc++")
        expect("UCRT64 Clang stays libstdc++", ucrt["tests"]["compiler"]["stdlib"] == "libstdc++")
        old = {"url": "https://x/runs/10", "workflow": "all.yml", "rows": {"lin": {}, "msvc": {}},
               "jobs": [("linux.yml / gcc", "failure"), ("windows-msvc.yml / msvc", "success")]}
        new = {"url": "https://x/runs/11", "workflow": "all.yml", "rows": {"lin": {}},
               "jobs": [("linux.yml / gcc", "success"), ("windows-msvc.yml", "skipped")]}
        supersede([old, new])
        expect("supersede: newer row wins", set(old["rows"]) == {"msvc"} and set(new["rows"]) == {"lin"})
        expect("supersede: older job of a re-run workflow dropped",
               old["jobs"] == [("windows-msvc.yml / msvc", "success")])
        expect("bugs table", "### Compiler and platform bugs found" in page)
        bad = pathlib.Path(tmp, "bugs.json")
        bad.write_text('[{"toolchain": "x", "summary": "y", "upstream": [{"label": "z", "url": "http://no"}], '
                       '"issues": [1], "match": "q"}]', encoding="utf-8")
        try:
            load_bugs(bad)
            expect("bugs: non-https upstream rejected", False)
        except SchemaError:
            pass
        expect("bugs file loads", all("match" in b for b in load_bugs()))
        expect("matrix in the page, before the gaps",
               0 < page.find("## Feature support matrix") < page.find("## Known gaps per toolchain"))
        # The archive: a run read back from docs/platform-support/runs/<id>/ matches the download.
        arch = pathlib.Path(tmp, "archive")
        rec = run_record("7", {"html_url": "https://example.invalid/runs/7", "run_started_at": "2026-11-02T01:02:03Z",
                               "path": ".github/workflows/linux.yml", "head_sha": "a" * 40, "conclusion": "success"},
                         [{"name": "gcc", "conclusion": "success"}], run["rows"])
        archive_run(arch / "7", ci_dir, rec)
        back = load_run("7", None, arch)
        expect("archive: rows", sorted(back["rows"]) == sorted(run["rows"]))
        expect("archive: metadata", back["url"].endswith("/7") and back["sha"] == "a" * 7
               and back["workflow"] == "linux.yml" and back["jobs"] == [("gcc", "success")])
        old_dir = arch / "10"
        old_dir.mkdir()
        (old_dir / "run.json").write_text("{}", encoding="utf-8")
        for lab in ("lin", "msvc"):
            (old_dir / f"results-{lab}.json").write_text('{\n "a": 1\n}\n', encoding="utf-8")
        gone = prune_archive(arch, [old], {"10"})
        expect("prune: superseded job file removed, the rest compact (#214)",
               gone == ["10/results-lin.json", "7/"] and not (arch / "7").exists()
               and (old_dir / "results-msvc.json").read_text(encoding="utf-8") == '{"a":1}\n')
        nf = pathlib.Path(tmp, "notes.md")
        nf.write_text("# Platform support notes\n\n<!-- notes-run: 11 -->\n\n## In short\nAll good.\n",
                      encoding="utf-8")
        expect("notes: body without title or marker, headings down a level (#226)",
               notes_body(nf) == ["### In short", "All good."])
        expect("notes: current", notes_stale([new], nf) == "")
        good = {"tests": tests(30)}
        bad = {"tests": tests(28, failed=2)}
        ro = {"tests": tests(30), "snippets": {**snippets(10, failed=3), "mode": "report-only"}}
        bc = badge_counts([{"rows": {"linux-a": good, "linux-b": bad, "macos-a": ro}}],
                          {"rows": {"macos-arm64-gcc-15": good}})
        expect("badges: counts per platform, local apart (#230)",
               bc == {"linux": (2, 1), "macos": (1, 0), "local": (1, 0)})
        svgs = badges([{"rows": {"linux-a": good, "linux-b": bad, "macos-a": ro}}], None)
        expect("badges: failing is red, passing green with a checkmark",
               RED in svgs["linux.svg"] and "&#10007; 1 of 2 failing" in svgs["linux.svg"]
               and GREEN in svgs["macos.svg"] and "&#10003; 1 toolchain" in svgs["macos.svg"])
        blk = badges_block([{"rows": {"linux-a": good}}], None)
        expect("badges: README block links the notes",
               "(docs/platform-support/badges/linux.svg)](docs/platform-support/notes.md)" in blk)
        expect("notes: stale when a newer run is in the report",
               "newest run in the report is 12" in notes_stale([{"url": "https://x/runs/12"}], nf))
        expect("archive: inputs and digest", rec["inputs"] == [{"areas": "^strings$", "tests": True, "snippets": True}]
               and rec["digest"]["linux-x86_64-ubuntu-24.04-gcc-16"]["tests"]["passed"] == 30)
    failures += feature_support.self_test()
    for f in failures:
        print(f"FAIL: {f}", file=sys.stderr)
    print(f"collect_results self-test: {'FAILED' if failures else 'OK'} ({len(failures)} failure(s))")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
