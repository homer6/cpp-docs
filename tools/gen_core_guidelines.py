#!/usr/bin/env python3
"""Generate the index of every C++ Core Guidelines rule.

Reads the pinned official text, modules/CppCoreGuidelines/CppCoreGuidelines.md,
and writes docs/guidelines/core-guidelines/: a README listing every section,
and one page per section listing every rule (and every other numbered entry)
in the official order, each with its official ID and title and a link to the
official text. Where one of our guideline pages cites a rule, the entry links
to it.

The rule bodies are not copied: the Guidelines' licence permits copies for
personal or internal business use only, so the index cites and links instead.

Usage:
    python3 tools/gen_core_guidelines.py              rewrite the index
    python3 tools/gen_core_guidelines.py --check      exit 1 if it is stale or a rule is missing
    python3 tools/gen_core_guidelines.py --upstream   exit 1 if the submodule pin is behind upstream master
    python3 tools/gen_core_guidelines.py --self-test  run the built-in samples
"""
from __future__ import annotations

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
MODULE = ROOT / "modules" / "CppCoreGuidelines"
SOURCE = MODULE / "CppCoreGuidelines.md"
GUIDELINES = ROOT / "docs" / "guidelines"
OUT = GUIDELINES / "core-guidelines"
EXAMPLES = OUT / "examples"            # our examples, one page per section (#235)
TESTS = ROOT / "tests" / "guidelines"  # our unit tests for the rules
OFFICIAL = "https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines"
UPSTREAM = "https://github.com/isocpp/CppCoreGuidelines"

HEADING = re.compile(r"^(#{1,3}) +(?:<a name=\"([^\"]+)\"></a>)?\s*(.*?)\s*$")
EDITION = re.compile(r"^([A-Z][a-z]+ [0-9]{1,2}, 20[0-9]{2})\s*$", re.M)
MDLINK = re.compile(r"\[([^\]]*)\]\([^)]*\)")
ENTRY_ID = re.compile(r"^([A-Za-z]+(?:\.[A-Za-z]+)*\.[0-9A-Za-z?-]+|[A-Za-z]+):")
# A profile rule is an anchored bullet ("* <a name=..></a>Type.1: ..."), and its
# anchored sub-points are numbered under it ("  1. <a name=..></a>Don't ...").
BULLET_RULE = re.compile(r"^[*-] +<a name=\"([^\"]+)\"></a>\s*([A-Z][A-Za-z]*\.[0-9]+:.*?)\s*$")
SUB_POINT = re.compile(r"^ +[0-9]+\. +<a name=\"([^\"]+)\"></a>\s*(.*?)\s*$")
# A section's rule summary: "* [F.1: title](#anchor)".
SUMMARY = re.compile(r"^\* \[([A-Za-z]+(?:\.[A-Za-z]+)*\.[0-9]+[a-z]?: .*)\]\(#([^)]+)\)\s*$")


