#!/usr/bin/env python3
"""Feature support: every documented feature and example x every toolchain.

Used by tools/collect_results.py, which loads the results (consented GitHub
Actions runs archived under docs/platform-support/runs/, local results under
docs/platform-support/local/; schema "cpp-docs/results/1") and calls
summary() for the "Feature support matrix" section of docs/platform-support.md
(rows = areas, columns = compilers) and toolchain_page() for each
docs/platform-support/<slug>.md (every area, page, feature and example on one
toolchain). It isn't a command of its own (`python3 tools/feature_support.py`
runs its self-test).

The rows come from this checkout, not from the results:

  * features: every GoogleTest case (`TEST(Suite, Name)` in
    tests/<area>/<path>_test.cpp checks docs/examples/<area>/<path>.md; the
    file's first line names the page), labelled by the one-line comment above
    the TEST that names the page section (`// "Section": claim`);
  * examples: every ```cpp block in docs/, with the stable ID
    `<file>#<heading-slug>/<n>` that tools/check_snippets.py gives it (its own
    extract() is used, so the two never disagree).

A job's result says which tests and snippets failed or were skipped. When it
also lists every test or block with its status (the optional "results" field
that tools/ctest_to_json.py and check_snippets.py --json write), that list is
used. Otherwise "passed" is inferred: in the job's scope (its `areas` input,
its snippet `paths`) and neither failed nor skipped. The inference is guarded:
if the job's counts don't match the size of its scope in this checkout (a
target didn't build, or the tree changed since the run), its cells are
unknown (❔), never invented passes. TESTs inside a preprocessor conditional
(#if at file scope) may not exist on every toolchain; they count either way
and are unknown when the counts can't tell.
"""
from __future__ import annotations

import collections
import dataclasses
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import check_snippets  # noqa: E402  (reuse the checker's fence parser and IDs)

LABEL_LIMIT = 80
REASON_LIMIT = 140

PASS, FAIL, SKIP = "passed", "failed", "skipped"
SYMBOL = {PASS: "✅", FAIL: "❌", SKIP: "⏭"}
NOT_RUN, UNKNOWN = "—", "❔"

# Column order: compiler families, then version.
FAMILY_ORDER = ["GCC", "Clang", "Apple Clang", "MSVC", "clang-cl"]
STDLIB_ORDER = {"libstdc++": 0, "libc++": 1, "MS STL": 2}


# --- the universe: features and examples in this checkout ---------------------

@dataclasses.dataclass(frozen=True)
class Feature:
    name: str         # ctest name, Suite.Name
    area: str         # ctest label: the top folder under tests/ ("support", "release" aren't pages)
    page: str         # docs-relative page, e.g. examples/strings/string.md ("" if none)
    label: str        # the comment above the TEST
    conditional: bool = False  # inside a file-scope #if: may not exist on every toolchain


@dataclasses.dataclass(frozen=True)
class Example:
    id: str           # <file>#<heading-slug>/<n>
    area: str         # docs/examples/<area>/..., "guidelines", or "docs"
    page: str         # docs-relative page
    section: str      # heading slug


TEST_RE = re.compile(r"^\s*TEST(?:_F)?\(\s*(\w+)\s*,\s*(\w+)\s*\)")
HEADER_RE = re.compile(r"^// Checks claims made by (docs/\S+?\.md)")
PP_OPEN = re.compile(r"^\s*#\s*if(?:n?def)?\b")
PP_CLOSE = re.compile(r"^\s*#\s*endif\b")


def page_for(test_file: pathlib.Path, root: pathlib.Path) -> str:
    """The page a test file checks: its header comment, else the path mapping."""
    first = test_file.read_text(encoding="utf-8").split("\n", 1)[0]
    if m := HEADER_RE.match(first):
        return m[1][len("docs/"):]
    rel = test_file.relative_to(root / "tests")
    if rel.parts[0] == "support":
        return ""
    stem = rel.name[:-len("_test.cpp")]
    page = rel.parent / ("README.md" if stem == "readme" else stem + ".md")
    return "examples/" + page.as_posix()


def parse_tests(path: pathlib.Path, area: str, page: str) -> list[Feature]:
    out, blocks, block = [], [], []
    depth = 0
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("//"):
            block.append(line[2:].strip())
            continue
        if block:
            blocks.append(" ".join(block))
            block = []
        if PP_OPEN.match(line):
            depth += 1
        elif PP_CLOSE.match(line):
            depth = max(0, depth - 1)
        m = TEST_RE.match(line)
        if not m:
            continue
        cands = [b for b in blocks if not b.startswith("Checks claims made by")]
        quoted = [b for b in cands if b.startswith('"')]
        label = (quoted or cands or [""])[-1]
        out.append(Feature(f"{m[1]}.{m[2]}", area, page, label, conditional=depth > 0))
        blocks = []
    return out


