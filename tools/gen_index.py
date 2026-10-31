#!/usr/bin/env python3
"""Regenerate the clickable reference index in README.md.

The Standard library and Core language lists in README.md's Layout section are
generated from docs/examples/: area order and descriptions come from the
tables in docs/examples/README.md, page titles from each page's first heading.
The generated text sits between the markers

    <!-- index:library:start --> ... <!-- index:library:end -->
    <!-- index:language:start --> ... <!-- index:language:end -->

Usage:
    python3 tools/gen_index.py           rewrite README.md
    python3 tools/gen_index.py --check   exit 1 if README.md is out of date
"""
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXAMPLES = ROOT / "docs" / "examples"
README = ROOT / "README.md"
ROW = re.compile(r"^\| \[([^\]]+)\]\(([^)]+)/README\.md\) \| [^|]* \| ([^|]*) \|", re.M)


def rel(path: pathlib.Path) -> str:
    return path.relative_to(ROOT).as_posix()


def title(page: pathlib.Path) -> str:
    for line in page.read_text(encoding="utf-8").splitlines():
        if line.startswith("# "):
            return line[2:].strip()
    return page.stem


def pages(folder: pathlib.Path) -> list[pathlib.Path]:
    return sorted(p for p in folder.glob("*.md") if p.name != "README.md")


def links(folder: pathlib.Path) -> str:
    return " · ".join(f"[{title(p)}]({rel(p)})" for p in pages(folder))


def area_lines(name: str, folder: pathlib.Path, desc: str) -> list[str]:
    out = [f"- **[{name}]({rel(folder / 'README.md')})** — {desc.strip()}"]
    if pages(folder):
        out.append(f"  - {links(folder)}")
    for sub in sorted(d for d in folder.iterdir() if d.is_dir()):
        if not pages(sub):
            continue
        label = f"[{sub.name}]({rel(sub / 'README.md')})" if (sub / "README.md").exists() else sub.name
        out.append(f"  - {label}: {links(sub)}")
    return out


def generate() -> dict[str, str]:
    library: list[str] = []
    language: list[str] = []
    for name, folder, desc in ROW.findall((EXAMPLES / "README.md").read_text(encoding="utf-8")):
        target = language if folder.startswith("language/") else library
        target += area_lines(name, EXAMPLES / folder, desc)
    return {"library": "\n".join(library), "language": "\n".join(language)}


def render(text: str, sections: dict[str, str]) -> str:
    for key, body in sections.items():
        start, end = f"<!-- index:{key}:start -->", f"<!-- index:{key}:end -->"
        pattern = re.compile(re.escape(start) + r".*?" + re.escape(end), re.S)
        if not pattern.search(text):
            sys.exit(f"README.md is missing the {start} / {end} markers")
        text = pattern.sub(lambda _: f"{start}\n{body}\n{end}", text)
    return text


def main() -> int:
    current = README.read_text(encoding="utf-8")
    updated = render(current, generate())
    if "--check" in sys.argv[1:]:
        if updated != current:
            print("README.md index is out of date: run python3 tools/gen_index.py")
            return 1
        print("README.md index is up to date")
        return 0
    if updated != current:
        README.write_text(updated, encoding="utf-8")
        print("README.md index regenerated")
    else:
        print("README.md index already up to date")
    return 0


if __name__ == "__main__":
    sys.exit(main())