def parse(text: str) -> list[dict]:
    """Sections (level 1), each with subsections (level 2) holding entries.

    Entries are the anchored ### headings, the profiles' anchored rule bullets
    (with their anchored sub-points), and rules a section's summary lists that
    have no text of their own yet (upstream links them to a placeholder).
    """
    sections: list[dict] = []
    summaries: list[tuple[dict, str, str]] = []
    fence = False
    for line in text.splitlines():
        if line.lstrip().startswith(("```", "~~~")):
            fence = not fence
            continue
        if fence:
            continue
        m = HEADING.match(line)
        if m:
            level, anchor, title = len(m.group(1)), m.group(2), MDLINK.sub(r"\1", m.group(3))
            if level == 1:
                sections.append({"anchor": anchor, "title": title,
                                 "subs": [{"anchor": None, "title": None, "entries": []}]})
            elif not sections:
                continue
            elif level == 2:
                sections[-1]["subs"].append({"anchor": anchor, "title": title, "entries": []})
            elif anchor:
                sections[-1]["subs"][-1]["entries"].append({"anchor": anchor, "title": title})
            continue
        if not sections:
            continue
        sub = sections[-1]["subs"][-1]
        if m := BULLET_RULE.match(line):
            title = MDLINK.sub(r"\1", m.group(2)).rstrip(":")
            sub["entries"].append({"anchor": m.group(1), "title": title, "points": []})
        elif (m := SUB_POINT.match(line)) and sub["entries"] and "points" in sub["entries"][-1]:
            point = MDLINK.sub(r"\1", m.group(2)).split(";")[0].rstrip(".")
            sub["entries"][-1]["points"].append({"anchor": m.group(1), "title": point})
        elif m := SUMMARY.match(line):
            summaries.append((sub, MDLINK.sub(r"\1", m.group(1)), m.group(2)))
    known = {e["anchor"] for s in sections for sub in s["subs"] for e in sub["entries"]}
    stubs: set[str] = set()  # placeholders share one dead link (#rt-???), so key on the title
    after = None  # the entry the summary lists just before, to keep the official order
    for sub, title, anchor in summaries:
        if anchor in known:
            after = next(e for s in sections for x in s["subs"] for e in x["entries"] if e["anchor"] == anchor)
            continue
        if title in stubs:
            continue
        stubs.add(title)
        stub = {"anchor": None, "title": title, "stub": True}
        home = next((x["entries"] for s in sections for x in s["subs"]
                     if any(e is after for e in x["entries"])), None)
        if home is None:
            sub["entries"].append(stub)
        else:
            home.insert(next(k for k, e in enumerate(home) if e is after) + 1, stub)
        after = stub
    return sections


def count_source_entries(text: str) -> int:
    """What the index must hold, counted independently of parse(): every anchor
    on a ### heading or a profile rule bullet, plus each summary-listed rule
    whose link goes to no such anchor."""
    anchors, linked, fence = set(), [], False
    for line in text.splitlines():
        if line.lstrip().startswith(("```", "~~~")):
            fence = not fence
            continue
        if fence:
            continue
        if m := re.match(r"^### +<a name=\"([^\"]+)\"></a>", line):
            anchors.add(m.group(1))
        elif m := BULLET_RULE.match(line):
            anchors.add(m.group(1))
        elif m := SUMMARY.match(line):
            linked.append((m.group(2), m.group(1)))
    return len(anchors) + len({title for anchor, title in linked if anchor not in anchors})


def slug(section: dict) -> str:
    base = section["anchor"] or section["title"]
    base = re.sub(r"^s-", "", base.lower())
    return re.sub(r"[^a-z0-9]+", "-", base).strip("-")


def link(anchor: str | None) -> str:
    return f"{OFFICIAL}#{anchor}" if anchor else OFFICIAL


RULE_ID = re.compile(r"[A-Za-z]+(?:\.[A-Za-z]+)*\.[0-9]+[a-z]?")
# Test comments use the same IDs as the index, including named ones (GSL.ptr).
TEST_COMMENT = re.compile(r"^// ([A-Za-z]+(?:\.[A-Za-z]+)*\.[0-9A-Za-z?]+):", re.M)
NO_EXAMPLE = re.compile(r"^\*No example: (.+?)\*\s*$", re.M)


def github_slug(heading: str) -> str:
    h = MDLINK.sub(r"\1", heading)
    h = re.sub(r"[`*]", "", h).strip().lower()
    h = re.sub(r"[^\w\- ]", "", h)
    return h.replace(" ", "-")


def rule_id(title: str) -> str | None:
    m = ENTRY_ID.match(title)
    return m.group(1) if m and "." in m.group(1) else None


