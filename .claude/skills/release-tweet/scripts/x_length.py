#!/usr/bin/env python3
"""Count a post the way X does, roughly (#395).

X weighs every link as 23 characters whatever its length, and counts most
emoji and CJK characters as 2. Over 280 needs a long post (X Premium).

    python3 .claude/skills/release-tweet/scripts/x_length.py post.txt
    python3 .claude/skills/release-tweet/scripts/x_length.py - < post.txt
    python3 .claude/skills/release-tweet/scripts/x_length.py --self-test
"""

from __future__ import annotations

import re
import sys
import unicodedata

URL = re.compile(r"https?://\S+|\b(?:[a-z0-9-]+\.)+(?:com|org|net|io|dev)(?:/\S*)?", re.IGNORECASE)
LINK_WEIGHT = 23
LIMIT = 280


def weight(ch: str) -> int:
    o = ord(ch)
    if o >= 0x1F000 or unicodedata.east_asian_width(ch) in ("W", "F"):
        return 2
    return 1


def x_length(text: str) -> int:
    text = text.strip("\n")
    n, pos = 0, 0
    for m in URL.finditer(text):
        n += sum(weight(c) for c in text[pos:m.start()]) + LINK_WEIGHT
        pos = m.end()
    return n + sum(weight(c) for c in text[pos:])


def self_test() -> int:
    cases = [
        ("hello", 5),
        ("see https://github.com/homer6/cpp-docs/releases/tag/v0.5.0", 4 + 23),
        ("http://github.com/homer6/cpp-docs", 23),
        ("go 🚀", 5),
        ("a\nb", 3),
    ]
    bad = [(t, want, x_length(t)) for t, want in cases if x_length(t) != want]
    for t, want, got in bad:
        print(f"FAIL {t!r}: want {want}, got {got}")
    print(f"x_length self-test: {'FAILED' if bad else 'OK'} ({len(cases)} samples)")
    return 1 if bad else 0


def main() -> int:
    if sys.argv[1:] == ["--self-test"]:
        return self_test()
    if len(sys.argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    text = sys.stdin.read() if sys.argv[1] == "-" else open(sys.argv[1], encoding="utf-8").read()
    n = x_length(text)
    print(f"{n} / {LIMIT}" + ("" if n <= LIMIT else f"  (over by {n - LIMIT}: needs a long post)"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
