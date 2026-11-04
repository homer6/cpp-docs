#!/usr/bin/env python3
"""Print the GitHub release text for docs/releases/vX.Y.Z.md (#390).

The notes file is Markdown wrapped at about 78 columns, which is right in the
repo. A GitHub release description is different: GitHub turns every newline
in it into a hard line break, so a wrapped file shows its sentences broken
mid-line. This prints the notes ready for `gh release create` / `gh release
edit`:

  * hard-wrapped lines of a paragraph, list item or blockquote are joined
    into one line. Headings, table rows, fenced code, HTML comments, blank
    lines and lines ending in a Markdown hard break (two spaces or `\\`) are
    left as they are, and a line that starts a list item, heading, table,
    quote or fence always starts a new line;
  * relative links (`../security/README.md`, `v0.4.0.md#x`) are made
    absolute at the tag, `https://github.com/<repo>/blob/vX.Y.Z/docs/...`
    (`tree/` for a link ending in `/`), because relative links are dead on a
    release page. Links that are absolute, `#anchors` or `mailto:` are kept.

The words don't change: only line breaks between them and link targets.

    python3 tools/release_notes.py v0.5.0 > notes.md
    gh release create v0.5.0 --title "Release v0.5.0" --notes-file notes.md --verify-tag
    python3 tools/release_notes.py --self-test   # built-in samples
"""

from __future__ import annotations

import argparse
import posixpath
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_REPO = "homer6/cpp-docs"

# A line that starts its own block: a list item, heading, table row, fence,
# quote or HTML. It is never joined onto the line before it.
STARTS_BLOCK = re.compile(r"^\s*([-*+]\s|\d+[.)]\s|#|\||```|~~~|>|<)")
# A line nothing may be joined onto.
ENDS_BLOCK = re.compile(r"^\s*(#|\||```|~~~|<!--|<)")
LINK = re.compile(r"(\]\()([^)\s]+)(\))")
FENCE = re.compile(r"^\s*(```|~~~)")


def absolute_link(target: str, repo: str, tag: str, base: str) -> str:
    """Make one relative link target absolute at the tag; leave the rest alone."""
    if re.match(r"^([a-z][a-z0-9+.-]*:|#|/)", target, re.IGNORECASE):
        return target
    path, frag = (target.split("#", 1) + [""])[:2]
    resolved = posixpath.normpath(posixpath.join(base, path))
    if resolved.startswith(".."):
        raise ValueError(f"link {target!r} points outside the repo")
    kind, slash = ("tree", "/") if path.endswith("/") else ("blob", "")
    return f"https://github.com/{repo}/{kind}/{tag}/{resolved}{slash}" + (f"#{frag}" if frag else "")


def convert(text: str, repo: str, tag: str, base: str = "docs/releases") -> str:
    out: list[str] = []
    in_fence = False
    for line in text.splitlines():
        if FENCE.match(line):
            in_fence = not in_fence
            out.append(line)
            continue
        if in_fence:
            out.append(line)
            continue
        line = LINK.sub(lambda m: m.group(1) + absolute_link(m.group(2), repo, tag, base) + m.group(3), line)
        prev = out[-1] if out else ""
        joinable = (
            prev.strip()
            and not ENDS_BLOCK.match(prev)
            and not prev.endswith(("  ", "\\"))
            and not (out and FENCE.match(prev))
        )
        if line.strip() and joinable and not STARTS_BLOCK.match(line):
            out[-1] = prev.rstrip() + " " + line.strip()
        elif (
            joinable
            and line.lstrip().startswith(">")
            and prev.lstrip().startswith(">")
            and prev.strip() != ">"
            and line.strip() != ">"
            and not STARTS_BLOCK.match(line.lstrip()[1:])
        ):
            out[-1] = prev.rstrip() + " " + line.lstrip()[1:].strip()
        else:
            out.append(line)
    return "\n".join(out) + "\n"


SAMPLES = [
    ("paragraph", "One two\nthree four.\n\nNext\npara.\n", "One two three four.\n\nNext para.\n"),
    ("list items and continuations",
     "- **A**\n  ([#1](x))\n  - nested\n    more\n- B\n",
     "- **A** ([#1](https://github.com/o/r/blob/v1.0.0/docs/releases/x))\n  - nested more\n- B\n"),
    ("numbered list", "1. one\n   two\n2. three\n", "1. one two\n2. three\n"),
    ("heading and table kept",
     "## Head\n\n| a | b |\n|---|---|\n| 1 | 2 |\n", "## Head\n\n| a | b |\n|---|---|\n| 1 | 2 |\n"),
    ("fenced code kept", "text\n```sh\na\nb\n```\nafter\nmore\n", "text\n```sh\na\nb\n```\nafter more\n"),
    ("blockquote", "> one\n> two\n>\n> three\n", "> one two\n>\n> three\n"),
    ("hard break kept", "line one  \nline two\n", "line one  \nline two\n"),
    ("comment kept", "<!-- marker -->\ntext\n", "<!-- marker -->\ntext\n"),
    ("links",
     "[a](../security/README.md) [b](v0.4.0.md#x) [c](https://e.com) [d](#here) [e](mailto:x@y)\n",
     "[a](https://github.com/o/r/blob/v1.0.0/docs/security/README.md) "
     "[b](https://github.com/o/r/blob/v1.0.0/docs/releases/v0.4.0.md#x) "
     "[c](https://e.com) [d](#here) [e](mailto:x@y)\n"),
    ("folder link", "[runs](../platform-support/runs/)\n",
     "[runs](https://github.com/o/r/tree/v1.0.0/docs/platform-support/runs/)\n"),
]


def self_test() -> int:
    failures = 0
    for name, given, want in SAMPLES:
        got = convert(given, "o/r", "v1.0.0")
        if got != want:
            failures += 1
            print(f"FAIL {name}:\n  want {want!r}\n  got  {got!r}")
    try:
        convert("[x](../../../outside.md)\n", "o/r", "v1.0.0")
        failures += 1
        print("FAIL link outside the repo: no error")
    except ValueError:
        pass
    total = len(SAMPLES) + 1
    print(f"release_notes self-test: {'FAILED' if failures else 'OK'} ({total} samples, {failures} failure(s))")
    return 1 if failures else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("tag", nargs="?", help="the release tag, vX.Y.Z (reads docs/releases/vX.Y.Z.md)")
    ap.add_argument("--repo", default=DEFAULT_REPO, help=f"owner/name for the links (default {DEFAULT_REPO})")
    ap.add_argument("--self-test", action="store_true", help="run the built-in samples and exit")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    if not args.tag or not re.fullmatch(r"v\d+\.\d+\.\d+", args.tag):
        ap.error("give the tag as vX.Y.Z")
    notes = ROOT / "docs" / "releases" / f"{args.tag}.md"
    if not notes.is_file():
        print(f"error: {notes.relative_to(ROOT)} not found", file=sys.stderr)
        return 2
    try:
        sys.stdout.write(convert(notes.read_text(encoding="utf-8"), args.repo, args.tag))
    except ValueError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