def coverage(examples: dict[str, str], tests: dict[str, str]) -> dict[str, dict]:
    """Per rule ID: its example (page, anchor), its "no example" reason, its test files.

    examples maps an examples page name to its text, tests a test file's
    repo-relative path to its text."""
    cov: dict[str, dict] = {}
    for name, text in sorted(examples.items()):
        parts = re.split(r"^## +(.+?)\s*$", text, flags=re.M)
        for heading, body in zip(parts[1::2], parts[2::2]):
            rid = rule_id(heading)
            if not rid:
                continue
            c = cov.setdefault(rid, {"tests": []})
            if m := NO_EXAMPLE.search(body):
                c["none"] = m.group(1)
            elif "```cpp" in body:
                c["example"] = (name, github_slug(heading))
    for path, text in sorted(tests.items()):
        for rid in dict.fromkeys(TEST_COMMENT.findall(text)):
            cov.setdefault(rid, {"tests": []})["tests"].append(path)
    return cov


def read_coverage() -> dict[str, dict]:
    examples = {p.name: p.read_text(encoding="utf-8") for p in sorted(EXAMPLES.glob("*.md"))} if EXAMPLES.exists() else {}
    tests = {p.relative_to(ROOT).as_posix(): p.read_text(encoding="utf-8")
             for p in sorted(TESTS.rglob("*_test.cpp"))} if TESTS.exists() else {}
    return coverage(examples, tests)


def our_pages() -> dict[str, str]:
    skip = {"README.md", "sources.md"}  # the section index and the change log only mention IDs
    return {p.name: p.read_text(encoding="utf-8") for p in sorted(GUIDELINES.glob("*.md")) if p.name not in skip}


def cited_by(title: str, pages: dict[str, str]) -> list[str]:
    m = ENTRY_ID.match(title)
    if not m or "." not in m.group(1):
        return []
    rid = re.compile(r"(?<![\w.])" + re.escape(m.group(1)) + r"(?!\w|\.\w)")
    return [name for name, text in pages.items() if rid.search(text)]


def entry_lines(e: dict, pages: dict[str, str], section_anchor: str | None,
                cov: dict[str, dict] | None = None) -> list[str]:
    if e.get("stub"):
        line = f"- {e['title']} — listed in the [official summary]({link(section_anchor)}), no text yet"
    else:
        line = f"- [{e['title']}]({link(e['anchor'])})"
    refs = cited_by(e["title"], pages)
    if refs:
        line += " · ours: " + ", ".join(f"[{n.removesuffix('.md')}](../{n})" for n in refs)
    c = (cov or {}).get(rule_id(e["title"]) or "", {})
    if "example" in c:
        line += f" · [example](examples/{c['example'][0]}#{c['example'][1]})"
    elif "none" in c:
        line += f" · no example: {c['none']}"
    if c.get("tests"):
        line += " · " + ", ".join(f"[test](../../../{t})" for t in c["tests"][:1])
    return [line] + [f"  - [{p['title']}]({link(p['anchor'])})" for p in e.get("points", [])]


def stamp(commit: str, edition: str) -> str:
    return (f"<!-- Generated by tools/gen_core_guidelines.py from modules/CppCoreGuidelines "
            f"(commit {commit[:7]}, {edition} edition); don't edit by hand. -->")


