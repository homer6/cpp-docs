#!/usr/bin/env python3
"""Check GitHub Actions workflows against the repo's manual-only policy.

Every file in .github/workflows/*.yml (and *.yaml) must:

  * have workflow_dispatch as its trigger. The owner's rule: no workflow
    ever runs automatically, so push, pull_request, schedule, workflow_run,
    release and every other event are rejected. The one addition allowed is
    workflow_call next to workflow_dispatch, for a platform workflow that a
    manually dispatched caller (all.yml) runs;
  * set top-level `permissions:` to a map with `contents: read`, grant no
    write scope at the top level, and have no job that grants write
    (`write-all`, or any scope set to `write`);
  * pin every `uses:` (step actions and reusable workflows) to a full
    40-character commit SHA, with the version in a trailing comment
    (`uses: actions/checkout@<sha>  # v4.2.2`). Local `./` paths are allowed.

Exit status: 0 when every workflow passes (or there are none), 1 when any
problem is found, 2 on a usage error. Each problem is printed as
`<file>: <message>`.

The YAML reader is a small, strict parser for the block-style subset that
workflow files use (maps, lists, plain and quoted scalars, `|`/`>` block
scalars, flow `[...]`/`{...}`). It needs no third-party package (PyYAML isn't
guaranteed here) and keeps keys as written, so `on:` stays the string "on"
rather than YAML 1.1's boolean True. Anything it doesn't understand (anchors,
aliases, tags, tabs, duplicate keys, odd indentation) is reported as a
problem, so an unreadable workflow fails the check instead of slipping past.

    python3 tools/check_workflows.py                 # the repo's workflows
    python3 tools/check_workflows.py path/to/a.yml   # specific files
    python3 tools/check_workflows.py --self-test     # built-in good/bad samples
"""

from __future__ import annotations

import argparse
import contextlib
import io
import json
import re
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SHA_RE = re.compile(r"^[0-9a-fA-F]{40}$")
ALLOWED_TRIGGER = "workflow_dispatch"
# Allowed only next to workflow_dispatch: a reusable workflow that a manually
# dispatched caller (all.yml) runs with `uses: ./.github/workflows/<file>`.
CALL_TRIGGER = "workflow_call"


# --------------------------------------------------------------------------
# Minimal YAML reader
# --------------------------------------------------------------------------


class YamlError(Exception):
    def __init__(self, lineno: int, message: str) -> None:
        super().__init__(f"line {lineno}: {message}")


def _strip_comment(text: str) -> str:
    """Drop a trailing `# comment` that sits outside quotes."""
    quote = None
    for i, ch in enumerate(text):
        if quote:
            if ch == quote:
                quote = None
        elif ch in "\"'":
            quote = ch
        elif ch == "#" and (i == 0 or text[i - 1] in " \t"):
            return text[:i].rstrip()
    return text.rstrip()


def _find_colon(text: str) -> int:
    """Index of the mapping colon (`:` followed by space or end), or -1."""
    quote = None
    depth = 0
    for i, ch in enumerate(text):
        if quote:
            if ch == quote:
                quote = None
            continue
        if ch in "\"'" and (i == 0 or depth == 0 and text[:i].strip() == ""):
            quote = ch
        elif ch in "[{":
            depth += 1
        elif ch in "]}":
            depth -= 1
        elif ch == ":" and depth == 0 and (i + 1 == len(text) or text[i + 1] in " \t"):
            return i
    return -1


def _unquote(text: str, lineno: int) -> str:
    if text.startswith('"'):
        if len(text) < 2 or not text.endswith('"'):
            raise YamlError(lineno, f"unterminated double-quoted string: {text}")
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            return text[1:-1]
    if text.startswith("'"):
        if len(text) < 2 or not text.endswith("'"):
            raise YamlError(lineno, f"unterminated single-quoted string: {text}")
        return text[1:-1].replace("''", "'")
    return text


