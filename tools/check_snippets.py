#!/usr/bin/env python3
"""Compile-check the C++ code blocks in the docs.

Each ```cpp block is compiled with `-fsyntax-only` after a shared prelude
(tools/snippet_prelude.hpp: common standard headers and placeholder names like
Widget and use()). The fence's info string controls how a block is checked:

    ```cpp                  compile at namespace scope (the default)
    ```cpp body             wrap in a function body (for statement snippets)
    ```cpp nocompile        skip: deliberately wrong (BAD examples) or pseudo-code
    ```cpp needs=reflection skip unless --enable reflection is given
                            (also: needs=contracts, needs=embed, ...)
    ```cpp compile-fail     must NOT compile (a BAD example that is a type error)
    ```cpp compile-fail="no matching function"
                            ... and the diagnostics must match this regex

Tokens can be combined: ```cpp body needs=contracts. Quote values that
contain spaces.

Every block has a stable ID, `<file>#<heading-slug>/<n>` (the nth cpp block
under its nearest heading), so editing a block doesn't change its ID.

Baseline of known failures (for docs/examples/, which has many):

    --baseline tools/snippet_baseline/gcc.txt
        Blocks listed there may fail. The run fails on a NEW failure, and also
        on a listed block that now passes or no longer exists, so the file
        only shrinks. Keep one baseline per compiler family.
    --update-baseline
        Rewrite the baseline with the current failures (entries for files
        outside the checked paths are kept).

Usage:
    tools/check_snippets.py [paths...] [--cxx g++-16] [--std c++26]
                            [--flag=-freflection ...] [--enable reflection ...]
                            [--baseline FILE [--update-baseline]]
                            [--report-only] [-j N]

Paths default to docs/. Exit status is 1 if any checked block fails (or, with
--baseline, on new failures and stale entries), unless --report-only is given.
"""
from __future__ import annotations

import argparse
import concurrent.futures as cf
import os
import pathlib
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
PRELUDE = ROOT / "tools" / "snippet_prelude.hpp"
FENCE = re.compile(r"^(?P<indent>\s*)(?P<fence>`{3,}|~{3,})(?P<info>.*)$")


HEADING = re.compile(r"^#{1,6}\s+(.*?)\s*#*\s*$")


def slug(heading: str) -> str:
    """GitHub-style heading anchor."""
    h = re.sub(r"[`*_]|\[([^\]]*)\]\([^)]*\)", r"\1", heading).strip().lower()
    return re.sub(r"[^\w\- ]", "", h).replace(" ", "-") or "top"


class Block:
    def __init__(self, path: pathlib.Path, line: int, info: list[str], code: str, ident: str = ""):
        self.path, self.line, self.info, self.code, self.ident = path, line, info, code, ident

    @property
    def where(self) -> str:
        return f"{self.path.relative_to(ROOT)}:{self.line}"

    @property
    def id(self) -> str:
        return f"{self.path.relative_to(ROOT)}#{self.ident}"

    def needs(self) -> set[str]:
        return {t.split("=", 1)[1] for t in self.info if t.startswith("needs=")}

    def compile_fail(self) -> str | None:
        """None if a normal block; otherwise the diagnostic regex ('' = any error)."""
        for t in self.info:
            if t == "compile-fail":
                return ""
            if t.startswith("compile-fail="):
                return t.split("=", 1)[1]
        return None


def extract(path: pathlib.Path) -> list[Block]:
    """Return the cpp blocks in a Markdown file, with their starting line."""
    blocks: list[Block] = []
    lines = path.read_text(encoding="utf-8").splitlines()
    heading, counts, seen = "top", {}, {}
    i = 0
    while i < len(lines):
        m = FENCE.match(lines[i])
        if not m:
            h = HEADING.match(lines[i])
            if h:  # repeated headings get -1, -2, ... like GitHub anchors
                base = slug(h[1])
                n = seen.get(base, 0)
                seen[base] = n + 1
                heading = base if n == 0 else f"{base}-{n}"
            i += 1
            continue
        fence = m["fence"]
        try:
            info = shlex.split(m["info"].strip())
        except ValueError:
            info = m["info"].strip().split()
        start = i + 1
        i += 1
        body: list[str] = []
        indent = len(m["indent"])
        while i < len(lines):
            close = FENCE.match(lines[i])
            if close and close["fence"][0] == fence[0] and len(close["fence"]) >= len(fence) and not close["info"].strip():
                break
            body.append(lines[i][indent:] if lines[i][:indent].isspace() else lines[i])
            i += 1
        i += 1  # skip closing fence
        # Only top-level cpp fences; anything nested in another fence is skipped
        # because the outer fence swallowed it.
        if info and info[0] == "cpp":
            counts[heading] = counts.get(heading, 0) + 1
            blocks.append(Block(path, start, info[1:], "\n".join(body) + "\n",
                                f"{heading}/{counts[heading]}"))
    return blocks


def source_for(block: Block) -> str:
    code = block.code
    if "body" in block.info:
        code = "void snippet_body_() {\n" + code + "}\n"
    # #line maps compiler diagnostics back to the Markdown file.
    first = block.line + 1 if "body" not in block.info else block.line
    return f'#include "{PRELUDE}"\n#line {first} "{block.path.relative_to(ROOT)}"\n{code}'