def render(sections: list[dict], commit: str, edition: str, pages: dict[str, str],
           cov: dict[str, dict] | None = None) -> dict[str, str]:
    cov = cov or {}
    files: dict[str, str] = {}
    rows = []
    total = 0
    for s in sections:
        entries = sum(len(sub["entries"]) for sub in s["subs"])
        total += entries
        name = f"{slug(s)}.md"
        rows.append(f"| [{s['title']}]({name}) | {entries} | [official]({link(s['anchor'])}) |")
        out = [f"# {s['title']}", "", stamp(commit, edition), "",
               f"Every entry in this section of the official C++ Core Guidelines, in their order. "
               f"Each link opens the official text. \"ours\" links the pages of this repo that cite the rule; "
               f"\"example\" and \"test\" link our own example and unit test for it.",
               "", f"[Official section]({link(s['anchor'])}) · [All sections](README.md)", ""]
        if not entries:
            out += ["This section has no numbered entries; read it in the official text.", ""]
        for sub in s["subs"]:
            if sub["title"] is not None:
                out += [f"## {sub['title']}", "", f"[Official text]({link(sub['anchor'] or s['anchor'])})", ""]
            if sub["entries"]:
                anchor = sub["anchor"] or s["anchor"]
                out += [l for e in sub["entries"] for l in entry_lines(e, pages, anchor, cov)] + [""]
        files[name] = "\n".join(out).rstrip() + "\n"
    ids = [rid for s in sections for sub in s["subs"] for e in sub["entries"] if (rid := rule_id(e["title"]))]
    with_example = sum(1 for r in ids if "example" in cov.get(r, {}))
    no_example = sum(1 for r in ids if "none" in cov.get(r, {}) and "example" not in cov.get(r, {}))
    with_test = sum(1 for r in ids if cov.get(r, {}).get("tests"))
    readme = [
        "# C++ Core Guidelines — every rule", "", stamp(commit, edition), "",
        f"Every section and every rule of the official [C++ Core Guidelines]({OFFICIAL}) "
        f"(Stroustrup, Sutter), {edition} edition, upstream commit "
        f"[`{commit[:7]}`]({UPSTREAM}/commit/{commit}): **{total} entries**, nothing left out. "
        "Each page lists its section's rules with their official IDs and titles and links "
        "to the official text.", "",
        "The rule text itself isn't copied here: the Guidelines' licence permits copies for "
        "personal or internal business use only. Read a rule at its link, or in the submodule "
        "(`git submodule update --init`, then search "
        "`modules/CppCoreGuidelines/CppCoreGuidelines.md` for the rule ID).", "",
        "Worked examples for some rules are in [core-guidelines.md](../core-guidelines.md); "
        "more are welcome. Regenerate this index with `python3 tools/gen_core_guidelines.py` "
        "after moving the submodule.", "",
        "## Examples and tests", "",
        "Our own examples are in [`examples/`](examples/), one page per section, and the unit "
        "tests that check them in [`tests/guidelines/`](../../../tests/guidelines/) (#235). "
        "Each section page links them, rule by rule, where they exist.", "",
        "| Numbered rules | With an example | No example (not about code) | Still to do | With a unit test |",
        "|---|---|---|---|---|",
        f"| {len(ids)} | {with_example} | {no_example} | {len(ids) - with_example - no_example} | {with_test} |", "",
        "## Sections", "",
        "| Section | Entries | Official |", "|---|---|---|", *rows,
    ]
    files["README.md"] = "\n".join(readme) + "\n"
    return files


def commit_of_module() -> str:
    return subprocess.run(["git", "-C", str(MODULE), "rev-parse", "HEAD"],
                          capture_output=True, text=True, check=True).stdout.strip()


def build() -> tuple[dict[str, str], int, int]:
    if not SOURCE.exists():
        sys.exit("modules/CppCoreGuidelines is not checked out: run git submodule update --init")
    text = SOURCE.read_text(encoding="utf-8")
    m = EDITION.search(text)
    edition = m.group(1) if m else "undated"
    sections = parse(text)
    cov = read_coverage()
    files = render(sections, commit_of_module(), edition, our_pages(), cov)
    listed = sum(len(sub["entries"]) for s in sections for sub in s["subs"])
    known = {rid for s in sections for sub in s["subs"] for e in sub["entries"] if (rid := rule_id(e["title"]))}
    unknown = sorted(set(cov) - known)
    return files, listed, count_source_entries(text), unknown


