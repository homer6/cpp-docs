#!/usr/bin/env python3
"""Collect deep-research prompts into one paste-ready file.

Each prompt file has an instructions header, a line containing only `---`,
then the prompt itself. This script takes the part after the first `---`,
replaces {{DATE}} with today's date (or --date), and writes one file with a
heading and a fenced block per prompt, ready to copy into Perplexity.

Usage:
    python3 collect_prompts.py PROMPT.md [PROMPT.md ...] [--date YYYY-MM-DD] [-o OUT]

Default output: .firecrawl/prompts-<date>.md (relative to the current directory).
"""

import argparse
import datetime
import pathlib
import sys


def prompt_body(text: str) -> str:
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if line.strip() == "---":
            return "\n".join(lines[i + 1:]).strip() + "\n"
    return text.strip() + "\n"  # no header: use the whole file


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("prompts", nargs="+", type=pathlib.Path)
    ap.add_argument("--date", default=datetime.date.today().isoformat())
    ap.add_argument("-o", "--out", type=pathlib.Path)
    args = ap.parse_args()

    out = args.out or pathlib.Path(".firecrawl") / f"prompts-{args.date}.md"
    out.parent.mkdir(parents=True, exist_ok=True)

    parts = [f"# Deep research prompts — {args.date}\n\n",
             "Copy the block under each heading into Perplexity Deep Research.\n"
             "Save each report as `.firecrawl/reports/<name>-<date>.md` and ask "
             "Claude to import it.\n"]
    prompts = [p for p in args.prompts if p.name != "TEMPLATE.md"]
    parts.append("\n" + "".join(f"{i}. [{p.stem}](#prompt-{p.stem.lower()})\n"
                                 for i, p in enumerate(prompts, 1)))
    for path in prompts:
        body = prompt_body(path.read_text()).replace("{{DATE}}", args.date)
        if "{{" in body:
            print(f"warning: {path} still has a {{{{placeholder}}}}", file=sys.stderr)
        fence = "````" if "```" in body else "```"
        parts.append(f"\n## Prompt: {path.stem}\n\n{fence}text\n{body}{fence}\n")

    out.write_text("".join(parts))
    print(out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
