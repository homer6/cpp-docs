#!/usr/bin/env python3
"""Check relative links and #anchors in Markdown files.

Usage:
    python3 check_links.py [DIR_OR_FILE ...]      (default: docs/guidelines)

Reports links whose target file doesn't exist, and #anchors that don't match a
heading in the target (GitHub-style slugs). Ignores http(s)/mailto links and
anything inside fenced code blocks. Exits 1 if anything is broken.
"""

import pathlib
import re
import sys

LINK = re.compile(r"\[[^\]]*\]\(([^)\s]+)(?:\s+\"[^\"]*\")?\)")
HEADING = re.compile(r"^#{1,6}\s+(.*)$", re.M)
FENCE = re.compile(r"^(```|~~~).*?^\1", re.M | re.S)


def slug(heading: str) -> str:
    h = re.sub(r"[`*_]|\[([^\]]*)\]\([^)]*\)", r"\1", heading).strip().lower()
    h = re.sub(r"[^\w\- ]", "", h)
    return h.replace(" ", "-")


def anchors(path: pathlib.Path) -> set[str]:
    text = FENCE.sub("", path.read_text())
    seen: dict[str, int] = {}
    result = set()
    for h in HEADING.findall(text):
        s = slug(h)
        n = seen.get(s, 0)
        result.add(s if n == 0 else f"{s}-{n}")
        seen[s] = n + 1
    return result


def check(md: pathlib.Path) -> list[str]:
    problems = []
    text = FENCE.sub("", md.read_text())
    for target in LINK.findall(text):
        if re.match(r"^(https?:|mailto:)", target):
            continue
        file_part, _, anchor = target.partition("#")
        dest = md if not file_part else (md.parent / file_part)
        if not dest.exists():
            problems.append(f"{md}: missing file -> {target}")
            continue
        if anchor and dest.suffix == ".md" and anchor not in anchors(dest):
            problems.append(f"{md}: missing anchor -> {target}")
    return problems


def main(argv: list[str]) -> int:
    roots = [pathlib.Path(a) for a in argv] or [pathlib.Path("docs/guidelines")]
    files = []
    for r in roots:
        files += [r] if r.is_file() else sorted(r.rglob("*.md"))
    problems = [p for f in files for p in check(f)]
    print("\n".join(problems) if problems else f"ok: {len(files)} files")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
