#!/usr/bin/env python3
"""Turn a ctest run of the GoogleTest suite into machine-readable JSON.

Reads a CMake build tree (the toolchain, platform and CMake version come from
its CMakeCache.txt and CMakeFiles/<version>/) and the test results, from a
JUnit file (`ctest --output-junit FILE`) when one is given, or found as
<build-dir>/junit.xml and newer than the log, otherwise from Testing/Temporary/LastTest.log, which
every ctest run writes. Skip reasons come from the GoogleTest output
(GTEST_SKIP() << "reason", CPPDOCS_REQUIRE), as tools/run_tests.sh prints them.

    tools/ctest_to_json.py --build-dir build/gcc-15 -o results.json
    tools/ctest_to_json.py --build-dir build/msvc --junit build/msvc/junit.xml

`tools/run_tests.sh --json FILE <preset>` runs it after the tests. The output
(schema "cpp-docs/results/1", kind "tests") shares its platform and compiler
fields with `tools/check_snippets.py --json` (kind "snippets"):

    {
      "schema": "cpp-docs/results/1", "kind": "tests",
      "preset": "gcc-15", "toolchain": "",
      "platform": {"system": "Darwin", "version": "23.6.0", "arch": "arm64", "host": "..."},
      "compiler": {"id": "GNU", "version": "15.2.0", "path": "...", "frontend": "GNU",
                   "stdlib": "libstdc++"},
      "cmake": "4.3.4", "source": "LastTest.log",
      "counts": {"total": 1234, "passed": 1200, "failed": 0, "skipped": 34},
      "failures": [{"name": "Suite.Test", "labels": [], "message": "...", "output": "..."}],
      "skips": [{"name": "Suite.Test", "labels": [], "reason": "needs __cpp_lib_hive ..."}],
      "generated": "2026-11-01T12:00:00Z"
    }

Exit status is 0 when the JSON was written, 2 if there are no results to read.
"""
from __future__ import annotations

import argparse
import datetime as dt
import json
import pathlib
import platform
import re
import sys
import xml.etree.ElementTree as ET

SCHEMA = "cpp-docs/results/1"
OUTPUT_TAIL = 40  # lines of a failing test's output kept in the JSON


def read_cmake_sets(path: pathlib.Path) -> dict[str, str]:
    """Values of set(NAME "value") lines in a CMakeFiles/<version>/*.cmake file."""
    vals: dict[str, str] = {}
    if path.is_file():
        for m in re.finditer(r'^set\((\w+) "([^"]*)"\)', path.read_text(encoding="utf-8", errors="replace"), re.M):
            vals.setdefault(m[1], m[2])
    return vals


def read_cache(build: pathlib.Path) -> dict[str, str]:
    vals: dict[str, str] = {}
    cache = build / "CMakeCache.txt"
    if cache.is_file():
        for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
            m = re.match(r"^([\w.-]+):\w+=(.*)$", line)
            if m:
                vals[m[1]] = m[2]
    return vals


def guess_stdlib(compiler_id: str, frontend: str, system: str, flags: str) -> str:
    if frontend == "MSVC" or compiler_id == "MSVC":
        return "MS STL"
    if "-stdlib=libc++" in flags:
        return "libc++"
    if "-stdlib=libstdc++" in flags or compiler_id == "GNU":
        return "libstdc++"
    if system in ("Darwin", "FreeBSD") or compiler_id == "AppleClang":
        return "libc++"
    return "libstdc++" if compiler_id == "Clang" else "unknown"


def toolchain_info(build: pathlib.Path) -> dict:
    """Platform and compiler of a configured build tree (empty fields if not configured)."""
    cache = read_cache(build)
    files = sorted(build.glob("CMakeFiles/*/CMakeCXXCompiler.cmake"))
    cxx = read_cmake_sets(files[-1]) if files else {}
    system = read_cmake_sets(files[-1].parent / "CMakeSystem.cmake") if files else {}
    flags = " ".join(cache.get(k, "") for k in ("CMAKE_CXX_FLAGS", "CMAKE_CXX_FLAGS_INIT"))
    return {
        "toolchain": cache.get("CPPDOCS_TOOLCHAIN", ""),
        "platform": {
            "system": system.get("CMAKE_SYSTEM_NAME", platform.system()),
            "version": system.get("CMAKE_SYSTEM_VERSION", platform.release()),
            "arch": system.get("CMAKE_SYSTEM_PROCESSOR", platform.machine()),
            "host": platform.platform(),
        },
        "compiler": {
            "id": cxx.get("CMAKE_CXX_COMPILER_ID", ""),
            "version": cxx.get("CMAKE_CXX_COMPILER_VERSION", ""),
            "path": cxx.get("CMAKE_CXX_COMPILER", cache.get("CMAKE_CXX_COMPILER", "")),
            "frontend": cxx.get("CMAKE_CXX_COMPILER_FRONTEND_VARIANT", ""),
            "stdlib": guess_stdlib(cxx.get("CMAKE_CXX_COMPILER_ID", ""),
                                   cxx.get("CMAKE_CXX_COMPILER_FRONTEND_VARIANT", ""),
                                   system.get("CMAKE_SYSTEM_NAME", ""), flags),
        },
        "cmake": files[-1].parent.name if files else "",
    }


# --- GoogleTest output ------------------------------------------------------

def skip_reason(output: str) -> str:
    """The message after gtest's "<file>:<line>: Skipped" (or "file(line): Skipped")."""
    lines = output.splitlines()
    for i, line in enumerate(lines):
        if line.rstrip().endswith(": Skipped"):
            reason = []
            for nxt in lines[i + 1:]:
                if not nxt.strip() or nxt.startswith("[  SKIPPED ]"):
                    break
                reason.append(nxt.strip())
            return " ".join(reason) or "(no reason given)"
    return "(no reason given)"


