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
