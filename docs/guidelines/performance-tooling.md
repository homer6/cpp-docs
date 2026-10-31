# Performance tooling — measure before you optimize

> How to benchmark, profile, and read the code the compiler generates, so
> performance work starts from data. **Sources:** C++ Core Guidelines (Per),
> Google Benchmark docs, [perf-report](https://github.com/homer6/perf-report) ·
> **Platform:** mostly Linux; tools noted where they're cross-platform

The Core Guidelines are blunt about it: **Per.1** don't optimize without
reason, **Per.2** don't optimize prematurely, **Per.6** don't make claims about
performance without measurements. This page is the "how to measure" part.

For Linux system-level analysis — `perf`, bpftrace/BCC one-liners, flame
graphs, and USE-method checks for CPU, memory, disk, and network — see
**perf-report** ([perf-report.com](https://perf-report.com),
[GitHub](https://github.com/homer6/perf-report), also checked out here at
[`modules/perf-report`](../../modules/perf-report)). This page links to it
rather than repeating it.

## The workflow

1. **State the goal as a number.** "p99 latency under 2 ms at 10k req/s", not
   "make it faster".
2. **Measure the whole system first.** Is the process even CPU-bound? Check
   with perf-report's
   [60-second analysis](https://github.com/homer6/perf-report/blob/main/docs/analysis/CPU.md)
   (`vmstat 1`, `mpstat -P ALL 1`, `pidstat 1`) before profiling code.
3. **Profile to find where the time goes.** CPU profile, then off-CPU if it's
   waiting.
4. **Benchmark the hot spot in isolation** to try fixes quickly.
5. **Re-measure the whole system** — a micro-benchmark win can disappear (or
   reverse) in production.

## Benchmarking

Use a benchmark library rather than timing loops by hand — it handles warm-up,
iteration counts, and statistics.

**[Google Benchmark](https://github.com/google/benchmark)**

```cpp
#include <benchmark/benchmark.h>
#include <vector>

static void BM_push_back(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> v;
        for (int i = 0; i < state.range(0); ++i) v.push_back(i);
        benchmark::DoNotOptimize(v.data());   // keep the result "used"
        benchmark::ClobberMemory();           // force writes to memory
    }
}
BENCHMARK(BM_push_back)->Range(8, 8 << 10);
BENCHMARK_MAIN();
```

Useful flags:

| Flag | Use |
|---|---|
| `--benchmark_repetitions=10` | Run several times and report mean/median/stddev |
| `--benchmark_report_aggregates_only=true` | Show only the aggregates |
| `--benchmark_enable_random_interleaving` | Interleave benchmarks to reduce ordering effects |
| `--benchmark_min_time=0.5s` | Minimum time per benchmark |
| `--benchmark_out=results.json --benchmark_out_format=json` | Save results to compare later |

Compare two runs with the library's `tools/compare.py`.

**[nanobench](https://github.com/martinus/nanobench)** is a single-header
alternative that's quick to drop into a test.

**Benchmarking traps:**

- **Dead-code elimination.** If the result isn't used, the compiler may delete
  the work. Use `DoNotOptimize` (or nanobench's `doNotOptimizeAway`).
- **Unrealistic data.** Tiny inputs fit in L1 cache; sorted inputs make
  branches predictable. Benchmark with production-like sizes and distributions.
- **Noisy machines.** Pin CPU frequency where you can, close other work, use
  repetitions, and look at the spread, not just the mean.
- **Debug builds.** Always benchmark the optimized build you ship.

## CPU profiling with `perf`

Build with symbols and frame pointers so stacks are complete:

```text
-O2 -g -fno-omit-frame-pointer
```

```bash
perf record -g ./app            # sample with call stacks (frame pointers)
perf record --call-graph dwarf ./app   # if frame pointers aren't available
perf report                     # interactive: hottest functions and callers
perf stat -e cycles,instructions,cache-misses,branch-misses ./app   # counters
```

- **Low instructions per cycle** (`instructions / cycles` well under 1) usually
  means the CPU is waiting on memory — look at data layout, not arithmetic.
- **`perf top`** shows a live profile of the whole machine — perf-report has a
  [tuned invocation](https://github.com/homer6/perf-report/blob/main/docs/analysis/CPU.md).

### Flame graphs

Turn `perf` samples into a picture where width is time spent:

```bash
perf record -F 99 -g ./app
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

The scripts are in Brendan Gregg's
[FlameGraph](https://github.com/brendangregg/FlameGraph) repo. Look for wide
plateaus — that's where the time goes.

### Off-CPU and tracing

If the program is slow but the CPU is idle, it's waiting: locks, I/O, page
faults. bpftrace and BCC can show where threads block. perf-report collects
one-liners for this:

- [CPU / code paths](https://github.com/homer6/perf-report/blob/main/docs/analysis/CPU.md) — stack sampling, context switches, run-queue latency
- [Memory](https://github.com/homer6/perf-report/blob/main/docs/analysis/Memory.md) — page faults, cache misses, allocation tracing
- [Filesystem & disk](https://github.com/homer6/perf-report/blob/main/docs/analysis/FilesystemDisk.md)
- [Network](https://github.com/homer6/perf-report/blob/main/docs/analysis/Network.md)
- [Installing the tools](https://github.com/homer6/perf-report/blob/main/docs/analysis/Install.md)

## Memory and allocation

- **Heap profiling:** [heaptrack](https://github.com/KDE/heaptrack)
  (`heaptrack ./app`) shows which call stacks allocate most, and how often.
  Allocator profilers in jemalloc and tcmalloc are alternatives in production.
- **Allocation count matters as much as bytes.** Many small allocations in a
  hot loop are a common C++ cost — `reserve()`, reuse buffers, or use
  `std::pmr` / arena allocation.
- **Cache behaviour:** `perf stat -e cache-misses` and the perf-report memory
  one-liners show whether data layout is the problem. Contiguous data
  (`std::vector`, struct-of-arrays) usually beats node-based containers.

## Reading the generated code

- **[Compiler Explorer](https://godbolt.org)** shows the assembly for a
  snippet across compilers and flags — the fastest way to check whether an
  abstraction is free.
- **`perf annotate`** shows the hot instructions inside a function from a real
  profile.
- **Optimization levels:** compare `-O2` and `-O3` rather than assuming `-O3`
  is faster. Link-time optimization (`-flto`) and profile-guided optimization
  (PGO) often give more than micro-tuning.

## C++ costs to check first

Before rewriting anything, look for these common, well-understood costs:

| Cost | What to look for | Guidance |
|---|---|---|
| Needless copies | pass-by-value of large types, missing `std::move`, copies in range-`for` | [F.15–F.17](core-guidelines.md#functions-f) |
| Allocation in hot loops | `push_back` without `reserve`, temporary strings | Reuse buffers; `std::string_view` |
| Type erasure | `std::function` and virtual calls in tight loops | Templates/concepts, or `std::function_ref` (C++26) |
| Shared ownership | `std::shared_ptr` copies (atomic ref-count updates) | Pass `const T&` or `T*` to non-owning code (R.30–R.37) |
| `std::regex` | regex in a hot path | Third-party library (SL.regex) |
| Reader/writer locks | `std::shared_mutex` assumed faster | Usually it isn't — measure (Abseil [#197](https://abseil.io/tips/197)) |
| Lazy ranges | a `filter`/`transform` view iterated more than once | Materialize with `std::ranges::to` ([design principles](design-principles.md)) |
| Exceptions on hot paths | throwing for routine failures | `std::expected` for expected failures |

## See also

- [Core Guidelines — Performance](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-performance) ·
  [digest](core-guidelines.md#performance-per)
- [perf-report](https://github.com/homer6/perf-report) ·
  [Brendan Gregg — perf examples](https://www.brendangregg.com/perf.html)
- [Google Benchmark user guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
