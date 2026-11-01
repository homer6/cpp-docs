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
    ```cpp module           a C++20 module unit (or a file that imports one),
                            compiled WITHOUT the prelude, with modules on
    ```cpp module=geo       module units that build together: every block in the
                            same file with the same group name is compiled in
                            document order in one directory, so later blocks see
                            the BMIs of earlier ones (interface, then
                            partitions/implementation units, then importers)

Tokens can be combined: ```cpp body needs=contracts. Quote values that
contain spaces.

Module blocks are compiled with -c (not -fsyntax-only), because importers need
the BMI an interface unit writes. Modules are enabled per compiler: GCC gets
-fmodules; Clang compiles interface and partition units as .cppm with
-fmodule-output and finds them with -fprebuilt-module-path. `import std;` and
`import std.compat;` build the standard library module once per run (GCC:
bits/std.cc; Clang: the std.cppm that libc++.modules.json names, or
--std-module-json). `import <header>;` builds that header unit first.
compile-fail works per block inside a group; a failing block provides no BMI,
so make it an importer.

Toolchains (--toolchain; default auto, which tells gcc from clang by
`--cxx --version`):

    gcc, clang   GCC-style command line, as above; the prelude is #included.
    msvc         cl /nologo /std:c++latest /permissive- /EHsc /Zs /utf-8
                 /Zc:__cplusplus /Zc:preprocessor /FI<prelude>  (run it from a
                 VS Developer shell). --std c++26 becomes /std:c++latest.
                 compile-fail blocks must fail, but their regex isn't matched
                 (it's written against GCC/Clang wording).
    clang-cl     clang-cl /nologo /std:c++latest /permissive- /EHsc /Zs /utf-8
                 /FI<prelude>; compile-fail regexes are matched.

Fence tags mean the same for every toolchain. Module blocks are checked with
gcc and clang only; with msvc and clang-cl they skip with that reason (a
separate step). Each toolchain has its own baseline file in
tools/snippet_baseline/ (gcc.txt, clang.txt, msvc.txt, clang-cl.txt).

--json FILE also writes the results as JSON (schema "cpp-docs/results/1",
kind "snippets": platform, compiler, command, counts, failures, skips with
reasons, baseline), the same shape tools/ctest_to_json.py writes for tests.

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
    tools/check_snippets.py [paths...] [--toolchain auto|gcc|clang|msvc|clang-cl]
                            [--cxx g++-16] [--std c++26]
                            [--flag=-freflection ...] [--enable reflection ...]
                            [--baseline FILE [--update-baseline]]
                            [--std-module-json FILE]
                            [--json FILE] [--report-only] [-j N]

Paths default to docs/. Exit status is 1 if any checked block fails (or, with
--baseline, on new failures and stale entries), unless --report-only is given.
"""
from __future__ import annotations

import argparse
import concurrent.futures as cf
import json
import os
import pathlib
import re
import shlex
import subprocess
import sys
import shutil
import tempfile
import threading

ROOT = pathlib.Path(__file__).resolve().parent.parent
PRELUDE = ROOT / "tools" / "snippet_prelude.hpp"
FENCE = re.compile(r"^(?P<indent>\s*)(?P<fence>`{3,}|~{3,})(?P<info>.*)$")


HEADING = re.compile(r"^#{1,6}\s+(.*?)\s*#*\s*$")


def rel(path: pathlib.Path) -> str:
    """Repo-relative path with forward slashes on every OS (IDs, #line, baselines)."""
    return path.relative_to(ROOT).as_posix()


def slug(heading: str) -> str:
    """GitHub-style heading anchor."""
    h = re.sub(r"[`*_]|\[([^\]]*)\]\([^)]*\)", r"\1", heading).strip().lower()
    return re.sub(r"[^\w\- ]", "", h).replace(" ", "-") or "top"


