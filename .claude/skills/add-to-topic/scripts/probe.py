#!/usr/bin/env python3
"""Try an idea on every local toolchain before writing about it.

    probe.py snippet idea.cpp           # compile (-fsyntax-only) on each toolchain
    probe.py snippet idea.cpp --run     # build and run; prints each program's output
    probe.py macros __cpp_lib_function_ref __cpp_lib_ranges_fold
    probe.py ... --only gcc-15,clang-22 # skip Docker

Toolchains match CMakePresets.json: gcc-15 (Homebrew g++-15), clang-22
(Homebrew Clang + libc++) and gcc-16 (Docker image gcc:16). Every compile uses
-std=c++26 -pedantic-errors. The snippet must be a whole translation unit
(includes plus main() if you use --run); the tools/check_snippets.py prelude is
not added.
"""
import argparse
import pathlib
import shutil
import subprocess
import sys
import tempfile

LLVM = "/opt/homebrew/opt/llvm"
CLANG_LINK = [f"-L{LLVM}/lib/c++", f"-L{LLVM}/lib/unwind", "-lunwind",
              f"-Wl,-rpath,{LLVM}/lib/c++", f"-Wl,-rpath,{LLVM}/lib/unwind"]
TOOLCHAINS = {
    "gcc-15": ["g++-15"],
    "clang-22": [f"{LLVM}/bin/clang++", "-stdlib=libc++", "-fexperimental-library"],
    "gcc-16": ["g++"],  # run inside Docker
}
FLAGS = ["-std=c++26", "-pedantic-errors"]


def run(name: str, workdir: pathlib.Path, src: str, execute: bool, extra: list[str]):
    """Returns (ok, text): text is the program output, or the first error lines."""
    cxx = TOOLCHAINS[name]
    if execute:
        cmd = cxx + FLAGS + extra + [src, "-o", "a.out"] + (CLANG_LINK if name == "clang-22" else [])
        cmd_str = " ".join(cmd) + " && ./a.out"
    else:
        cmd = cxx + FLAGS + extra + ["-fsyntax-only", src]
        cmd_str = " ".join(cmd)
    if name == "gcc-16":
        if not shutil.which("docker"):
            return False, "docker not found"
        argv = ["docker", "run", "--rm", "-v", f"{workdir}:/w", "-w", "/w", "gcc:16", "sh", "-c", cmd_str]
    else:
        argv = ["sh", "-c", cmd_str]
    p = subprocess.run(argv, cwd=workdir, capture_output=True, text=True)
    out = (p.stdout + p.stderr).strip()
    if p.returncode != 0:
        errs = [l for l in out.splitlines() if "error" in l][:3] or out.splitlines()[:3]
        return False, "\n".join(errs)
    return True, p.stdout.strip()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="mode", required=True)
    s = sub.add_parser("snippet")
    s.add_argument("file")
    s.add_argument("--run", action="store_true")
    s.add_argument("--flag", action="append", default=[], help="extra compiler flag, e.g. --flag=-freflection")
    m = sub.add_parser("macros")
    m.add_argument("names", nargs="+")
    for p in (s, m):
        p.add_argument("--only", help="comma-separated toolchains (default: all three)")
    args = ap.parse_args()

    names = args.only.split(",") if args.only else list(TOOLCHAINS)
    with tempfile.TemporaryDirectory() as d:
        work = pathlib.Path(d)
        if args.mode == "snippet":
            shutil.copy(args.file, work / "probe.cpp")
            failed = 0
            for n in names:
                ok, text = run(n, work, "probe.cpp", args.run, args.flag)
                failed += not ok
                print(f"== {n}: {'ok' if ok else 'FAIL'}")
                if text:
                    print("   " + text.replace("\n", "\n   "))
            return 1 if failed else 0

        # macros: print each macro's value per toolchain ("-" = not defined)
        lines = ["#include <version>", "#include <cstdio>", "#define S2(x) #x", "#define S(x) S2(x)",
                 "int main() {"]
        for mac in args.names:
            lines.append(f'  {{ const char* v = S({mac}); std::printf("%s\\n", v[0] == \'_\' ? "-" : v); }}')
        lines.append("}")
        (work / "probe.cpp").write_text("\n".join(lines) + "\n")
        cols = {}
        for n in names:
            ok, text = run(n, work, "probe.cpp", True, [])
            cols[n] = text.splitlines() if ok else ["error"] * len(args.names)
        w = max(len(x) for x in args.names)
        print(f"{'macro':<{w}}  " + "  ".join(f"{n:<9}" for n in names))
        for i, mac in enumerate(args.names):
            vals = [cols[n][i] if i < len(cols[n]) else "?" for n in names]
            print(f"{mac:<{w}}  " + "  ".join(f"{v:<9}" for v in vals))
        return 0


if __name__ == "__main__":
    sys.exit(main())