def _scalar(text: str, lineno: int):
    text = text.strip()
    if not text:
        return None
    if text[0] in "&*!":
        raise YamlError(lineno, f"anchors, aliases and tags are not supported: {text}")
    if text[0] in "[{":
        value, rest = _flow(text, 0, lineno)
        if text[rest:].strip():
            raise YamlError(lineno, f"unexpected text after flow value: {text[rest:]}")
        return value
    if text in ("~", "null", "Null", "NULL"):
        return None
    return _unquote(text, lineno)


def _flow(text: str, i: int, lineno: int):
    """Parse a flow value starting at text[i]; return (value, next index)."""

    def skip(j: int) -> int:
        while j < len(text) and text[j] in " \t":
            j += 1
        return j

    def atom(j: int, stops: str):
        j = skip(j)
        if j < len(text) and text[j] in "[{":
            return _flow(text, j, lineno)
        if j < len(text) and text[j] in "\"'":
            q = text[j]
            k = j + 1
            while k < len(text):
                if text[k] == q:
                    if q == "'" and k + 1 < len(text) and text[k + 1] == "'":
                        k += 2
                        continue
                    if q == '"' and text[k - 1] == "\\":
                        k += 1
                        continue
                    break
                k += 1
            return _unquote(text[j : k + 1], lineno), k + 1
        k = j
        while k < len(text) and text[k] not in stops:
            if text[k] == ":" and ":" in stops and (k + 1 == len(text) or text[k + 1] in " ,]}"):
                break
            k += 1
        return _scalar(text[j:k], lineno), k

    i = skip(i)
    opener = text[i]
    closer = "]" if opener == "[" else "}"
    result: list | dict = [] if opener == "[" else {}
    i = skip(i + 1)
    while True:
        if i >= len(text):
            raise YamlError(lineno, f"unterminated flow collection: {text}")
        if text[i] == closer:
            return result, i + 1
        if opener == "[":
            value, i = atom(i, ",]")
            result.append(value)
        else:
            key, i = atom(i, ":,}")
            i = skip(i)
            value = None
            if i < len(text) and text[i] == ":":
                value, i = atom(i + 1, ",}")
            if key in result:
                raise YamlError(lineno, f"duplicate key: {key}")
            result[key] = value
        i = skip(i)
        if i < len(text) and text[i] == ",":
            i = skip(i + 1)
        elif i < len(text) and text[i] != closer:
            raise YamlError(lineno, f"expected ',' or '{closer}' in flow collection: {text}")