class Block:
    def __init__(self, path: pathlib.Path, line: int, info: list[str], code: str, ident: str = ""):
        self.path, self.line, self.info, self.code, self.ident = path, line, info, code, ident

    @property
    def where(self) -> str:
        return f"{rel(self.path)}:{self.line}"

    @property
    def id(self) -> str:
        return f"{rel(self.path)}#{self.ident}"

    def needs(self) -> set[str]:
        return {t.split("=", 1)[1] for t in self.info if t.startswith("needs=")}

    def module_group(self) -> str | None:
        """None for an ordinary block; otherwise the module group's key."""
        for t in self.info:
            if t == "module":
                return f"{self.path}:{self.line}"  # a group of its own
            if t.startswith("module="):
                return f"{self.path}#{t.split('=', 1)[1]}"
        return None

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


# --- Toolchains ---------------------------------------------------------------
#
# gcc and clang take GCC-style command lines (-std=, -fsyntax-only); the
# prelude is #included at the top of each block. msvc (cl) and clang-cl take
# cl-style ones (/std:, /Zs for a syntax-only check) and get the prelude with
# /FI. cl writes its diagnostics to stdout, so both streams are read.

TOOLCHAINS = ("gcc", "clang", "msvc", "clang-cl")
DEFAULT_CXX = {"msvc": "cl", "clang-cl": "clang-cl"}


class Toolchain:
    def __init__(self, name: str, cxx: str, std: str, flags: list[str]):
        self.name, self.cxx, self.std, self.flags = name, cxx, std, flags

    @property
    def cl_style(self) -> bool:
        return self.name in ("msvc", "clang-cl")

    def cl_std(self) -> str:
        # cl has no /std:c++26; its newest mode is /std:c++latest.
        return "c++latest" if self.std in ("c++26", "c++2c", "c++latest") else self.std

    def syntax_cmd(self) -> list[str]:
        """The syntax-only command for an ordinary block (the file name goes last)."""
        if self.name == "msvc":
            return [self.cxx, "/nologo", f"/std:{self.cl_std()}", "/permissive-", "/EHsc", "/Zs", "/utf-8",
                    "/Zc:__cplusplus", "/Zc:preprocessor", f"/FI{PRELUDE}", *self.flags]
        if self.name == "clang-cl":
            return [self.cxx, "/nologo", f"/std:{self.cl_std()}", "/permissive-", "/EHsc", "/Zs", "/utf-8",
                    f"/FI{PRELUDE}", *self.flags]
        return [self.cxx, f"-std={self.std}", "-fsyntax-only", "-pedantic-errors", *self.flags]

    def source_for(self, block: Block) -> str:
        code = block.code
        if "body" in block.info:
            code = "void snippet_body_() {\n" + code + "}\n"
        # #line maps compiler diagnostics back to the Markdown file.
        first = block.line + 1 if "body" not in block.info else block.line
        line = f'#line {first} "{rel(block.path)}"\n{code}'
        return line if self.cl_style else f'#include "{PRELUDE}"\n{line}'

    def diagnostics(self, r: subprocess.CompletedProcess) -> str:
        return (r.stdout + r.stderr) if self.cl_style else r.stderr

    def checks_regex(self) -> bool:
        """compile-fail="regex" is written against GCC and Clang diagnostics;
        on msvc any error satisfies it (clang-cl prints Clang's diagnostics)."""
        return self.name != "msvc"

    def module_skip(self) -> str | None:
        """Why module blocks aren't checked with this toolchain (None: they are)."""
        if self.cl_style:
            return f"module blocks aren't checked with {self.name} yet"
        return None

    def version(self) -> str:
        try:
            if self.name == "msvc":  # cl has no --version; its banner goes to stderr
                r = subprocess.run([self.cxx], capture_output=True, text=True, timeout=30)
                out = (r.stderr or r.stdout).strip()
            else:
                out = subprocess.run([self.cxx, "--version"], capture_output=True, text=True, timeout=30).stdout
            return out.splitlines()[0] if out else "unknown"
        except (OSError, subprocess.SubprocessError):
            return "unknown"


def detect_toolchain(cxx: str) -> str:
    """gcc or clang for a GCC-style driver, from its --version banner."""
    base = pathlib.Path(cxx).name.lower()
    if base.startswith("clang-cl"):
        return "clang-cl"
    if base in ("cl", "cl.exe"):
        return "msvc"
    try:
        out = subprocess.run([cxx, "--version"], capture_output=True, text=True, timeout=30).stdout
    except (OSError, subprocess.SubprocessError):
        out = ""
    return "clang" if "clang" in out.lower() else "gcc"


