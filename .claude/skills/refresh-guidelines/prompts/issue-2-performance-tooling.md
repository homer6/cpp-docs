# Deep research prompt — C++ performance tooling (issue #2)

Paste everything below the line into Perplexity Deep Research. Save the report
as `.firecrawl/deep-research-perf-tooling-<YYYY-MM-DD>.md`, then ask Claude to
run the `refresh-guidelines` skill in **import** mode on it, targeting
`docs/guidelines/performance-tooling.md` (issue #2).

---

I maintain C++ coding guidelines for experienced engineers (C++20/23 baseline,
C++26 where available). I'm writing a page on **performance tooling for C++**:
benchmarking, CPU profiling, flame graphs, heap profiling, and reading
generated code. Linux is the primary platform; note macOS/Windows equivalents
where they matter. Today's date is {{DATE}}.

## Already covered (only report corrections, changes, or important nuance)

- Core Guidelines Per.1/Per.2/Per.6 (measure before optimizing)
- Google Benchmark basics: `DoNotOptimize`, `ClobberMemory`,
  `--benchmark_repetitions`, `--benchmark_min_time`, JSON output, `compare.py`
- `perf record -g`, `--call-graph dwarf`, `perf report`, `perf stat`,
  `perf annotate`, `-fno-omit-frame-pointer`
- Flame graphs via Brendan Gregg's FlameGraph scripts
- bpftrace/BCC one-liners for CPU, memory, disk, network (I link to my own
  collection, perf-report, for these)
- heaptrack, Compiler Explorer, `-O2` vs `-O3`, LTO, PGO
- Verified 2026-10-31: `compare.py` runs a Mann–Whitney U test and its docs
  warn it needs at least 9 repetitions; `--benchmark_enable_random_interleaving`;
  `--benchmark_min_time=0.5s` / `100x` syntax
- Verified 2026-10-31: perf `--call-graph fp|dwarf|lbr`, `fp` is the
  user-space default, DWARF copies 8192 bytes of stack by default
  (`dwarf,<size>`), LBR is newer-Intel-only, user-space only, and can't be
  combined with branch-stack sampling
- Verified 2026-10-31: Fedora's F38 frame-pointer change (accepted, per-package
  opt-out, re-evaluate by F40) and its F37 measurements (kernel build 2.4%,
  Blender 2%, CPython 1–10%, openssl/botan/zstd/Redis not significant);
  Red Hat KB 7041624 (updated 2024-10-09) position
- Valgrind's heap profilers are Massif and DHAT

## Still open from the last run (prioritize these)

- **Current** frame-pointer policy: did Fedora keep it after the F40 review?
  Ubuntu (24.04 enabled it on 64-bit?), Arch, Debian — with the official
  announcement for each.
- Google Benchmark warm-up option (`--benchmark_min_warmup_time`?) and whether
  random interleaving is still recommended; nanobench and Catch2 benchmark
  statistics and maintenance status, from their own docs.
- Profiler matrix (VTune, uProf, Tracy, Hotspot, samply, Instruments,
  WPA/ETW): licence, platforms, current release — from each project's own page.
- Measured overhead numbers for heaptrack, jemalloc/tcmalloc heap profiling,
  bytehound, with conditions.
- Continuous profiling for native C++ (Parca, Pyroscope, Cloud Profiler):
  which unwinders they support (frame pointers only? DWARF? .eh_frame?).
- LTO/PGO/AutoFDO/BOLT gains **with** benchmark, hardware and baseline from
  the primary paper or vendor doc (e.g. the BOLT CGO 2019 paper).
- Top-down analysis: `perf stat --topdown` and `toplev` usage from their docs.
- Any primary measurements for the C++ cost traps (question 8). The last
  report found none; say so again rather than guessing.

## Questions

1. **Benchmarking best practice (2024–2026):** What do Google Benchmark,
   nanobench, and Catch2/doctest benchmarking currently recommend for stable,
   comparable results? CPU frequency scaling, `taskset`/isolation, warm-up,
   statistical comparison (what does `compare.py` actually test?), and how to
   avoid dead-code elimination correctly. Any newer libraries worth listing?
2. **Frame pointers vs DWARF vs LBR:** current state of frame pointers in
   distributions (Fedora/Ubuntu enabling `-fno-omit-frame-pointer` by default),
   the cost of frame pointers, and when to use `--call-graph dwarf` or `lbr`.
   Include measured overhead with sources.
3. **Profilers beyond perf:** current recommendations for Intel VTune, AMD
   uProf, Tracy, Hotspot (KDAB), samply, Apple Instruments, Windows Performance
   Analyzer / ETW — what each is best for, licence, platform.
4. **Microarchitecture analysis:** top-down analysis (`perf stat --topdown`,
   `toplev`), what IPC thresholds practitioners use, and how to diagnose
   memory-bound vs front-end-bound C++ code.
5. **Heap and allocation profiling:** heaptrack vs jemalloc/tcmalloc profiling
   vs Valgrind massif/DHAT vs bytehound — overhead, production suitability.
6. **Continuous profiling in production:** Parca, Pyroscope/Grafana, Google
   Cloud Profiler, perf-based fleet profiling — what's mature for native C++?
7. **Compiler optimization levers:** current measured gains and pitfalls of
   LTO (thin vs full), PGO (instrumented vs sampling/AutoFDO), BOLT, and
   `-march`/`-mtune`, with sources.
8. **C++-specific cost traps:** any authoritative measurements (talks, papers,
   vendor docs) on `std::function`, `shared_ptr` ref-counting, exceptions,
   `std::regex`, iostreams vs `std::format`/`std::print`, range adaptors, and
   coroutine frame allocation.

## Output format

- Headings matching my page: **The workflow · Benchmarking · CPU profiling ·
  Flame graphs · Off-CPU and tracing · Memory and allocation · Reading the
  generated code · C++ costs to check first · Sources**.
- Each recommendation: **short imperative rule** → one-line reason → source(s)
  → platform/tool version → (optional) exact command or minimal `cpp` snippet.
- **Exact flags and commands only from the tool's own docs or man page**
  (perf wiki/man7.org, Google Benchmark docs, vendor docs). Don't invent flags.
- Where practitioners disagree, show both positions.
- Mark every number (overheads, speedups) with its source, date, and
  conditions, plus a confidence level.
- End with a candidate sources table: name, URL, best for, update frequency,
  licence.