def failure_message(output: str) -> str:
    """gtest's failure blocks (the failure line and what follows it)."""
    lines = output.splitlines()
    out: list[str] = []
    i = 0
    while i < len(lines):
        # GCC/Clang builds: "<file>:<line>: Failure"; MSVC: "<file>(<line>): error: ...".
        if re.search(r": Failure\s*$|\(\d+\): error: ", lines[i]) or "C++ exception" in lines[i]:
            block = [lines[i]]
            i += 1
            while i < len(lines) and lines[i].strip() and not lines[i].startswith("["):
                block.append(lines[i])
                i += 1
            out.append("\n".join(block))
        else:
            i += 1
    return "\n\n".join(out)


def tail(text: str, n: int = OUTPUT_TAIL) -> str:
    return "\n".join(text.splitlines()[-n:])


# --- result sources ---------------------------------------------------------

def from_junit(path: pathlib.Path) -> list[dict]:
    results = []
    for tc in ET.parse(path).getroot().iter("testcase"):
        out = tc.findtext("system-out") or ""
        labels = [p.get("value", "") for p in tc.iter("property") if p.get("name") == "cmake_labels"]
        labels = [l for v in labels for l in v.split(";") if l]
        status = tc.get("status", "run")
        fail = tc.find("failure")
        if tc.find("skipped") is not None or status in ("notrun", "disabled"):
            kind = "skipped"
        elif fail is not None or status == "fail":
            kind = "failed"
        else:
            kind = "passed"
        results.append({"name": tc.get("name", ""), "labels": labels, "status": kind,
                        "output": out, "ctest_message": fail.get("message", "") if fail is not None else ""})
    return results


def from_last_test_log(path: pathlib.Path) -> list[dict]:
    text = path.read_text(encoding="utf-8", errors="replace").replace("\r\n", "\n")
    results = []
    # Each test: "N/M Test: <name>" ... "Output:" ---- <output> "<end of output>" ...
    # "Test Passed." / "Test Failed." / ..., then an optional "Test Pass Reason:".
    for m in re.finditer(r"^\d+/\d+ Test: (?P<name>.+?)\n(?P<body>.*?)(?=^\d+/\d+ Testing: |\Z)", text, re.M | re.S):
        body = m["body"]
        om = re.search(r"^Output:\n-+\n(.*?)^<end of output>", body, re.M | re.S)
        out = om[1] if om else ""
        sm = re.search(r"^Test (Passed|Failed|Not Run|Skipped)\b.*$", body, re.M)
        verdict = sm[1] if sm else "Failed"
        skipped = "Skip regular expression found" in body or verdict in ("Skipped", "Not Run") and "[  SKIPPED ]" in out
        kind = "skipped" if skipped else ("passed" if verdict == "Passed" else "failed")
        reason = re.search(r"^Test (?:Fail|Pass) Reason:\n(.*)$", body, re.M)
        results.append({"name": m["name"].strip(), "labels": [], "status": kind, "output": out,
                        "ctest_message": reason[1].strip() if reason and kind == "failed" else ""})
    return results


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build-dir", type=pathlib.Path, required=True, help="the CMake build tree, e.g. build/gcc-15")
    ap.add_argument("--junit", type=pathlib.Path, help="ctest --output-junit file (default: <build-dir>/junit.xml if it's newer than LastTest.log)")
    ap.add_argument("--log", type=pathlib.Path, help="LastTest.log (default: <build-dir>/Testing/Temporary/LastTest.log)")
    ap.add_argument("--preset", help="preset name to record (default: the build directory's name)")
    ap.add_argument("-o", "--output", type=pathlib.Path, help="write here instead of stdout")
    args = ap.parse_args()

    build = args.build_dir
    log = args.log or build / "Testing" / "Temporary" / "LastTest.log"
    junit = args.junit
    found = build / "junit.xml"
    if not junit and not args.log and found.is_file() and (
            not log.is_file() or found.stat().st_mtime >= log.stat().st_mtime):
        junit = found  # only if it's from the latest run
    if junit:
        results, source = from_junit(junit), "junit"
    elif log.is_file():
        results, source = from_last_test_log(log), "LastTest.log"
    else:
        print(f"ctest_to_json.py: no test results in {build} (no junit.xml or LastTest.log)", file=sys.stderr)
        return 2

    failures = [{"name": r["name"], "labels": r["labels"],
                 "message": failure_message(r["output"]) or r["ctest_message"],
                 "output": tail(r["output"])} for r in results if r["status"] == "failed"]
    skips = [{"name": r["name"], "labels": r["labels"], "reason": skip_reason(r["output"])}
             for r in results if r["status"] == "skipped"]
    doc = {
        "schema": SCHEMA,
        "kind": "tests",
        "preset": args.preset or build.resolve().name,
        **toolchain_info(build),
        "source": source,
        "counts": {
            "total": len(results),
            "passed": sum(r["status"] == "passed" for r in results),
            "failed": len(failures),
            "skipped": len(skips),
        },
        "failures": sorted(failures, key=lambda f: f["name"]),
        "skips": sorted(skips, key=lambda s: s["name"]),
        "generated": dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    }
    text = json.dumps(doc, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
        c = doc["counts"]
        print(f"wrote {args.output}: {c['total']} tests, {c['passed']} passed, "
              f"{c['failed']} failed, {c['skipped']} skipped")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