class _Reader:
    def __init__(self, source: str) -> None:
        self.lines: list[tuple[int, str, int]] = []  # (indent, raw text, lineno)
        for n, raw in enumerate(source.splitlines(), start=1):
            body = raw.lstrip(" ")
            if body.startswith("\t"):
                raise YamlError(n, "tab used for indentation")
            self.lines.append((len(raw) - len(body), body.rstrip(), n))

    def structural(self, i: int) -> int:
        """Skip blank lines, comment lines and document markers."""
        while i < len(self.lines):
            indent, text, _ = self.lines[i]
            stripped = _strip_comment(text)
            if stripped and not (indent == 0 and stripped in ("---", "...")) and not stripped.startswith("%"):
                return i
            i += 1
        return i

    @staticmethod
    def is_item(text: str) -> bool:
        return text == "-" or text.startswith("- ")

    def node(self, i: int, min_indent: int):
        i = self.structural(i)
        if i >= len(self.lines) or self.lines[i][0] < min_indent:
            return None, i
        indent, text, lineno = self.lines[i]
        text = _strip_comment(text)
        if self.is_item(text):
            return self.sequence(i, indent)
        if _find_colon(text) >= 0 and text[0] not in "[{":
            return self.mapping(i, indent)
        return self.inline(text, i, indent, lineno)

    def inline(self, text: str, i: int, owner_indent: int, lineno: int):
        """A scalar or flow value, folding continuation lines deeper than its owner."""
        parts = [text]
        i += 1
        while True:
            j = self.structural(i)
            if j >= len(self.lines) or self.lines[j][0] <= owner_indent:
                break
            parts.append(_strip_comment(self.lines[j][1]))
            i = j + 1
        return _scalar(" ".join(p for p in parts if p), lineno), i

    def block_scalar(self, i: int, owner_indent: int, folded: bool):
        parts = []
        while i < len(self.lines):
            indent, text, _ = self.lines[i]
            if text and indent <= owner_indent:
                break
            parts.append(text)
            i += 1
        return (" " if folded else "\n").join(parts).strip() + "\n", i

    def sequence(self, i: int, indent: int):
        items = []
        while True:
            i = self.structural(i)
            if i >= len(self.lines):
                break
            ind, text, lineno = self.lines[i]
            if ind < indent:
                break
            if ind > indent:
                raise YamlError(lineno, "unexpected indentation in list")
            if not self.is_item(_strip_comment(text)):
                break
            rest = text[1:]
            content = rest.lstrip(" ")
            if not _strip_comment(content):
                value, i = self.node(i + 1, indent + 1)
            else:
                # Re-read the item's content as if it started on its own line,
                # so `- key: value` opens a map at the content's column.
                self.lines[i] = (indent + 1 + len(rest) - len(content), content, lineno)
                value, i = self.node(i, indent + 1)
            items.append(value)
        return items, i

    def mapping(self, i: int, indent: int):
        result: dict = {}
        while True:
            i = self.structural(i)
            if i >= len(self.lines):
                break
            ind, raw, lineno = self.lines[i]
            if ind < indent:
                break
            text = _strip_comment(raw)
            if ind > indent:
                raise YamlError(lineno, f"unexpected indentation: {text}")
            if self.is_item(text):
                break
            colon = _find_colon(text)
            if colon < 0:
                raise YamlError(lineno, f"expected 'key: value': {text}")
            key_text = text[:colon].strip()
            if key_text[:1] in "&*!?":
                raise YamlError(lineno, f"unsupported key syntax: {key_text}")
            key = _unquote(key_text, lineno)
            if key in result:
                raise YamlError(lineno, f"duplicate key: {key}")
            value_text = text[colon + 1 :].strip()
            if not value_text:
                nxt = self.structural(i + 1)
                if nxt < len(self.lines) and self.lines[nxt][0] == indent and self.is_item(
                    _strip_comment(self.lines[nxt][1])
                ):
                    value, i = self.sequence(nxt, indent)
                else:
                    value, i = self.node(i + 1, indent + 1)
            elif re.fullmatch(r"[|>][-+0-9]*", value_text):
                value, i = self.block_scalar(i + 1, indent, value_text[0] == ">")
            else:
                value, i = self.inline(value_text, i, indent, lineno)
            result[key] = value
        return result, i


def parse_yaml(source: str):
    reader = _Reader(source)
    value, i = reader.node(0, 0)
    i = reader.structural(i)
    if i < len(reader.lines):
        _, text, lineno = reader.lines[i]
        raise YamlError(lineno, f"unexpected content: {_strip_comment(text)}")
    return value


# --------------------------------------------------------------------------
# Policy checks
# --------------------------------------------------------------------------


def _is_trigger_key(key) -> bool:
    # `on` is the real key. YAML 1.1 readers (PyYAML) turn `on:` and `true:`
    # into the boolean True, so accept that spelling too.
    return key is True or (isinstance(key, str) and key.lower() in ("on", "true"))


def check_triggers(doc: dict) -> list[str]:
    keys = [k for k in doc if _is_trigger_key(k)]
    if not keys:
        return ["no `on:` trigger; the only allowed trigger is `on: workflow_dispatch`"]
    if len(keys) > 1:
        return [f"more than one trigger key ({', '.join(map(repr, keys))}); use a single `on: workflow_dispatch`"]
    on = doc[keys[0]]
    if isinstance(on, str):
        events = [on]
    elif isinstance(on, list):
        events = on
    elif isinstance(on, dict):
        events = list(on)
        dispatch = on.get(ALLOWED_TRIGGER)
        if ALLOWED_TRIGGER in on and dispatch is not None and not isinstance(dispatch, dict):
            return [f"`workflow_dispatch` must be empty or a map (inputs:), not {dispatch!r}"]
    else:
        return [f"`on:` must be `workflow_dispatch`, a list or a map, not {on!r}"]
    problems = [
        f"trigger `{e}` is not allowed: workflows are manual only (`workflow_dispatch`)"
        for e in events
        if e not in (ALLOWED_TRIGGER, CALL_TRIGGER)
    ]
    if not events:
        problems.append("`on:` is empty; the only allowed trigger is `workflow_dispatch`")
    elif CALL_TRIGGER in events and ALLOWED_TRIGGER not in events:
        # A reusable workflow still has to be startable by hand, and it only
        # runs when a manually dispatched caller runs it.
        problems.append(f"`{CALL_TRIGGER}` is allowed only alongside `{ALLOWED_TRIGGER}`")
    return problems


