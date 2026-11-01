#!/usr/bin/env python3
"""Report which C++ names or phrases the reference already covers.

    find_gaps.py fold_left cartesian_product 'std::launder'
    find_gaps.py --file terms.txt          # one term per line, '#' comments
    find_gaps.py --regex 'auto\\(x\\)|decay-copy'

Each term is searched in docs/examples/ (the pages) and tests/ (the claims
checked at run time). A term found in neither is printed as MISSING. Search
for the bare name (`fold_left`, not `std::ranges::fold_left`), because pages
often drop the namespace.
"""
import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[4]


def files(sub: str, pattern: str) -> list[pathlib.Path]:
    return sorted((ROOT / sub).rglob(pattern))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("terms", nargs="*")
    ap.add_argument("--file", help="read terms from this file, one per line")
    ap.add_argument("--regex", action="store_true", help="treat terms as regular expressions")
    ap.add_argument("-v", "--verbose", action="store_true", help="list every matching file")
    args = ap.parse_args()

    terms = list(args.terms)
    if args.file:
        for line in pathlib.Path(args.file).read_text().splitlines():
            line = line.split("#", 1)[0].strip()
            if line:
                terms.append(line)
    if not terms:
        ap.error("give terms or --file")

    pages = {p: p.read_text(errors="replace") for p in files("docs/examples", "*.md")}
    tests = {p: p.read_text(errors="replace") for p in files("tests", "*.cpp")}

    missing = 0
    for term in terms:
        rx = re.compile(term if args.regex else re.escape(term))
        in_pages = [p for p, t in pages.items() if rx.search(t)]
        in_tests = [p for p, t in tests.items() if rx.search(t)]
        if not in_pages and not in_tests:
            print(f"MISSING  {term}")
            missing += 1
            continue
        where = ", ".join(str(p.relative_to(ROOT / "docs/examples")) for p in in_pages[:3])
        more = f" (+{len(in_pages) - 3})" if len(in_pages) > 3 else ""
        print(f"found    {term}: {len(in_pages)} page(s), {len(in_tests)} test file(s)  {where}{more}")
        if args.verbose:
            for p in in_pages + in_tests:
                print(f"           {p.relative_to(ROOT)}")
    print(f"\n{missing} of {len(terms)} missing")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