def check(block: Block, tc: Toolchain) -> tuple[Block, bool, str]:
    with tempfile.NamedTemporaryFile("w", suffix=".cpp", delete=False, encoding="utf-8") as f:
        f.write(tc.source_for(block))
        name = f.name
    try:
        r = subprocess.run(tc.syntax_cmd() + [name], capture_output=True, text=True, timeout=120)
        err = tc.diagnostics(r)
        expect = block.compile_fail()
        if expect is None:
            return block, r.returncode == 0, err
        if r.returncode == 0:
            return block, False, "compile-fail block compiled without errors"
        if expect and tc.checks_regex() and not re.search(expect, err):
            return block, False, f"compile-fail block failed, but no diagnostic matched /{expect}/:\n{err}"
        return block, True, ""
    finally:
        os.unlink(name)


# --- C++20 module blocks (```cpp module / module=<group>) -------------------

MODULE_DECL = re.compile(r"^\s*(export\s+)?module\s+([A-Za-z_][\w.]*)(?:\s*:\s*([A-Za-z_][\w.]*))?\s*(?:\[\[.*\]\])?\s*;",
                         re.M)
IMPORT_STD = re.compile(r"^\s*(?:export\s+)?import\s+(std(?:\.compat)?)\s*;", re.M)
HEADER_UNIT = re.compile(r"^\s*(?:export\s+)?import\s*<([^>]+)>\s*;", re.M)
QUOTED_UNIT = re.compile(r'^\s*(?:export\s+)?import\s*"[^"]+"\s*;', re.M)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", lambda m: "\n" * m[0].count("\n"), code, flags=re.S)
    return re.sub(r"//[^\n]*", "", code)