def _write_grants(perms) -> list[str]:
    if isinstance(perms, str):
        return [perms] if perms.strip() == "write-all" else []
    if isinstance(perms, dict):
        return [f"{scope}: write" for scope, level in perms.items() if level == "write"]
    return []


def check_permissions(doc: dict) -> list[str]:
    problems = []
    perms = doc.get("permissions")
    if not isinstance(perms, dict) or perms.get("contents") != "read":
        problems.append(f"top-level `permissions:` must be a map with `contents: read` (found {perms!r})")
    for grant in _write_grants(perms):
        problems.append(f"top-level `permissions:` grants write (`{grant}`)")
    jobs = doc.get("jobs")
    if isinstance(jobs, dict):
        for name, job in jobs.items():
            if isinstance(job, dict):
                for grant in _write_grants(job.get("permissions")):
                    problems.append(f"job `{name}` grants write (`{grant}`)")
    return problems


def _uses_values(doc: dict):
    jobs = doc.get("jobs")
    if not isinstance(jobs, dict):
        return
    for name, job in jobs.items():
        if not isinstance(job, dict):
            continue
        if "uses" in job:
            yield f"job `{name}`", job["uses"]
        steps = job.get("steps")
        if isinstance(steps, list):
            for n, step in enumerate(steps, start=1):
                if isinstance(step, dict) and "uses" in step:
                    label = step.get("name") or step.get("id") or f"step {n}"
                    yield f"job `{name}` {label!r}", step["uses"]


def check_pins(doc: dict, source: str) -> list[str]:
    problems = []
    for where, uses in _uses_values(doc):
        if not isinstance(uses, str):
            problems.append(f"{where}: `uses:` must be a string, not {uses!r}")
            continue
        if uses.startswith("./"):
            continue
        ref = uses.rpartition("@")[2] if "@" in uses else ""
        if uses.startswith("docker://") or not SHA_RE.match(ref):
            problems.append(f"{where}: `uses: {uses}` is not pinned to a full 40-character commit SHA")
    # The version comment is invisible to the parser, so read the raw lines.
    for n, line in enumerate(source.splitlines(), start=1):
        m = re.match(r"^\s*(?:-\s+)?uses:\s*([^\s#]+@[0-9a-fA-F]{40})\s*(#.*)?$", line)
        if m and not (m.group(2) or "").lstrip("#").strip():
            problems.append(f"line {n}: `uses: {m.group(1)}` needs the version in a comment (`# vX.Y.Z`)")
    return problems


def check_source(source: str) -> list[str]:
    try:
        doc = parse_yaml(source)
    except YamlError as e:
        return [f"cannot read workflow ({e})"]
    if not isinstance(doc, dict):
        return ["workflow is not a YAML map"]
    return check_triggers(doc) + check_permissions(doc) + check_pins(doc, source)


def workflow_files(root: Path) -> list[Path]:
    wf = root / ".github" / "workflows"
    if not wf.is_dir():
        return []
    return sorted(p for p in wf.iterdir() if p.suffix in (".yml", ".yaml") and p.is_file())