def cmake_tests(root: pathlib.Path) -> list[Feature]:
    """Plain add_test() tests outside GoogleTest (e.g. release_notes_exist, label "release")."""
    out = []
    for f in [root / "CMakeLists.txt", *sorted((root / "tests").rglob("*.cmake")),
              *sorted((root / "tests").rglob("CMakeLists.txt"))]:
        if not f.is_file():
            continue
        text = f.read_text(encoding="utf-8")
        for m in re.finditer(r"add_test\(\s*NAME\s+([\w.-]+)", text):
            lab = re.search(r"set_tests_properties\(\s*" + re.escape(m[1]) + r"\s+PROPERTIES\s+LABELS\s+(\w+)", text)
            out.append(Feature(m[1], lab[1] if lab else "", "", ""))
    return out


def test_universe(root: pathlib.Path = ROOT) -> list[Feature]:
    out = []
    for f in sorted((root / "tests").rglob("*_test.cpp")):
        area = f.relative_to(root / "tests").parts[0]
        out += parse_tests(f, area, page_for(f, root))
    return out + cmake_tests(root)


def example_area(page: str) -> str:
    parts = page.split("/")
    if parts[0] == "examples" and len(parts) > 2:
        return parts[1]
    return parts[0] if len(parts) > 1 else "docs"


def snippet_files(paths, root: pathlib.Path = ROOT) -> list[pathlib.Path]:
    """The Markdown files check_snippets.py reads for these paths (same walk)."""
    files = []
    for p in paths:
        p = root / p if not pathlib.Path(p).is_absolute() else pathlib.Path(p)
        files += sorted(p.rglob("*.md")) if p.is_dir() else ([p] if p.is_file() else [])
    return files


def example_universe(paths=("docs",), root: pathlib.Path = ROOT) -> list[Example]:
    out = []
    for f in snippet_files(paths, root):
        page = f.relative_to(root / "docs").as_posix() if (root / "docs") in f.parents else f.name
        for b in check_snippets.extract(f):
            ident = f.relative_to(root).as_posix() + "#" + b.ident
            out.append(Example(ident, example_area(page), page, b.ident.rsplit("/", 1)[0]))
    return out


def github_slug(heading: str) -> str:
    """GitHub's heading anchor. Unlike check_snippets.slug() (frozen, since snippet IDs use
    it), it keeps underscores inside words: `not_fn` -> not_fn."""
    h = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", heading)
    h = re.sub(r"(?<!\w)_+([^_]+?)_+(?!\w)", r"\1", h)
    h = re.sub(r"[`*]", "", h).strip().lower()
    return re.sub(r"[^\w\- ]", "", h).replace(" ", "-")


def heading_slugs(page: pathlib.Path) -> dict[str, str]:
    """{snippet-ID heading slug: GitHub anchor} for a page's headings, numbered alike."""
    seen, gseen, out, fence = {}, {}, {}, None
    for line in page.read_text(encoding="utf-8").splitlines():
        if m := check_snippets.FENCE.match(line):
            if fence is None:
                fence = m["fence"][0]
            elif m["fence"][0] == fence and not m["info"].strip():
                fence = None
            continue
        if fence is None and (h := check_snippets.HEADING.match(line)):
            base, gbase = check_snippets.slug(h[1]), github_slug(h[1])
            n, g = seen.get(base, 0), gseen.get(gbase, 0)
            seen[base], gseen[gbase] = n + 1, g + 1
            out[base if n == 0 else f"{base}-{n}"] = gbase if g == 0 else f"{gbase}-{g}"
    return out


# --- one job's results, matched against the universe -------------------------

@dataclasses.dataclass
class Outcome:
    """A job's status per feature or example. statuses None: unknown for the whole job."""
    statuses: dict[str, str | None] | None   # key -> PASS/FAIL/SKIP, None = unknown for that key
    reasons: dict[str, str]                  # key -> skip reason
    note: str = ""                           # why it's unknown or partial


def _listed(doc: dict, key: str, universe: set[str]) -> Outcome | None:
    """Use the job's per-item list ("results"), if it has one."""
    items = doc.get("results")
    if not isinstance(items, list):
        return None
    statuses: dict[str, str | None] = {}
    extra = 0
    for it in items:
        k, s = (it or {}).get(key), (it or {}).get("status")
        if s not in (PASS, FAIL, SKIP) or not isinstance(k, str):
            continue
        if k in universe:
            statuses[k] = s
        else:
            extra += 1
    reasons = {s.get(key, s.get("name", "")): s["reason"] for s in doc["skips"]}
    note = f"{extra} result(s) for tests or blocks no longer in this checkout ignored" if extra else ""
    return Outcome(statuses, reasons, note)