def check(files: dict[str, str], listed: int, expected: int, unknown: list[str]) -> int:
    bad = [f"{rid}: an examples heading or test comment names a rule the Guidelines don't have" for rid in unknown]
    if listed != expected:
        bad.append(f"the index lists {listed} entries but the official text has {expected}")
    for name, body in files.items():
        p = OUT / name
        if not p.exists() or p.read_text(encoding="utf-8") != body:
            bad.append(f"{p.relative_to(ROOT)} is stale or missing")
    for p in sorted(OUT.glob("*.md")) if OUT.exists() else []:
        if p.name not in files:
            bad.append(f"{p.relative_to(ROOT)} is not generated any more")
    for b in bad:
        print(b)
    if bad:
        print("run python3 tools/gen_core_guidelines.py")
        return 1
    print(f"Core Guidelines index is up to date ({listed} entries)")
    return 0


def upstream() -> int:
    pin = commit_of_module()
    out = subprocess.run(["git", "ls-remote", UPSTREAM + ".git", "refs/heads/master"],
                         capture_output=True, text=True, check=True).stdout.split()
    head = out[0] if out else ""
    if head == pin:
        print(f"modules/CppCoreGuidelines is at upstream master ({pin[:7]})")
        return 0
    print(f"modules/CppCoreGuidelines is at {pin[:7]}, upstream master is {head[:7]}: "
          "refresh it (refresh-guidelines skill) and regenerate the index")
    return 1


SAMPLE = """# <a name="main"></a>C++ Core Guidelines

June 14, 2026

# <a name="s-philosophy"></a>P: Philosophy

### <a name="rp-direct"></a>P.1: Express ideas directly in code

```cpp
### <a name="fake"></a>X.1: inside a fence, not a rule
```

# <a name="s-class"></a>C: Classes and class hierarchies

## <a name="ss-ctor"></a>C.ctor: Constructors

### <a name="rc-dtor"></a>C.30: Define a destructor if needed

## C.other: Other default operation rules

### <a name="rc-x"></a>SL.str.1: Use `std::string`

### <a name="St-tidy"></a>Tools: [Clang-tidy](https://example.com)

# <a name="s-templates"></a>T: Templates

* [T.1: Use templates](#rt-raise)
* [T.30: Use concept negation sparingly](#rt-???)
* [T.31: Use concept disjunction sparingly](#rt-???)

### <a name="rt-raise"></a>T.1: Use templates

# <a name="s-profile"></a>Pro: Profiles

## <a name="ss-type"></a>Pro.safety: Type-safety profile

* <a name="pro-type-avoidcasts"></a>Type.1: [Avoid casts](#res-casts):
  1. <a name="pro-type-reinterpretcast"></a>Don't use `reinterpret_cast`; A strict version.
* <a name="pro-type-downcast"></a>Type.2: Don't use `static_cast` to downcast:

# Bibliography
"""