def run(paths: list[Path], display_root: Path) -> int:
    failed = 0
    for path in paths:
        try:
            shown = path.resolve().relative_to(display_root.resolve())
        except ValueError:
            shown = path
        problems = check_source(path.read_text(encoding="utf-8"))
        for p in problems:
            print(f"{shown}: {p}")
        failed += bool(problems)
    if failed:
        print(f"check_workflows: {failed} of {len(paths)} workflow(s) failed", file=sys.stderr)
        return 1
    print(f"check_workflows: {len(paths)} workflow(s) OK")
    return 0


# --------------------------------------------------------------------------
# Self-test
# --------------------------------------------------------------------------

SHA = "11bd71901bbe5b1630ceea73d27597364c9af683"

GOOD = {
    "dispatch-and-call.yml": """
on:
  workflow_dispatch:
  workflow_call:
    inputs:
      areas:
        type: string
permissions:
  contents: read
jobs:
  a:
    runs-on: ubuntu-24.04
    steps:
      - run: echo ok
""",
    "minimal.yml": """
on: workflow_dispatch
permissions:
  contents: read
jobs:
  hello:
    runs-on: ubuntu-latest
    steps:
      - run: echo hi
""",
    "full.yaml": f"""
# A realistic manual workflow.
name: Tests (manual)
"on":
  workflow_dispatch:
    inputs:
      preset:
        description: "CMake preset: gcc-15, clang-22"
        type: choice
        options: [gcc-15, clang-22]
        default: gcc-15
permissions:
  contents: read
concurrency: {{group: "tests-${{{{ github.ref }}}}", cancel-in-progress: true}}
jobs:
  test:
    runs-on: ${{{{ matrix.os }}}}
    permissions:
      contents: read
    strategy:
      matrix:
        os:
        - ubuntu-24.04
        - macos-15
    steps:
      - uses: actions/checkout@{SHA}  # v4.2.2
        with:
          submodules: false
      - name: Local action
        uses: ./.github/actions/setup
      - name: Build
        run: |
          cmake --workflow --preset "${{{{ inputs.preset }}}}"
          echo "done: # not a comment"
        env:
          A: 'it''s fine'
  reuse:
    uses: ./.github/workflows/other.yml
""",
    "list.yml": """
on: [workflow_dispatch]
permissions: {contents: read}
jobs: {}
""",
}

BAD = {
    "call-only.yml": ("""
on: workflow_call
permissions:
  contents: read
jobs: {}
""", ["allowed only alongside `workflow_dispatch`"]),
    "push.yml": ("""
on: [push, workflow_dispatch]
permissions:
  contents: read
jobs: {}
""", ["trigger `push`"]),
    "map_triggers.yml": ("""
on:
  workflow_dispatch:
  schedule:
    - cron: "0 0 * * *"
  pull_request:
    branches: [dev]
permissions:
  contents: read
jobs: {}
""", ["trigger `schedule`", "trigger `pull_request`"]),
    "string_trigger.yml": ("""
on: pull_request_target
permissions:
  contents: read
jobs: {}
""", ["trigger `pull_request_target`"]),
    "true_key.yml": ("""
true: workflow_run
permissions:
  contents: read
jobs: {}
""", ["trigger `workflow_run`"]),
    "no_trigger.yml": ("""
permissions:
  contents: read
jobs: {}
""", ["no `on:` trigger"]),
    "no_permissions.yml": ("""
on: workflow_dispatch
jobs: {}
""", ["`contents: read`"]),
    "read_all.yml": ("""
on: workflow_dispatch
permissions: read-all
jobs: {}
""", ["`contents: read`"]),
    "top_write.yml": ("""
on: workflow_dispatch
permissions:
  contents: write
jobs: {}
""", ["`contents: read`", "grants write (`contents: write`)"]),
    "job_write.yml": ("""
on: workflow_dispatch
permissions:
  contents: read
jobs:
  a:
    runs-on: ubuntu-latest
    permissions:
      contents: read
      pull-requests: write
    steps: [{run: echo}]
  b:
    runs-on: ubuntu-latest
    permissions: write-all
    steps: [{run: echo}]
""", ["job `a` grants write (`pull-requests: write`)", "job `b` grants write (`write-all`)"]),
    "unpinned.yml": (f"""
on: workflow_dispatch
permissions:
  contents: read
jobs:
  a:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@{SHA[:7]}  # v5
      - uses: docker://alpine:3.20
      - name: no version comment
        uses: actions/cache@{SHA}
  b:
    uses: some/repo/.github/workflows/x.yml@main
""", [
        "`uses: actions/checkout@v4` is not pinned",
        f"`uses: actions/setup-python@{SHA[:7]}` is not pinned",
        "`uses: docker://alpine:3.20` is not pinned",
        f"`uses: actions/cache@{SHA}` needs the version in a comment",
        "`uses: some/repo/.github/workflows/x.yml@main` is not pinned",
    ]),
    "anchor.yml": ("""
on: workflow_dispatch
permissions: &p
  contents: read
jobs: {}
""", ["cannot read workflow"]),
    "duplicate.yml": ("""
on: workflow_dispatch
on: push
permissions:
  contents: read
""", ["duplicate key"]),
}