def test_outcome(doc: dict | None, run: dict | None, universe: list[Feature]) -> Outcome | None:
    """None when the job didn't run the tests."""
    if not doc:
        return None
    names = {f.name for f in universe}
    if (o := _listed(doc, "name", names)) is not None:
        return o
    areas = ((run or {}).get("inputs") or {}).get("areas") or ""
    try:
        rx = re.compile(areas) if areas else None
    except re.error:
        return Outcome(None, {}, f"areas regex {areas!r} doesn't compile")
    scope = [f for f in universe if rx is None or rx.search(f.area)]
    fixed = {f.name for f in scope if not f.conditional}
    maybe = {f.name for f in scope if f.conditional}
    failed = {x.get("name", "") for x in doc["failures"]}
    skipped = {x.get("name", ""): x["reason"] for x in doc["skips"]}
    total = doc["counts"]["total"]
    stray = (failed | set(skipped)) - fixed - maybe
    if stray or not len(fixed) <= total <= len(fixed) + len(maybe):
        size = f"{len(fixed)}" + (f"–{len(fixed) + len(maybe)}" if maybe else "")
        why = (f"{len(stray)} failed/skipped test(s) not in this checkout" if stray
               else f"the job ran {total} tests; this checkout has {size} in its scope")
        return Outcome(None, {}, why + "; the per-test results can't be matched")
    statuses: dict[str, str | None] = {n: PASS for n in fixed}
    known_maybe = maybe & (failed | set(skipped))
    if total == len(fixed) + len(maybe):
        statuses.update({n: PASS for n in maybe})
    elif total == len(fixed) + len(known_maybe):
        pass  # the other conditional tests weren't built on this toolchain: not run
    else:
        statuses.update({n: None for n in maybe - known_maybe})
    statuses.update({n: FAIL for n in failed})
    statuses.update({n: SKIP for n in skipped})
    return Outcome(statuses, skipped)


def example_outcome(doc: dict | None, universe_for) -> Outcome | None:
    """universe_for(paths) -> list[Example] for the paths the job checked."""
    if not doc:
        return None
    paths = doc.get("paths") or ["docs"]
    scope = {e.id for e in universe_for(tuple(paths))}
    if (o := _listed(doc, "id", scope)) is not None:
        return o
    failed = {x.get("id", "") for x in doc["failures"]}
    skipped = {x.get("id", ""): x["reason"] for x in doc["skips"]}
    stray = (failed | set(skipped)) - scope
    total = doc["counts"]["total"]
    if stray or total != len(scope):
        why = (f"{len(stray)} failed/skipped block ID(s) not in this checkout" if stray
               else f"the job checked {total} blocks; this checkout has {len(scope)} in "
                    f"{', '.join(paths)}")
        return Outcome(None, {}, why + "; the per-block results can't be matched")
    statuses: dict[str, str | None] = {i: PASS for i in scope}
    statuses.update({i: FAIL for i in failed})
    statuses.update({i: SKIP for i in skipped})
    return Outcome(statuses, skipped)


# --- columns: jobs with the same compiler and identical results collapse ------

@dataclasses.dataclass
class Column:
    family: str
    version: str
    stdlib: str
    report_only: bool
    jobs: list[dict]
    tests: Outcome | None
    examples: Outcome | None
    local: bool = False
    suffix: str = ""

    @property
    def header(self) -> str:
        parts = [self.family, self.version + self.suffix]
        if self.family == "Clang" and self.stdlib:
            parts.append(self.stdlib)
        if self.local:
            parts.append("*local*")
        return "<br>".join(parts) + ("†" if self.report_only else "")

    @property
    def name(self) -> str:
        n = f"{self.family} {self.version}{self.suffix}"
        n += f", {self.stdlib}" if self.family == "Clang" and self.stdlib else ""
        return n + (" (local)" if self.local else "") + ("†" if self.report_only else "")

    def test(self, key: str) -> tuple[str, str]:
        return _state(self.tests, key)

    def example(self, key: str) -> tuple[str, str]:
        return _state(self.examples, key)


def _state(o: Outcome | None, key: str) -> tuple[str, str]:
    """(state, skip reason) with state PASS/FAIL/SKIP, "unknown" or "not run"."""
    if o is None:
        return "not run", ""
    if o.statuses is None:
        return "unknown", ""
    if key not in o.statuses:
        return "not run", ""
    s = o.statuses[key]
    return (s, o.reasons.get(key, "") if s == SKIP else "") if s else ("unknown", "")


def _signature(o: Outcome | None):
    if o is None:
        return None
    if o.statuses is None:
        return ("unknown", o.note)
    return tuple(sorted((k, v or "?", o.reasons.get(k, "")) for k, v in o.statuses.items()))