def self_test() -> int:
    bad = []
    sections = parse(SAMPLE)
    entries = [e for s in sections for sub in s["subs"] for e in sub["entries"]]
    if [e["title"].split(":")[0] for e in entries] != ["P.1", "C.30", "SL.str.1", "Tools", "T.1", "T.30", "T.31", "Type.1", "Type.2"]:
        bad.append(f"entries: {[e['title'] for e in entries]}")
    if count_source_entries(SAMPLE) != len(entries):
        bad.append("fenced heading counted, or an entry dropped")
    if [slug(s) for s in sections] != ["main", "philosophy", "class", "templates", "profile", "bibliography"]:
        bad.append(f"slugs: {[slug(s) for s in sections]}")
    if entries[3]["title"] != "Tools: Clang-tidy" or entries[7]["title"] != "Type.1: Avoid casts":
        bad.append("link inside a title not flattened")
    if entries[7].get("points") != [{"anchor": "pro-type-reinterpretcast", "title": "Don't use `reinterpret_cast`"}]:
        bad.append(f"profile sub-points: {entries[7].get('points')}")
    if not entries[5].get("stub") or entries[4].get("stub"):
        bad.append("summary-only rule not marked, or a rule with text marked")
    pages = {"a.md": "See SL.str.1 and C.300.", "b.md": "C.30, then P.10."}
    if cited_by("C.30: x", pages) != ["b.md"] or cited_by("P.1: x", pages) != [] \
            or cited_by("SL.str.1: x", pages) != ["a.md"]:
        bad.append("rule ID citations matched wrongly")
    files = render(sections, "0123456789abcdef", "June 14, 2026", pages)
    if "**9 entries**" not in files["README.md"] or "## C.other: Other default operation rules" not in files["class.md"]:
        bad.append("render: README total or anchorless subsection missing")
    if "no numbered entries" not in files["bibliography.md"]:
        bad.append("render: empty section not explained")
    if "T.30: Use concept negation sparingly — listed in the [official summary]" not in files["templates.md"]:
        bad.append("render: summary-only rule missing")
    if "  - [Don't use `reinterpret_cast`]" not in files["profile.md"]:
        bad.append("render: profile sub-point missing")
    ex = {"class.md": "# C\n\n## C.30: Define a destructor if needed\n\nText.\n\n```cpp\nstruct S {};\n```\n\n"
                      "## SL.str.1: Use `std::string`\n\n*No example: shown elsewhere.*\n\n## C.31: no code yet\n"}
    ts = {"tests/guidelines/class_test.cpp": "// C.30: the destructor runs\nTEST(A, B) {}\n// C.30: again\n",
          "tests/guidelines/gsl_test.cpp": "// Checks claims made by x.\n// GSL.ptr: owner is an alias\n"}
    cov = coverage(ex, ts)
    if cov.get("C.30", {}).get("example") != ("class.md", "c30-define-a-destructor-if-needed") \
            or cov["C.30"]["tests"] != ["tests/guidelines/class_test.cpp"]:
        bad.append(f"coverage C.30: {cov.get('C.30')}")
    if cov.get("GSL.ptr", {}).get("tests") != ["tests/guidelines/gsl_test.cpp"] or "Checks" in cov:
        bad.append(f"named test ID: {cov.get('GSL.ptr')}")
    if cov.get("SL.str.1", {}).get("none") != "shown elsewhere." or "example" in cov.get("C.31", {"x": 1}):
        bad.append(f"coverage no-example / empty: {cov.get('SL.str.1')}, {cov.get('C.31')}")
    files = render(sections, "0123456789abcdef", "June 14, 2026", pages, cov)
    if "· [example](examples/class.md#c30-define-a-destructor-if-needed) · [test](../../../tests/guidelines/class_test.cpp)" not in files["class.md"]:
        bad.append("render: example/test links missing")
    if "· no example: shown elsewhere." not in files["class.md"]:
        bad.append("render: no-example reason missing")
    if "| 8 | 1 | 1 | 6 | 1 |" not in files["README.md"]:
        bad.append("render: coverage totals wrong")
    for b in bad:
        print("FAIL", b)
    print(f"gen_core_guidelines self-test: {'OK' if not bad else 'FAILED'} ({len(bad)} failure(s))")
    return 1 if bad else 0


def main() -> int:
    args = sys.argv[1:]
    if "--self-test" in args:
        return self_test()
    if "--upstream" in args:
        return upstream()
    files, listed, expected, unknown = build()
    if "--check" in args:
        return check(files, listed, expected, unknown)
    for rid in unknown:
        print(f"warning: {rid} is not an official rule (examples heading or test comment)")
    if listed != expected:
        sys.exit(f"parsed {listed} entries but the official text has {expected}: fix the parser")
    OUT.mkdir(parents=True, exist_ok=True)
    for p in OUT.glob("*.md"):
        if p.name not in files:
            p.unlink()
    for name, body in files.items():
        (OUT / name).write_text(body, encoding="utf-8")
    print(f"wrote {len(files)} pages to {OUT.relative_to(ROOT)} ({listed} entries)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