class ModuleToolchain:
    """Per-compiler commands for module units. Builds std modules lazily, once."""

    def __init__(self, cxx: str, std: str, flags: list[str], std_json: str | None):
        self.cxx, self.std, self.flags, self.std_json = cxx, std, flags, std_json
        out = subprocess.run([cxx, "--version"], capture_output=True, text=True).stdout
        self.clang = "clang" in out.lower()
        self.base = [cxx, f"-std={std}", "-pedantic-errors", *flags]
        if not self.clang:
            self.base.append("-fmodules")
        self.cache = pathlib.Path(tempfile.mkdtemp(prefix="snippet-std-"))
        self.lock = threading.Lock()
        self.built: dict[str, pathlib.Path | str] = {}  # name -> BMI, or error text

    def close(self) -> None:
        shutil.rmtree(self.cache, ignore_errors=True)

    def _run(self, cmd: list[str], cwd: pathlib.Path) -> subprocess.CompletedProcess:
        return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=600)

    def _clang_std_sources(self) -> dict[str, pathlib.Path]:
        js = self.std_json
        if not js:
            r = subprocess.run([self.cxx, *self.flags, "-print-file-name=libc++.modules.json"],
                               capture_output=True, text=True)
            js = r.stdout.strip()
            if not os.path.isabs(js):  # e.g. Homebrew LLVM keeps it in lib/c++/
                exe = pathlib.Path(shutil.which(self.cxx) or self.cxx).resolve()
                js = str(exe.parent.parent / "lib" / "c++" / "libc++.modules.json")
        path = pathlib.Path(js)
        data = json.loads(path.read_text())
        return {m["logical-name"]: (path.parent / m["source-path"]).resolve() for m in data["modules"]}

    def std_module(self, name: str) -> pathlib.Path:
        """Build `std` or `std.compat` once; return the BMI path. Raises on failure."""
        with self.lock:
            for dep in (["std", "std.compat"] if name == "std.compat" else ["std"]):
                if dep not in self.built:
                    self.built[dep] = self._build_std(dep)
                if isinstance(self.built[dep], str):
                    raise RuntimeError(self.built[dep])
            return self.built[name]

    def _build_std(self, name: str) -> pathlib.Path | str:
        try:
            if self.clang:
                src = self._clang_std_sources()[name]
                bmi = self.cache / f"{name}.pcm"
                cmd = [*self.base, "-Wno-reserved-module-identifier", "--precompile", str(src), "-o", str(bmi)]
                if name == "std.compat":
                    cmd.append(f"-fmodule-file=std={self.cache / 'std.pcm'}")
            else:
                src = "bits/std.cc" if name == "std" else "bits/std.compat.cc"
                bmi = self.cache / "gcm.cache" / f"{name}.gcm"
                cmd = [*self.base, "-fsearch-include-path", "-c", src, "-o", str(self.cache / f"{name}.o")]
            r = self._run(cmd, self.cache)
        except (OSError, KeyError, ValueError, subprocess.SubprocessError) as e:
            return f"can't build module {name}: {e}"
        return bmi if r.returncode == 0 else f"can't build module {name}:\n{r.stderr}"

    def header_unit(self, header: str, work: pathlib.Path) -> list[str]:
        """Build a system header unit in `work`; return flags importers need."""
        if self.clang:
            bmi = work / (re.sub(r"[^\w.]", "_", header) + ".pcm")
            cmd = [*self.base, "-Wno-pragma-system-header-outside-header", "-xc++-system-header",
                   "--precompile", header, "-o", str(bmi)]
            flags = [f"-fmodule-file={bmi}", "-Wno-experimental-header-units"]
        else:
            cmd = [*self.base, "-x", "c++-system-header", header]
            flags = []
        r = self._run(cmd, work)
        if r.returncode != 0:
            raise RuntimeError(f"can't build header unit <{header}>:\n{r.stderr}")
        return flags

    def compile_unit(self, block: Block, work: pathlib.Path, n: int, extra: list[str]) -> subprocess.CompletedProcess:
        code = strip_comments(block.code)
        decl = MODULE_DECL.search(code)
        provides = None
        if decl and (decl[1] or decl[3]):  # interface unit, or a partition of either kind
            provides = decl[2] + (f"-{decl[3]}" if decl[3] else "")
        src = work / f"unit{n}.{'cppm' if provides else 'cpp'}"
        src.write_text(block.code, encoding="utf-8")
        cmd = [*self.base, *extra, "-c", str(src), "-o", str(work / f"unit{n}.o")]
        if self.clang:
            cmd.append(f"-fprebuilt-module-path={work}")
            if provides:
                cmd.append(f"-fmodule-output={work / (provides + '.pcm')}")
        return self._run(cmd, work)