def _version_key(v: str) -> tuple:
    return tuple(int(x) for x in re.findall(r"\d+", v)) or (0,)


def build_columns(jobs: list[dict], tests_u: list[Feature], universe_for) -> list[Column]:
    """jobs: dicts with family, version, stdlib, report_only, tests, snippets, run (see collect_results)."""
    groups: dict[tuple, list[tuple[dict, Outcome | None, Outcome | None]]] = collections.defaultdict(list)
    for j in jobs:
        t = test_outcome(j.get("tests"), j.get("run"), tests_u)
        e = example_outcome(j.get("snippets"), universe_for)
        if t is None and e is None:
            continue
        groups[(j["family"], j["version"], j["stdlib"], j["report_only"], j["local"])].append((j, t, e))
    cols = []
    for (fam, ver, std, ro, loc), members in groups.items():
        parts: dict[tuple, Column] = {}
        for j, t, e in members:
            sig = (_signature(t), _signature(e))
            if sig in parts:
                parts[sig].jobs.append(j)
            else:
                parts[sig] = Column(fam, ver, std, ro, [j], t, e, local=loc)
        split = list(parts.values())
        if len(split) > 1:
            for i, c in enumerate(split):
                c.suffix = " " + "abcdefghijklmnopqrstuvwxyz"[i] if i < 26 else f" {i}"
        cols += split
    fam_rank = {f: i for i, f in enumerate(FAMILY_ORDER)}
    cols.sort(key=lambda c: (fam_rank.get(c.family, len(fam_rank)), c.family, STDLIB_ORDER.get(c.stdlib, 9),
                             _version_key(c.version), c.local, c.report_only, c.suffix))
    return cols


# --- rendering ------------------------------------------------------------------

def esc(text: str) -> str:
    return (re.sub(r"\s+", " ", text).strip().replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace("|", "\\|"))


def shorten(text: str, limit: int) -> str:
    text = re.sub(r"\s+", " ", text).strip()
    return text if len(text) <= limit else text[:limit - 1].rstrip() + "…"


def summary_cell(states: list[str]) -> str:
    """An area or a page on one toolchain: ✅ all, 🟡 n skipped, ❌ n failed."""
    known = [s for s in states if s in (PASS, FAIL, SKIP)]
    if not known:
        return UNKNOWN if "unknown" in states else NOT_RUN
    fails, skips = known.count(FAIL), known.count(SKIP)
    gap = " ❔" if "unknown" in states else ""
    if fails:
        return f"❌ {fails}" + gap
    if skips:
        return f"🟡 {skips}" + gap
    return "✅" + gap


def item_cell(state: tuple[str, str]) -> str:
    """One feature on one toolchain, with the skip reason inline."""
    s, reason = state
    if s == SKIP:
        return "⏭ " + esc(shorten(reason or "(no reason given)", REASON_LIMIT))
    return {PASS: "✅", FAIL: "❌", "unknown": UNKNOWN}.get(s, NOT_RUN)


def section_cell(states: list[tuple[str, str]]) -> str:
    """The blocks of one page section on one toolchain."""
    known = [(s, r) for s, r in states if s in (PASS, FAIL, SKIP)]
    if not known:
        return UNKNOWN if any(s == "unknown" for s, _ in states) else NOT_RUN
    fails = sum(s == FAIL for s, _ in known)
    passes = sum(s == PASS for s, _ in known)
    reasons = sorted({shorten(r or "(no reason given)", REASON_LIMIT) for s, r in known if s == SKIP})
    skips = len(known) - fails - passes
    out = []
    if fails:
        out.append(f"❌ {fails} of {len(states)}")
    elif passes:
        out.append("✅" if passes == len(states) else f"✅ {passes}")
    if skips:
        out.append(f"⏭ {skips if skips > 1 or passes or fails else ''}".rstrip()
                   + " (" + "; ".join(esc(r) for r in reasons) + ")")
    return " · ".join(out)


def area_title(area: str) -> str:
    return {"guidelines": "Guidelines (snippets only)", "docs": "Other docs pages"}.get(area, area)


def anchor(area: str) -> str:
    return github_slug(area_title(area))


def by_area(tests_u: list[Feature], examples_u: list[Example]):
    """({area: {page: ([features], [examples])}}, area order). Only features with a page."""
    out: dict[str, dict[str, tuple[list[Feature], list[Example]]]] = {}
    for f in tests_u:
        if f.page:
            out.setdefault(f.area, {}).setdefault(f.page, ([], []))[0].append(f)
    for e in examples_u:
        out.setdefault(e.area, {}).setdefault(e.page, ([], []))[1].append(e)
    return out, sorted(out, key=lambda a: (a in ("guidelines", "docs"), a == "docs", a))