def self_test() -> int:
    failures = []

    def expect(cond: bool, what: str) -> None:
        if not cond:
            failures.append(what)

    # The parser itself.
    expect(parse_yaml("on: x\n") == {"on": "x"}, "`on:` stays a string key")
    expect(parse_yaml("a:\n- 1\n- b: 2\n  c: 3\n") == {"a": ["1", {"b": "2", "c": "3"}]}, "lists of maps")
    expect(parse_yaml("a: |\n  x: 1\n  - y\nb: z\n") == {"a": "x: 1\n- y\n", "b": "z"}, "block scalars")
    expect(parse_yaml("a: {b: [1, 'c, d'], e: f}\n") == {"a": {"b": ["1", "c, d"], "e": "f"}}, "flow values")
    # A YAML 1.1 reader's True key (PyYAML's spelling of `on:`).
    expect(check_triggers({True: {"workflow_dispatch": None}}) == [], "True key accepted")
    expect(any("push" in p for p in check_triggers({True: ["push"]})), "True key checked")

    with tempfile.TemporaryDirectory() as tmp, contextlib.redirect_stdout(io.StringIO()), \
            contextlib.redirect_stderr(io.StringIO()):
        root = Path(tmp)
        if run(workflow_files(root), root) != 0:
            failures.append("no .github/workflows should pass")
        wf = root / ".github" / "workflows"
        wf.mkdir(parents=True)
        if run(workflow_files(root), root) != 0:
            failures.append("an empty workflows folder should pass")
        for name, text in GOOD.items():
            (wf / name).write_text(text)
            problems = check_source(text)
            expect(not problems, f"good sample {name} failed: {problems}")
        expect(run(workflow_files(root), root) == 0, "all good samples should pass together")
        for name, (text, wanted) in BAD.items():
            (wf / name).write_text(text)
            problems = check_source(text)
            for w in wanted:
                expect(any(w in p for p in problems), f"bad sample {name}: no problem mentioning {w!r} in {problems}")
            expect(len(problems) == len(wanted), f"bad sample {name}: expected {len(wanted)} problem(s), got {problems}")
        expect(run(workflow_files(root), root) == 1, "a folder with bad samples should fail")

    for f in failures:
        print(f"SELF-TEST FAIL: {f}", file=sys.stderr)
    total = len(GOOD) + len(BAD)
    print(f"check_workflows self-test: {'FAILED' if failures else 'OK'} ({total} samples, {len(failures)} failure(s))")
    return 1 if failures else 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("paths", nargs="*", type=Path, help="workflow files (default: .github/workflows/*.yml|*.yaml)")
    ap.add_argument("--root", type=Path, default=REPO_ROOT, help="repository root (default: this checkout)")
    ap.add_argument("--self-test", action="store_true", help="run the built-in good/bad samples and exit")
    args = ap.parse_args(argv)
    if args.self_test:
        return self_test()
    paths = args.paths or workflow_files(args.root)
    for p in paths:
        if not p.is_file():
            print(f"check_workflows: no such file: {p}", file=sys.stderr)
            return 2
    return run(paths, args.root)


if __name__ == "__main__":
    sys.exit(main())