def check_module_group(group: list[Block], tc: ModuleToolchain) -> list[tuple[Block, bool, str]]:
    """Compile a group's blocks in order in one directory; one result per block."""
    work = pathlib.Path(tempfile.mkdtemp(prefix="snippet-mod-"))
    results: list[tuple[Block, bool, str]] = []
    extra: list[str] = []
    try:
        all_code = "\n".join(strip_comments(b.code) for b in group)
        try:
            if QUOTED_UNIT.search(all_code):
                raise RuntimeError('quoted header units (import "x.h";) are not supported by the checker')
            wanted = set(IMPORT_STD.findall(all_code))
            if "std.compat" in wanted:
                wanted.add("std")  # std.compat imports std
            for name in sorted(wanted):
                bmi = tc.std_module(name)
                if tc.clang:
                    extra.append(f"-fmodule-file={name}={bmi}")
                else:
                    (work / "gcm.cache").mkdir(exist_ok=True)
                    os.symlink(bmi, work / "gcm.cache" / bmi.name)
            for header in dict.fromkeys(HEADER_UNIT.findall(all_code)):
                extra += tc.header_unit(header, work)
        except RuntimeError as e:
            return [(b, False, str(e)) for b in group]
        for n, block in enumerate(group):
            r = tc.compile_unit(block, work, n, extra)
            # Map diagnostics back to the Markdown file.
            src = str(work / f"unit{n}.")
            err = re.sub(re.escape(src) + r"cppm?:(\d+)",
                         lambda m: f"{rel(block.path)}:{int(m[1]) + block.line}", r.stderr)
            expect = block.compile_fail()
            if expect is None:
                results.append((block, r.returncode == 0, err))
            elif r.returncode == 0:
                results.append((block, False, "compile-fail block compiled without errors"))
            elif expect and not re.search(expect, err):
                results.append((block, False, f"compile-fail block failed, but no diagnostic matched /{expect}/:\n{err}"))
            else:
                results.append((block, True, ""))
        return results
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("paths", nargs="*", default=["docs"])
    ap.add_argument("--toolchain", choices=("auto", *TOOLCHAINS), default="auto",
                    help="command-line style: gcc, clang, msvc (cl), clang-cl "
                         "(default auto: gcc or clang from --cxx --version)")
    ap.add_argument("--cxx", help="compiler (default: cl or clang-cl for those toolchains, else $CXX, else c++)")
    ap.add_argument("--std", default="c++26")
    ap.add_argument("--flag", action="append", default=[], help="extra compiler flag (repeatable)")
    ap.add_argument("--enable", action="append", default=[], help="feature for needs=... blocks (repeatable)")
    ap.add_argument("--baseline", type=pathlib.Path, help="file of block IDs allowed to fail")
    ap.add_argument("--update-baseline", action="store_true", help="rewrite --baseline with the current failures")
    ap.add_argument("--report-only", action="store_true", help="always exit 0")
    ap.add_argument("-j", type=int, default=os.cpu_count() or 2)
    ap.add_argument("-q", "--quiet", action="store_true", help="don't print compiler output")
    ap.add_argument("--std-module-json", help="Clang: libc++.modules.json to find std.cppm (default: ask the compiler)")
    ap.add_argument("--json", type=pathlib.Path, help="also write the results as JSON to this file")
    args = ap.parse_args()

    cxx = args.cxx or DEFAULT_CXX.get(args.toolchain) or os.environ.get("CXX") or "c++"
    name = detect_toolchain(cxx) if args.toolchain == "auto" else args.toolchain
    tc_plain = Toolchain(name, cxx, args.std, args.flag)

    files: list[pathlib.Path] = []
    for p in args.paths:
        p = (ROOT / p) if not os.path.isabs(p) else pathlib.Path(p)
        files += sorted(p.rglob("*.md")) if p.is_dir() else [p]

    blocks = [b for f in files for b in extract(f)]
    nocompile = [b for b in blocks if "nocompile" in b.info]
    gated = [b for b in blocks if b not in nocompile and not b.needs() <= set(args.enable)]
    # A module group builds as a unit: if one of its blocks is gated, skip them all.
    gated_groups = {b.module_group() for b in gated} - {None}
    gated += [b for b in blocks if b not in nocompile and b not in gated and b.module_group() in gated_groups]
    # Module blocks the toolchain can't check yet (msvc, clang-cl) skip with a reason.
    module_skip = tc_plain.module_skip()
    unsupported = [b for b in blocks if module_skip and b.module_group()
                   and b not in nocompile and b not in gated]
    todo = [b for b in blocks if b not in nocompile and b not in gated and b not in unsupported]
    groups: dict[str, list[Block]] = {}
    for b in todo:
        if b.module_group():
            groups.setdefault(b.module_group(), []).append(b)
    plain = [b for b in todo if not b.module_group()]

    cmd = tc_plain.syntax_cmd()
    failures: list[tuple[Block, str]] = []
    tc = ModuleToolchain(cxx, args.std, args.flag, args.std_module_json) if groups else None
    try:
        with cf.ThreadPoolExecutor(max_workers=args.j) as pool:
            for block, ok, err in pool.map(lambda b: check(b, tc_plain), plain):
                if not ok:
                    failures.append((block, err))
            for results in pool.map(lambda g: check_module_group(g, tc), groups.values()):
                failures += [(block, err) for block, ok, err in results if not ok]
    finally:
        if tc:
            tc.close()

    failures.sort(key=lambda x: (str(x[0].path), x[0].line))

    # Baseline: split failures into known and new; find fixed and stale entries.
    known: set[str] = set()
    base_lines: list[str] = []
    if args.baseline and args.baseline.exists():
        base_lines = args.baseline.read_text(encoding="utf-8").splitlines()
        known = {l.strip() for l in base_lines if l.strip() and not l.startswith("#")}
    checked_files = {rel(f) for f in files}
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
    n_mod = sum(len(g) for g in groups.values())
    n_skip = len(nocompile) + len(gated) + len(unsupported)
    extra = f", {len(unsupported)} module units: {module_skip}" if unsupported else ""
    print(f"\n{len(todo)} checked ({n_mod} module units in {len(groups)} groups), {len(failures)} failed, "
          f"{n_skip} skipped "
          f"({len(nocompile)} nocompile, {len(gated)} need {', '.join(missing) or '-'}{extra})")
    if args.baseline:
        print(f"baseline: {len(failures) - len(new)} known failures still failing, "
              f"{len(new)} new, {len(fixed)} fixed, {len(stale)} stale")
    version = tc_plain.version()
    print(f"compiler: {version}\ncommand: {' '.join(cmd)}")
    if tc:
        print(f"modules: {' '.join(tc.base[1:])} -c" + (" (.cppm + -fmodule-output, -fprebuilt-module-path)" if tc.clang else ""))
    if name == "msvc" and any(b.compile_fail() for b in todo):
        print("note: msvc: compile-fail blocks must fail, but their regexes (GCC/Clang wording) aren't matched")

    bad = (new or fixed or stale) if args.baseline else failures
    if args.json:
        write_json(args.json, {
            "toolchain": name, "cxx": cxx, "version": version, "command": cmd,
            "modules": ([*tc.base[1:], "-c"] if tc else None), "paths": args.paths,
            "enabled": args.enable, "todo": todo, "groups": groups, "failures": failures,
            "nocompile": nocompile, "gated": gated, "unsupported": unsupported,
            "module_skip": module_skip, "baseline": args.baseline, "new": new,
            "fixed": fixed, "stale": stale, "ok": not bad,
        })
    if args.report_only:
        return 0
    return 1 if bad else 0