class Universe:
    """The checkout's features and examples, with the per-paths example lists cached."""

    def __init__(self, root: pathlib.Path = ROOT, tests_u=None, examples_u=None):
        self.root = root
        self.tests = test_universe(root) if tests_u is None else tests_u
        self.examples = example_universe(("docs",), root) if examples_u is None else examples_u
        self.cache: dict[tuple, list[Example]] = {}
        self.areas, self.order = by_area(self.tests, self.examples)

    def examples_for(self, paths: tuple) -> list[Example]:
        if paths == ("docs",):
            return self.examples
        if paths not in self.cache:
            self.cache[paths] = example_universe(paths, self.root)
        return self.cache[paths]


# --- the entry page: the summary matrix ----------------------------------------

def summary(jobs: list[dict], u: Universe) -> str:
    """The "Feature support matrix" section of docs/platform-support.md (## level).

    Each job dict has a "file" (its per-toolchain page, relative to docs/); a column that
    stands for one job links to it, a collapsed one to the "Columns" list.
    """
    cols = build_columns(jobs, u.tests, u.examples_for)
    features = [f for f in u.tests if f.page]
    ci_cols = [c for c in cols if any(not j["local"] for j in c.jobs)]
    covered_t = {f.name for f in features for c in ci_cols if c.test(f.name)[0] in (PASS, FAIL, SKIP)}
    covered_e = {e.id for e in u.examples for c in ci_cols if c.example(e.id)[0] in (PASS, FAIL, SKIP)}
    ci_areas = sorted({f.area for f in features if f.name in covered_t})
    out = ["## Feature support matrix", "",
           "Every feature this reference documents and every example it shows, per toolchain: "
           "**measured results, not vendor claims.** A *feature* is one GoogleTest case in `tests/`, "
           "which checks one claim of one page section; an *example* is one compile-checked "
           "```` ```cpp ```` block in `docs/`. This summary has one row per area; **each toolchain's "
           "own page** in [`platform-support/`](platform-support/) (linked from the column headers, "
           "the tables above and the list below) has every area, page, feature and example with "
           "its result and skip reason.", "",
           "- **Cells:** ✅ everything that ran passed · 🟡 *n* skipped, none failed · ❌ *n* failed · "
           f"{NOT_RUN} not run (outside the run's areas) · {UNKNOWN} unknown (the job's results "
           "couldn't be matched to this checkout).",
           "- **Columns:** jobs with the same compiler, version and standard library and identical "
           "results share a column (the [Columns](#columns) list names every toolchain page behind "
           "it); when results differ the column splits (*a*, *b*, …). *local* marks the owner's local "
           "runs; † marks report-only snippet results (no reviewed snippet baseline yet, #159).",
           "- **Rows come from this checkout** (test names and section comments, snippet IDs). A job "
           "that lists every test's status (`results`) is matched exactly; otherwise *passed* is "
           "inferred only when the job's counts match, and its cells are ❔ when they don't.", ""]
    total_t, total_e = len(features), len(u.examples)
    if ci_cols:
        partial = len(covered_t) < total_t or len(covered_e) < total_e
        out += [f"**GitHub Actions coverage: {len(covered_t)} of {total_t} features"
                f" and {len(covered_e)} of {total_e} examples** ran on at least one GitHub Actions "
                "toolchain" + (f" (areas: {', '.join(f'`{a}`' for a in ci_areas)})" if ci_areas else "")
                + (". **Coverage is partial:** the other areas show "
                   f"{NOT_RUN} on those columns until a consented every-area run (empty `areas`) is "
                   "collected." if partial else "."), ""]
    else:
        out += ["No GitHub Actions results in this refresh.", ""]

    def head(c: Column) -> str:
        target = c.jobs[0]["file"] if len(c.jobs) == 1 else "#columns"
        return f"[{c.header}]({target})"
    for kind, title in (("tests", "Features (GoogleTest cases) per area:"),
                        ("examples", "Examples (compile-checked snippets) per area:")):
        out += [title, "", "| Area | " + ("Features" if kind == "tests" else "Examples") + " | "
                + " | ".join(head(c) for c in cols) + " |", "|---|---:|" + "---|" * len(cols)]
        for area in u.order:
            pages = u.areas[area]
            if kind == "tests":
                keys, get = [f.name for fs, _ in pages.values() for f in fs], Column.test
            else:
                keys, get = [e.id for _, es in pages.values() for e in es], Column.example
            if keys:
                out.append(f"| {area_title(area)} | {len(keys)} | "
                           + " | ".join(summary_cell([get(c, k)[0] for k in keys]) for c in cols) + " |")
        out.append("")
    out += ["### Columns", "", "| Column | Toolchain pages |", "|---|---|"]
    for c in cols:
        pages = "<br>".join(f"[{esc(j['title'])}]({j['file']})" for j in sorted(c.jobs, key=lambda j: j["file"]))
        out.append(f"| {c.name} | {pages} |")
    return "\n".join(out) + "\n"


