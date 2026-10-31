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

Tokens can be combined: ```cpp body needs=contracts

Usage:
    tools/check_snippets.py [paths...] [--cxx g++-16] [--std c++26]
                            [--flag=-freflection ...] [--enable reflection ...]
                            [--report-only] [-j N]

Paths default to docs/. Exit status is 1 if any checked block fails,
unless --report-only is given.
"""
from __future__ import annotations

import argparse
import concurrent.futures as cf
import os
import pathlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
PRELUDE = ROOT / "tools" / "snippet_prelude.hpp"
FENCE = re.compile(r"^(?P<indent>\s*)(?P<fence>`{3,}|~{3,})(?P<info>.*)$")


class Block:
    def __init__(self, path: pathlib.Path, line: int, info: list[str], code: str):
        self.path, self.line, self.info, self.code = path, line, info, code

    @property
    def where(self) -> str:
        return f"{self.path.relative_to(ROOT)}:{self.line}"

    def needs(self) -> set[str]:
        return {t.split("=", 1)[1] for t in self.info if t.startswith("needs=")}


def extract(path: pathlib.Path) -> list[Block]:
    """Return the cpp blocks in a Markdown file, with their starting line."""
    blocks: list[Block] = []
    lines = path.read_text(encoding="utf-8").splitlines()
    i = 0
    while i < len(lines):
        m = FENCE.match(lines[i])
        if not m:
            i += 1
            continue
        fence, info = m["fence"], m["info"].strip().split()
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
            blocks.append(Block(path, start, info[1:], "\n".join(body) + "\n"))
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
        return block, r.returncode == 0, r.stderr
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
    for block, err in failures:
        print(f"FAIL {block.where}")
        if not args.quiet:
            print("\n".join("    " + l for l in err.strip().splitlines()[:15]))
    # Skips are reported by reason so untested blocks stay visible.
    missing = sorted({n for b in gated for n in b.needs() - set(args.enable)})
    print(f"\n{len(todo)} checked, {len(failures)} failed, "
          f"{len(nocompile) + len(gated)} skipped "
          f"({len(nocompile)} nocompile, {len(gated)} need {', '.join(missing) or '-'})")
    print(f"compiler: {compiler_version(args.cxx)}\ncommand: {' '.join(cmd)}")
    return 0 if args.report_only or not failures else 1


if __name__ == "__main__":
    sys.exit(main())
