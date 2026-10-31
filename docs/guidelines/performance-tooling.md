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

**Record enough to reproduce a result** (Per.6 asks for measurements, so make
them checkable). With every number, keep:

| What | Examples |
|---|---|
| Build | commit, compiler and standard library versions, flags, LTO/PGO on or off |
| Machine | CPU model, cores/NUMA layout, memory, OS and kernel |
| Conditions | CPU pinning, frequency governor, other load on the box |
| Experiment | input sizes and distribution, warm-up, repetitions, run order |
| Result | the change *and* its spread; the statistical test if you used one |
| Correctness | the tests that show the faster version still does the same thing |

Keep a quiet, controlled machine for spotting changes, and a production-like
one for deciding whether they matter.

## Benchmarking

Use a benchmark library rather than timing loops by hand — it handles warm-up,
iteration counts, and statistics.

**[Google Benchmark](https://github.com/google/benchmark)**

```cpp nocompile
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

Compare two runs with the library's
[`tools/compare.py`](https://github.com/google/benchmark/blob/main/docs/tools.md).
Besides the percentage change, it runs a **Mann–Whitney U test** on the
repetitions and prints a p-value. Read it carefully:

- **The docs warn the test needs at least 9 repetitions** to mean anything
  (`--benchmark_repetitions=9` or more). That's a floor, not a guarantee.
- **A low p-value says the two runs differ, not by how much.** Report the size
  of the change and its spread too; a significant 0.3% may not be worth the
  code.

**[nanobench](https://github.com/martinus/nanobench)** is a single-header
alternative that's quick to drop into a test.

**Benchmarking traps:**

- **Dead-code elimination.** If the result isn't used, the compiler may delete
  the work. Use `DoNotOptimize` (or nanobench's `doNotOptimizeAway`). If a
  result looks too good, check the assembly (`perf annotate` or Compiler
  Explorer) to confirm the work is still there.
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

**Pick the stack unwinder on purpose** (`perf record --call-graph <mode>`;
see the [perf-record manual](https://man7.org/linux/man-pages/man1/perf-record.1.html)):

| Mode | How it works | Limits |
|---|---|---|
| `fp` (default for user space; `-g`) | Follows frame pointers | Gives bogus call graphs for code built without frame pointers |
| `dwarf` | Copies part of the user stack with each sample and unwinds it later from DWARF CFI | Needs perf built with libunwind or libdw; copies 8192 bytes of stack by default (`--call-graph dwarf,16384` for deeper stacks), so very deep stacks get cut off; large `perf.data` files |
| `lbr` | Reads Intel's hardware Last Branch Record | Newer Intel CPUs only (Haswell on), user-space call chains only, can't be combined with branch-stack sampling |

LBR is not a portable replacement for DWARF. Kernel stacks are unwound by
whatever unwinder the kernel was built with (frame pointers or ORC),
whichever user-space mode you pick.

**Frame pointers cost something, but how much depends on the workload.**
Fedora's measurements for its Fedora 38 change (Fedora 37 systems, 2022):
kernel compile with GCC 2.4% slower, one Blender render 2% slower,
CPython benchmarks 1–10% slower depending on the benchmark, and openssl,
botan, zstd and Redis not significantly affected. Those are old numbers for
those programs, not a figure for your C++ code; measure your own. Red Hat's
knowledge-base answer (updated 2024) argues frame pointers make almost no
difference on register-rich targets like x86-64 and greatly help profiling.

Fedora 38 turned on `-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer` for
distribution packages, with a per-package opt-out. That covers the distro's
libraries, not your own binaries — build those with frame pointers yourself.

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

When you share one, say what width means: the event (on-CPU cycles, off-CPU
time, allocations), the sampling rate, how long it ran, and any filtering.
Don't compare the shapes of two graphs recorded with different unwinders —
frame-pointer gaps can make a function look cheaper than it is.

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
  Valgrind's two heap profilers,
  [Massif](https://valgrind.org/docs/manual/ms-manual.html) (heap size over
  time) and [DHAT](https://valgrind.org/docs/manual/dh-manual.html) (how
  allocations are used), are for investigation runs, not production. Compare
  tools on the same workload: allocation rate and peak/retained memory answer
  different questions.
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
  (PGO) often give more than micro-tuning. Label every result with its build
  variant (plain `-O2`, LTO, PGO, BOLT); a speedup someone else published
  without the benchmark, hardware and baseline doesn't transfer to your code.

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
- [Google Benchmark user guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md) ·
  [tools (`compare.py`, U test)](https://github.com/google/benchmark/blob/main/docs/tools.md)
- [perf-record manual](https://man7.org/linux/man-pages/man1/perf-record.1.html) (call-graph modes)
- [Fedora change: frame pointers by default](https://fedoraproject.org/wiki/Changes/fno-omit-frame-pointer) (Fedora 38, with measurements)