# --- one page per toolchain -------------------------------------------------------

def counts(o: Outcome | None, keys: list[str]) -> dict[str, int]:
    c = collections.Counter(_state(o, k)[0] for k in keys)
    return {k: c.get(k, 0) for k in (PASS, FAIL, SKIP, "not run", "unknown")}


def toolchain_page(job: dict, u: Universe, entry: str = "../platform-support.md") -> str:
    """docs/platform-support/<slug>.md: one platform x arch x compiler, every area/page/feature.

    job: the collect_results job dict (title, header rows in "facts", tests, snippets, run).
    """
    t = test_outcome(job.get("tests"), job.get("run"), u.tests)
    e = example_outcome(job.get("snippets"), u.examples_for)
    up = "../"
    out = [f"# {job['title']}", "",
           "<!-- Generated by tools/collect_results.py; don't edit by hand. "
           "Refresh it only from runs the owner consented to (see CLAUDE.md). -->", "",
           f"Every feature and example of this reference on one toolchain. Back to "
           f"[platform support]({entry}) (all toolchains, the summary matrix and the known gaps).", "",
           "| | |", "|---|---|"]
    out += [f"| {k} | {v} |" for k, v in job["facts"]]
    notes = [o.note for o in (t, e) if o is not None and o.note]
    if notes:
        out.append("| Matching | " + esc("; ".join(notes)) + " |")
    out += ["", "## Totals", "", "| | Passed | Failed | Skipped | Not run | Unknown |",
            "|---|---:|---:|---:|---:|---:|"]
    all_t = [f.name for f in u.tests if f.page]
    all_e = [x.id for x in u.examples]
    for label, o, keys in (("Features", t, all_t), ("Examples", e, all_e)):
        c = counts(o, keys)
        out.append(f"| {label} ({len(keys)}) | {c[PASS]} | {c[FAIL]} | {c[SKIP]} | {c['not run']} | {c['unknown']} |")
    out += ["", "Cells: ✅ passed · ❌ failed · ⏭ skipped (reason given) · "
            f"{NOT_RUN} not run · {UNKNOWN} unknown (the results couldn't be matched to this checkout)."
            + (" Snippet results are **report-only** here: no reviewed snippet baseline yet (#159)."
               if job.get("report_only") else ""), ""]

    def ran(area):
        pages = u.areas[area]
        return any(_state(t, f.name)[0] != "not run" for fs, _ in pages.values() for f in fs) or \
            any(_state(e, x.id)[0] != "not run" for _, es in pages.values() for x in es)
    ran_areas = [a for a in u.order if ran(a)]
    idle = [a for a in u.order if a not in ran_areas]
    out += ["## Areas", "", "| Area | Features | Examples |", "|---|---|---|"]
    for area in u.order:
        pages = u.areas[area]
        ft = summary_cell([_state(t, f.name)[0] for fs, _ in pages.values() for f in fs]) \
            if any(fs for fs, _ in pages.values()) else ""
        ex = summary_cell([_state(e, x.id)[0] for _, es in pages.values() for x in es]) \
            if any(es for _, es in pages.values()) else ""
        name = f"[{area_title(area)}](#{anchor(area)})" if area in ran_areas else area_title(area)
        out.append(f"| {name} | {ft} | {ex} |")
    out.append("")
    if idle:
        out += [f"Not run in this job (outside its areas, so not listed below): "
                + ", ".join(area_title(a) for a in idle) + ".", ""]
    for area in ran_areas:
        pages = u.areas[area]
        n_t = sum(len(fs) for fs, _ in pages.values())
        n_e = sum(len(es) for _, es in pages.values())
        allstates = [_state(t, f.name)[0] for fs, _ in pages.values() for f in fs] + \
            [_state(e, x.id)[0] for _, es in pages.values() for x in es]
        out += [f"## {area_title(area)}", "",
                f"<details><summary>{len(pages)} pages · {n_t} features · {n_e} examples: "
                f"{summary_cell(allstates)}</summary>", ""]
        for page in sorted(pages):
            fs, es = pages[page]
            file = u.root / "docs" / page
            anchors = heading_slugs(file) if file.is_file() else {}
            out += [f"**[{esc(page)}]({up}{page})**", ""]
            if fs:
                out += ["| Feature | Test | Result |", "|---|---|---|"]
                out += [f"| {feature_label(f, up + page, anchors)} | `{f.name}` | {item_cell(_state(t, f.name))} |"
                        for f in fs]
                out.append("")
            if es:
                sections: dict[str, list[Example]] = {}
                for x in es:
                    sections.setdefault(x.section, []).append(x)
                out += ["| Examples in section | Blocks | Result |", "|---|---:|---|"]
                for sec, blocks in sections.items():
                    name = f"[{anchors[sec]}]({up}{page}#{anchors[sec]})" if sec in anchors else f"`{sec}`"
                    out.append(f"| {name} | {len(blocks)} | {section_cell([_state(e, x.id) for x in blocks])} |")
                out.append("")
        out += ["</details>", ""]
    return "\n".join(out).rstrip() + "\n"