def check(block: Block, cmd: list[str]) -> tuple[Block, bool, str]:
    with tempfile.NamedTemporaryFile("w", suffix=".cpp", delete=False) as f:
        f.write(source_for(block))
        name = f.name
    try:
        r = subprocess.run(cmd + [name], capture_output=True, text=True, timeout=120)
        expect = block.compile_fail()
        if expect is None:
            return block, r.returncode == 0, r.stderr
        if r.returncode == 0:
            return block, False, "compile-fail block compiled without errors"
        if expect and not re.search(expect, r.stderr):
            return block, False, f"compile-fail block failed, but no diagnostic matched /{expect}/:\n{r.stderr}"
        return block, True, ""
    finally:
        os.unlink(name)


def compiler_version(cxx: str) -> str:
    try:
        out = subprocess.run([cxx, "--version"], capture_output=True, text=True, timeout=30).stdout
        return out.splitlines()[0] if out else "unknown"
    except (OSError, subprocess.SubprocessError):
        return "unknown"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("paths", nargs="*", default=["docs"])
    ap.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    ap.add_argument("--std", default="c++26")
    ap.add_argument("--flag", action="append", default=[], help="extra compiler flag (repeatable)")
    ap.add_argument("--enable", action="append", default=[], help="feature for needs=... blocks (repeatable)")
    ap.add_argument("--baseline", type=pathlib.Path, help="file of block IDs allowed to fail")
    ap.add_argument("--update-baseline", action="store_true", help="rewrite --baseline with the current failures")
    ap.add_argument("--report-only", action="store_true", help="always exit 0")
    ap.add_argument("-j", type=int, default=os.cpu_count() or 2)
    ap.add_argument("-q", "--quiet", action="store_true", help="don't print compiler output")
    args = ap.parse_args()

    files: list[pathlib.Path] = []
    for p in args.paths:
        p = (ROOT / p) if not os.path.isabs(p) else pathlib.Path(p)
        files += sorted(p.rglob("*.md")) if p.is_dir() else [p]

    blocks = [b for f in files for b in extract(f)]
    nocompile = [b for b in blocks if "nocompile" in b.info]
    gated = [b for b in blocks if b not in nocompile and not b.needs() <= set(args.enable)]
    todo = [b for b in blocks if b not in nocompile and b not in gated]

    cmd = [args.cxx, f"-std={args.std}", "-fsyntax-only", "-pedantic-errors", *args.flag]
    failures: list[tuple[Block, str]] = []
    with cf.ThreadPoolExecutor(max_workers=args.j) as pool:
        for block, ok, err in pool.map(lambda b: check(b, cmd), todo):
            if not ok:
                failures.append((block, err))

    failures.sort(key=lambda x: (str(x[0].path), x[0].line))

    # Baseline: split failures into known and new; find fixed and stale entries.
    known: set[str] = set()
    base_lines: list[str] = []
    if args.baseline and args.baseline.exists():
        base_lines = args.baseline.read_text(encoding="utf-8").splitlines()
        known = {l.strip() for l in base_lines if l.strip() and not l.startswith("#")}
    checked_files = {str(f.relative_to(ROOT)) for f in files}
    in_scope = {k for k in known if k.split("#", 1)[0] in checked_files}
    failed_ids = {b.id for b, _ in failures}
    checked_ids = {b.id for b in todo}
    new = [(b, e) for b, e in failures if b.id not in known]
    fixed = sorted(k for k in in_scope if k in checked_ids and k not in failed_ids)
    stale = sorted(k for k in in_scope if k not in checked_ids)

    if args.baseline and args.update_baseline:
        header = [l for l in base_lines if l.startswith("#")] or [
            "# Known snippet failures for tools/check_snippets.py --baseline.",
            "# One block ID per line. Regenerate with --update-baseline; the list should only shrink."]
        keep = sorted((known - in_scope) | failed_ids)
        args.baseline.parent.mkdir(parents=True, exist_ok=True)
        args.baseline.write_text("\n".join(header + keep) + "\n", encoding="utf-8")
        print(f"wrote {len(keep)} entries to {args.baseline}")
        new, fixed, stale = [], [], []

    for block, err in (new if args.baseline else failures):
        print(f"FAIL {block.where}  [{block.id}]")
        if not args.quiet:
            print("\n".join("    " + l for l in err.strip().splitlines()[:15]))
    for k in fixed:
        print(f"FIXED {k}  (now compiles: remove it from the baseline)")
    for k in stale:
        print(f"STALE {k}  (no such checked block: remove it from the baseline)")
    # Skips are reported by reason so untested blocks stay visible.
    missing = sorted({n for b in gated for n in b.needs() - set(args.enable)})
    print(f"\n{len(todo)} checked, {len(failures)} failed, "
          f"{len(nocompile) + len(gated)} skipped "
          f"({len(nocompile)} nocompile, {len(gated)} need {', '.join(missing) or '-'})")
    if args.baseline:
        print(f"baseline: {len(failures) - len(new)} known failures still failing, "
              f"{len(new)} new, {len(fixed)} fixed, {len(stale)} stale")
    print(f"compiler: {compiler_version(args.cxx)}\ncommand: {' '.join(cmd)}")
    if args.report_only:
        return 0
    bad = (new or fixed or stale) if args.baseline else failures
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