def write_json(path: pathlib.Path, r: dict) -> None:
    """Machine-readable results; platform and compiler fields match tools/ctest_to_json.py."""
    import datetime as dt
    import platform

    enabled = set(r["enabled"])
    skips = ([{"id": b.id, "where": b.where, "reason": "nocompile"} for b in r["nocompile"]]
             + [{"id": b.id, "where": b.where,
                 "reason": "needs " + ", ".join(sorted(b.needs() - enabled)) if b.needs() - enabled
                 else "needs: another block of its module group is gated"} for b in r["gated"]]
             + [{"id": b.id, "where": b.where, "reason": r["module_skip"]} for b in r["unsupported"]])
    new_ids = {b.id for b, _ in r["new"]}
    doc = {
        "schema": "cpp-docs/results/1",
        "kind": "snippets",
        "toolchain": r["toolchain"],
        "platform": {"system": platform.system(), "version": platform.release(),
                     "arch": platform.machine(), "host": platform.platform()},
        "compiler": {"id": r["toolchain"], "version": r["version"], "path": r["cxx"]},
        "command": r["command"],
        "modules_command": r["modules"],
        "paths": r["paths"],
        "enabled": r["enabled"],
        "counts": {
            "total": len(r["todo"]) + len(r["nocompile"]) + len(r["gated"]) + len(r["unsupported"]),
            "checked": len(r["todo"]),
            "module_units": sum(len(g) for g in r["groups"].values()),
            "module_groups": len(r["groups"]),
            "failed": len(r["failures"]),
            "skipped": len(skips),
            "nocompile": len(r["nocompile"]),
            "gated": len(r["gated"]),
            "unsupported": len(r["unsupported"]),
        },
        "failures": [{"id": b.id, "where": b.where, "known": r["baseline"] is not None and b.id not in new_ids,
                      "message": "\n".join(e.strip().splitlines()[:40])} for b, e in r["failures"]],
        "skips": skips,
        "baseline": None if r["baseline"] is None else {
            "file": r["baseline"].as_posix(), "known_failing": len(r["failures"]) - len(r["new"]),
            "new": len(r["new"]), "fixed": r["fixed"], "stale": r["stale"]},
        "ok": r["ok"],
        "generated": dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")

if __name__ == "__main__":
    sys.exit(main())