def feature_label(f: Feature, page: str, anchors: dict[str, str]) -> str:
    """The TEST's section comment; the section links to its anchor when the page has it."""
    text = f.label or f.name
    m = re.match(r'"([^"]+)"\s*:?\s*(.*)$', text)
    if not m:
        return esc(shorten(text, LABEL_LIMIT))
    section, claim = m[1], shorten(m[2], max(20, LABEL_LIMIT - len(m[1])))
    sl = github_slug(section)
    head = f"[{esc(section)}]({page}#{sl})" if sl in anchors.values() else f"**{esc(section)}**"
    return head + (f": {esc(claim)}" if claim else "")


# --- self-test ------------------------------------------------------------------

def self_test() -> list[str]:
    """Built-in samples: pass, fail, skip, unknown, conditional tests, column collapsing."""
    failures = []

    def expect(what, cond):
        if not cond:
            failures.append(f"feature_support: {what}")

    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        root = pathlib.Path(tmp)
        (root / "tests" / "strings").mkdir(parents=True)
        (root / "docs" / "examples" / "strings").mkdir(parents=True)
        (root / "tests" / "strings" / "string_test.cpp").write_text(
            "// Checks claims made by docs/examples/strings/string.md.\n#include <string>\n\n"
            '// "Examples": size() counts chars.\nTEST(Str, Size) {\n}\n\n'
            '// "Gotchas": helper below.\ntemplate <class T> void h();\n// a helper comment\n'
            "TEST(Str, Find) {\n#if X\n#endif\n}\n\n"
            '// "Examples": skipped one.\nTEST(Str, Skip) {}\n'
            '#if !defined(__APPLE__)\n// "Gotchas": not on Apple.\nTEST(Str, NotApple) {}\n#endif\n',
            encoding="utf-8")
        (root / "docs" / "examples" / "strings" / "string.md").write_text(
            "# std::string\n\n## Examples\n\n```cpp\nint a;\n```\n\n```cpp nocompile\nx\n```\n\n"
            "## Gotchas\n\n```cpp\nint b;\n```\n", encoding="utf-8")
        tu = test_universe(root)
        expect("four tests", [f.name for f in tu] == ["Str.Size", "Str.Find", "Str.Skip", "Str.NotApple"])
        expect("label from the comment", tu[0].label == '"Examples": size() counts chars.')
        expect("quoted comment wins over a helper comment", tu[1].label == '"Gotchas": helper below.')
        expect("page from the header", tu[0].page == "examples/strings/string.md")
        expect("conditional test", tu[3].conditional and not tu[1].conditional)
        expect("GitHub anchors keep underscores", github_slug("`not_fn` (C++17)") == "not_fn-c17")
        eu = example_universe(("docs",), root)
        expect("three examples", [e.id for e in eu] == ["docs/examples/strings/string.md#examples/1",
                                                         "docs/examples/strings/string.md#examples/2",
                                                         "docs/examples/strings/string.md#gotchas/1"])

        def tests(total, failed=(), skipped=(), results=None):
            d = {"counts": {"total": total}, "failures": [{"name": n} for n in failed],
                 "skips": [{"name": n, "reason": "needs __cpp_lib_x"} for n in skipped]}
            if results is not None:
                d["results"] = results
            return d

        def snips(total, failed=(), skipped=()):
            return {"counts": {"total": total}, "paths": ["docs/examples/strings"],
                    "failures": [{"id": i} for i in failed],
                    "skips": [{"id": i, "reason": "nocompile"} for i in skipped]}

        run = {"inputs": {"areas": "^strings$"}}
        e1, e2, e3 = (e.id for e in eu)

        def job(fam, ver, where, t, s, std="", ro=False, local=False):
            return {"family": fam, "version": ver, "stdlib": std, "report_only": ro, "where": where,
                    "compiler": f"{fam} {ver}", "local": local, "tests": t, "snippets": s, "run": run}
        jobs = [
            job("GCC", "16", "Linux x86_64", tests(4, skipped=["Str.Skip"]), snips(3, skipped=[e2])),
            job("GCC", "16", "macOS arm64", tests(4, skipped=["Str.Skip"]), snips(3, skipped=[e2])),
            job("GCC", "15", "Linux", tests(4, failed=["Str.Find"]), snips(3, failed=[e3], skipped=[e2])),
            job("Apple Clang", "17", "macOS", tests(3), snips(3, skipped=[e2]), ro=True),   # NotApple compiled out
            job("MSVC", "19.44", "Windows", tests(9), None, ro=True),                       # counts don't match
            job("Clang", "22", "Linux", tests(2, results=[{"name": "Str.Size", "status": "passed"},
                                                          {"name": "Str.Gone", "status": "passed"}]),
                None, std="libc++"),
            job("Clang", "21", "FreeBSD", tests(4), None, std="libc++"),
            job("Clang", "21", "Linux", tests(4, failed=["Str.Size"]), None, std="libc++"),
        ]
        cols = build_columns(jobs, tu, lambda p: example_universe(p, root))
        names = [c.name for c in cols]
        expect(f"columns collapse and split ({names})",
               names == ["GCC 15", "GCC 16", "Clang 21 a, libc++", "Clang 21 b, libc++", "Clang 22, libc++",
                         "Apple Clang 17†", "MSVC 19.44†"])
        g16 = cols[1]
        expect("collapsed column lists both jobs", len(g16.jobs) == 2)
        expect("pass", g16.test("Str.Size") == (PASS, ""))
        expect("skip with reason", g16.test("Str.Skip") == (SKIP, "needs __cpp_lib_x"))
        expect("fail", cols[0].test("Str.Find")[0] == FAIL)
        expect("snippet fail", cols[0].example(e3)[0] == FAIL)
        apple = cols[5]
        expect("conditional test not run where it wasn't built", apple.test("Str.NotApple")[0] == "not run")
        expect("unconditional test passes on Apple", apple.test("Str.Find")[0] == PASS)
        expect("unknown when counts don't match", cols[6].test("Str.Size")[0] == "unknown"
               and "ran 9 tests" in cols[6].tests.note)
        expect("per-test list wins", cols[4].test("Str.Size")[0] == PASS and cols[4].test("Str.Find")[0] == "not run"
               and "1 result" in cols[4].tests.note)
        u = Universe(root, tu, eu)
        for i, j in enumerate(jobs):
            j["file"], j["title"] = f"platform-support/t{i}.md", f"T{i}"
            j["facts"] = [("Compiler", j["compiler"])]
        page = summary(jobs, u)
        expect("summary row", "| strings | 4 |" in page)
        expect("failed summary", "❌ 1" in page)
        expect("skipped summary", "🟡 1" in page)
        expect("unknown cell", UNKNOWN in page)
        expect("single-job column links its page", "[GCC<br>15](platform-support/t2.md)" in page)
        expect("collapsed column links the list", "[GCC<br>16](#columns)" in page
               and "[T0](platform-support/t0.md)<br>[T1](platform-support/t1.md)" in page)
        expect("report-only mark", "Apple Clang<br>17†" in page)
        expect("coverage line", "4 of 4 features" in page)
        tp = toolchain_page(jobs[2], u)                                    # GCC 15: a failure, a skip
        expect("toolchain page header", tp.startswith("# T2") and "| Compiler | GCC 15 |" in tp)
        expect("toolchain page links back", "(../platform-support.md)" in tp)
        expect("toolchain section link",
               "[Examples](../examples/strings/string.md#examples): size() counts chars." in tp)
        expect("toolchain fail", "| `Str.Find` | ❌ |" in tp)
        expect("toolchain totals", "| Features (4) | 3 | 1 | 0 | 0 | 0 |" in tp)
        expect("toolchain examples", "| [gotchas](../examples/strings/string.md#gotchas) | 1 | ❌ 1 of 1 |" in tp
               and "✅ 1 · ⏭ 1 (nocompile)" in tp)
        tp0 = toolchain_page(jobs[0], u)
        expect("skip reason inline", "| `Str.Skip` | ⏭ needs __cpp_lib_x |" in tp0)
        tp_apple = toolchain_page(jobs[3], u)
        expect("compiled-out test is not run", "| `Str.NotApple` | — |" in tp_apple)
        tp_msvc = toolchain_page(jobs[4], u)
        expect("unknown job", "| `Str.Size` | ❔ |" in tp_msvc and "ran 9 tests" in tp_msvc)
        idle = dict(jobs[0], tests=tests(0), run={"inputs": {"areas": "^nothing$"}}, snippets=None)
        tp_idle = toolchain_page(idle, u)
        expect("areas not run are named, not listed", "Not run in this job" in tp_idle and "Str.Size" not in tp_idle)
    return failures


if __name__ == "__main__":
    fails = self_test()
    for f in fails:
        print(f"FAIL: {f}", file=sys.stderr)
    print(f"feature_support self-test: {'FAILED' if fails else 'OK'} ({len(fails)} failure(s))")
    sys.exit(1 if fails else 0)
