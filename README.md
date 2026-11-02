# cpp-docs

A hands-on reference for learning **modern C++** — the language and the standard
library, from C++11 through the latest standard (C++26) — organized as
bite-sized, example-driven Markdown notes, plus opinionated guidelines on how to
write C++ well.

The reference mirrors how [cppreference.com](https://cppreference.com/)
organizes C++. Facts can come from any good source — cppreference, the standard
draft, WG21 papers, compiler and library docs — as long as they've been
verified locally (the snippet compiles, and behaves as described when run).

[![Linux](https://github.com/homer6/cpp-docs/actions/workflows/linux.yml/badge.svg?branch=main&event=workflow_dispatch)](https://github.com/homer6/cpp-docs/actions/workflows/linux.yml)
[![macOS](https://github.com/homer6/cpp-docs/actions/workflows/macos.yml/badge.svg?branch=main&event=workflow_dispatch)](https://github.com/homer6/cpp-docs/actions/workflows/macos.yml)
[![Windows (MSVC)](https://github.com/homer6/cpp-docs/actions/workflows/windows-msvc.yml/badge.svg?branch=main&event=workflow_dispatch)](https://github.com/homer6/cpp-docs/actions/workflows/windows-msvc.yml)
[![Windows (clang-cl, MinGW)](https://github.com/homer6/cpp-docs/actions/workflows/windows-gnu.yml/badge.svg?branch=main&event=workflow_dispatch)](https://github.com/homer6/cpp-docs/actions/workflows/windows-gnu.yml)
[![FreeBSD](https://github.com/homer6/cpp-docs/actions/workflows/freebsd.yml/badge.svg?branch=main&event=workflow_dispatch)](https://github.com/homer6/cpp-docs/actions/workflows/freebsd.yml)

GitHub Actions runs are **manual only**: each one needs the owner's consent,
and local runs (GCC 15, Clang 22, GCC 16) stay the primary proof. The badges
show the last consented run on `main`.

## Layout

Everything is clickable. The reference lives in
[`docs/examples/`](docs/examples/README.md) (master index and progress); each
area has its own README.

### Standard library

<!-- index:library:start -->
- **[Containers](docs/examples/containers/README.md)** — vector, map, span, the lot
  - adaptors: [std::flat_map / std::flat_multimap](docs/examples/containers/adaptors/flat_map.md) · [std::flat_set / std::flat_multiset](docs/examples/containers/adaptors/flat_set.md) · [std::priority_queue](docs/examples/containers/adaptors/priority_queue.md) · [std::queue](docs/examples/containers/adaptors/queue.md) · [std::stack](docs/examples/containers/adaptors/stack.md)
  - associative: [std::map](docs/examples/containers/associative/map.md) · [std::multimap](docs/examples/containers/associative/multimap.md) · [std::multiset](docs/examples/containers/associative/multiset.md) · [std::set](docs/examples/containers/associative/set.md)
  - sequence: [std::array](docs/examples/containers/sequence/array.md) · [std::deque](docs/examples/containers/sequence/deque.md) · [std::forward_list](docs/examples/containers/sequence/forward_list.md) · [std::hive](docs/examples/containers/sequence/hive.md) · [std::inplace_vector](docs/examples/containers/sequence/inplace_vector.md) · [std::list](docs/examples/containers/sequence/list.md) · [std::vector](docs/examples/containers/sequence/vector.md)
  - unordered: [std::unordered_map](docs/examples/containers/unordered/unordered_map.md) · [std::unordered_multimap](docs/examples/containers/unordered/unordered_multimap.md) · [std::unordered_multiset](docs/examples/containers/unordered/unordered_multiset.md) · [std::unordered_set](docs/examples/containers/unordered/unordered_set.md)
  - views: [std::mdspan](docs/examples/containers/views/mdspan.md) · [std::span](docs/examples/containers/views/span.md)
- **[Strings](docs/examples/strings/README.md)** — `string`, `string_view`, char traits
  - [std::char_traits](docs/examples/strings/char_traits.md) · [std::string (std::basic_string)](docs/examples/strings/string.md) · [std::string_view](docs/examples/strings/string_view.md)
- **[Text processing](docs/examples/text-processing/README.md)** — `format`/`print`, `charconv`, regex, locales, `text_encoding`, C strings
  - [std::to_chars / std::from_chars](docs/examples/text-processing/charconv.md) · [std::format and std::print](docs/examples/text-processing/formatting.md) · [std::locale and facets](docs/examples/text-processing/localization.md) · [Null-terminated byte strings](docs/examples/text-processing/null-terminated-byte-strings.md) · [Null-terminated multibyte strings](docs/examples/text-processing/null-terminated-multibyte-strings.md) · [Null-terminated wide strings](docs/examples/text-processing/null-terminated-wide-strings.md) · [std::regex](docs/examples/text-processing/regex.md) · [std::text_encoding](docs/examples/text-processing/text-encoding.md)
- **[Algorithms](docs/examples/algorithms/README.md)** — `<algorithm>`, `<numeric>`, parallel & ranges algos
  - [Execution policies & ranges algorithms](docs/examples/algorithms/execution-and-ranges.md) · [Modifying algorithms](docs/examples/algorithms/modifying.md) · [Non-modifying algorithms](docs/examples/algorithms/non-modifying.md) · [Numeric algorithms (`<numeric>`)](docs/examples/algorithms/numeric.md) · [Partition, heap, and min/max](docs/examples/algorithms/partition-heap-minmax.md) · [Sorting & searching](docs/examples/algorithms/sorting-searching.md)
- **[Ranges](docs/examples/ranges/README.md)** — views, adaptors, factories, pipelines (C++20)
  - [Range factories & `ranges::to`](docs/examples/ranges/factories.md) · [Range concepts & access](docs/examples/ranges/range-concepts.md) · [Views & pipelines](docs/examples/ranges/views.md)
- **[Iterators](docs/examples/iterators/README.md)** — categories, adaptors, `iterator_traits`
  - [Iterator adaptors](docs/examples/iterators/adaptors.md) · [Iterator categories & concepts](docs/examples/iterators/categories.md) · [Iterator operations & traits](docs/examples/iterators/operations.md)
- **[Utilities](docs/examples/utilities/README.md)** — `optional`, `variant`, `tuple`, `expected`, function objects, callable wrappers (`function_ref` …)
  - [std::any](docs/examples/utilities/any.md) · [std::bitset](docs/examples/utilities/bitset.md) · [Callable wrappers](docs/examples/utilities/callable-wrappers.md) · [std::expected](docs/examples/utilities/expected.md) · [Function objects (`<functional>`)](docs/examples/utilities/functional.md) · [std::optional](docs/examples/utilities/optional.md) · [std::pair](docs/examples/utilities/pair.md) · [std::tuple](docs/examples/utilities/tuple.md) · [`<utility>` helpers](docs/examples/utilities/utility.md) · [std::variant](docs/examples/utilities/variant.md)
- **[Memory](docs/examples/memory/README.md)** — smart pointers, `indirect`/`polymorphic`, allocators, memory resources
  - [Allocators](docs/examples/memory/allocators.md) · [std::indirect and std::polymorphic](docs/examples/memory/indirect-polymorphic.md) · [Polymorphic memory resources (PMR)](docs/examples/memory/memory-resources.md) · [Raw memory utilities](docs/examples/memory/raw-memory.md) · [std::shared_ptr](docs/examples/memory/shared_ptr.md) · [std::unique_ptr](docs/examples/memory/unique_ptr.md) · [std::weak_ptr](docs/examples/memory/weak_ptr.md)
- **[Metaprogramming](docs/examples/metaprogramming/README.md)** — `type_traits`, constant evaluation, reflection (`<meta>`), `ratio`, `integer_sequence`
  - [Constant evaluation context](docs/examples/metaprogramming/constant-evaluation.md) · [std::integer_sequence / index_sequence](docs/examples/metaprogramming/integer-sequence.md) · [std::ratio](docs/examples/metaprogramming/ratio.md) · [Reflection queries (`<meta>`)](docs/examples/metaprogramming/reflection-queries.md) · [Static reflection (`<meta>`)](docs/examples/metaprogramming/reflection.md) · [Type traits](docs/examples/metaprogramming/type-traits.md)
- **[Numerics](docs/examples/numerics/README.md)** — `random`, `complex`, `<cmath>`, `<cfenv>`, `<bit>`, saturation arithmetic, SIMD, linalg
  - [Bit manipulation (`<bit>`)](docs/examples/numerics/bit.md) · [Floating-point environment](docs/examples/numerics/cfenv.md) · [std::complex](docs/examples/numerics/complex.md) · [Basic linear algebra (`<linalg>`)](docs/examples/numerics/linalg.md) · [Math functions & constants](docs/examples/numerics/math.md) · [std::random](docs/examples/numerics/random.md) · [Saturation arithmetic](docs/examples/numerics/saturation.md) · [Data-parallel types (SIMD)](docs/examples/numerics/simd.md) · [std::valarray](docs/examples/numerics/valarray.md)
- **[Date & time](docs/examples/datetime/README.md)** — `<chrono>`: durations, clocks, calendars, time zones; `<ctime>`
  - [C date and time (`<ctime>`)](docs/examples/datetime/c-time.md) · [Calendar & time zones (C++20)](docs/examples/datetime/calendar-timezones.md) · [Clocks & time points](docs/examples/datetime/clocks-timepoints.md) · [std::chrono durations](docs/examples/datetime/durations.md)
- **[Input/output](docs/examples/io/README.md)** — streams, files, string streams, filesystem, `syncstream`, `print`
  - [File streams](docs/examples/io/files.md) · [std::filesystem](docs/examples/io/filesystem.md) · [I/O manipulators](docs/examples/io/manipulators.md) · [Print functions (`std::print`, `std::println`)](docs/examples/io/print.md) · [Streams & stream state](docs/examples/io/streams.md) · [String streams](docs/examples/io/string-streams.md) · [std::osyncstream](docs/examples/io/syncstream.md)
- **[Concurrency](docs/examples/concurrency/README.md)** — threads, mutexes, atomics, futures, RCU & hazard pointers
  - [std::atomic](docs/examples/concurrency/atomics.md) · [Latches, barriers & semaphores (C++20)](docs/examples/concurrency/coordination.md) · [Futures, promises & async](docs/examples/concurrency/futures.md) · [Mutexes & condition variables](docs/examples/concurrency/mutexes.md) · [Safe reclamation: RCU and hazard pointers](docs/examples/concurrency/safe-reclamation.md) · [std::thread / std::jthread](docs/examples/concurrency/threads.md)
- **[Execution](docs/examples/execution/README.md)** — senders/receivers (C++26)
  - [Senders & receivers](docs/examples/execution/senders-receivers.md)
- **[Diagnostics](docs/examples/diagnostics/README.md)** — exceptions, `system_error`, `stacktrace`, `<debugging>`
  - [Debugging support (`<debugging>`)](docs/examples/diagnostics/debugging.md) · [The standard exception hierarchy](docs/examples/diagnostics/exception-hierarchy.md) · [std::stacktrace](docs/examples/diagnostics/stacktrace.md) · [std::error_code & std::system_error](docs/examples/diagnostics/system_error.md)
- **[Language support](docs/examples/language-support/README.md)** — `numeric_limits`, `<=>`, `source_location`, `initializer_list`, `typeid`, signals, `setjmp`
  - [Three-way comparison (`operator<=>`)](docs/examples/language-support/comparison.md) · [std::initializer_list](docs/examples/language-support/initializer-list.md) · [Non-local jumps](docs/examples/language-support/non-local-jumps.md) · [std::numeric_limits](docs/examples/language-support/numeric-limits.md) · [Signal handling](docs/examples/language-support/signals.md) · [std::source_location](docs/examples/language-support/source-location.md) · [typeid, std::type_info, std::type_index](docs/examples/language-support/type-info.md)
- **[Concepts](docs/examples/concepts/README.md)** — the standard concepts library (C++20)
  - [Standard concepts](docs/examples/concepts/standard-concepts.md)
- **[Feature-test macros](docs/examples/feature-test-macros/README.md)** — detecting what your compiler supports
- **[Named requirements](docs/examples/named-requirements/README.md)** — *CopyConstructible*, *Compare*, *Hash* … and how concepts relate
<!-- index:library:end -->

### Core language

<!-- index:language:start -->
- **[Basic concepts](docs/examples/language/basic-concepts/README.md)** — comments, keywords, types, names/lookup, storage/linkage, object lifetime, erroneous behaviour, `main`, freestanding
  - [Comments](docs/examples/language/basic-concepts/comments.md) · [Contracts (C++26)](docs/examples/language/basic-concepts/contracts.md) · [Erroneous behaviour](docs/examples/language/basic-concepts/erroneous-behaviour.md) · [Freestanding and hosted implementations](docs/examples/language/basic-concepts/freestanding.md) · [Fundamental types](docs/examples/language/basic-concepts/fundamental-types.md) · [Keywords and reserved identifiers](docs/examples/language/basic-concepts/keywords.md) · [The object model & `main`](docs/examples/language/basic-concepts/object-model.md) · [Scope, storage duration & linkage](docs/examples/language/basic-concepts/scope-storage-linkage.md)
- **[Expressions](docs/examples/language/expressions/README.md)** — value categories, operators, conversions, literals, constant exprs
  - [What constant evaluation allows](docs/examples/language/expressions/constant-evaluation-rules.md) · [Constant expressions (`constexpr`, `consteval`, `constinit`)](docs/examples/language/expressions/constant-expressions.md) · [Conversions & casts](docs/examples/language/expressions/conversions.md) · [Literals](docs/examples/language/expressions/literals.md) · [Operator overloading](docs/examples/language/expressions/operator-overloading.md) · [Operators, precedence & evaluation order](docs/examples/language/expressions/operators.md) · [Reflection (C++26)](docs/examples/language/expressions/reflection.md) · [Value categories](docs/examples/language/expressions/value-categories.md)
- **[Statements](docs/examples/language/statements/README.md)** — `if`/`switch`, loops, range-`for`, `break`/`continue`/`return`/`goto`
  - [Iteration statements](docs/examples/language/statements/iteration.md) · [Jump statements](docs/examples/language/statements/jump-statements.md) · [Selection statements](docs/examples/language/statements/selection.md)
- **[Declarations & initialization](docs/examples/language/declarations-initialization/README.md)** — `auto`, `const`/`constexpr`, the many initializations
  - [Attributes](docs/examples/language/declarations-initialization/attributes.md) · [`const`, `constexpr`, `mutable` & `volatile`](docs/examples/language/declarations-initialization/const-mutable.md) · [Declarations, `auto` & type deduction](docs/examples/language/declarations-initialization/declarations-and-auto.md) · [Enumerations](docs/examples/language/declarations-initialization/enumerations.md) · [Initialization](docs/examples/language/declarations-initialization/initialization.md) · [Namespaces & name lookup](docs/examples/language/declarations-initialization/namespaces.md) · [Pointers, references & arrays](docs/examples/language/declarations-initialization/pointers-references-arrays.md) · [Type aliases (`using` / `typedef`)](docs/examples/language/declarations-initialization/type-aliases.md)
- **[Functions](docs/examples/language/functions/README.md)** — overloading, default args, lambdas
  - [Default & variadic arguments](docs/examples/language/functions/default-variadic-args.md) · [Functions & overloading](docs/examples/language/functions/functions-and-overloading.md) · [Lambdas](docs/examples/language/functions/lambdas.md)
- **[Classes](docs/examples/language/classes/README.md)** — special members, RAII, inheritance, virtual, unions, deducing `this`, bit-fields, operator overloading
  - [Bit-fields](docs/examples/language/classes/bit-fields.md) · [Explicit object parameters ("deducing `this`")](docs/examples/language/classes/explicit-object-parameter.md) · [Inheritance & polymorphism](docs/examples/language/classes/inheritance-polymorphism.md) · [Members & access control](docs/examples/language/classes/members-access.md) · [Special member functions & RAII](docs/examples/language/classes/special-members.md) · [Unions & aggregates](docs/examples/language/classes/unions-aggregates.md)
- **[Templates](docs/examples/language/templates/README.md)** — function/class templates, variadics, CTAD, constraints & concepts
  - [Constraints and concepts (the language)](docs/examples/language/templates/constraints.md) · [Specialization & CTAD](docs/examples/language/templates/specialization-ctad.md) · [Template basics](docs/examples/language/templates/templates-basics.md) · [Variadic templates](docs/examples/language/templates/variadic-templates.md)
- **[Exceptions](docs/examples/language/exceptions/README.md)** — `throw`/`try`/`catch`, `noexcept`, exception safety
  - [noexcept & exception safety](docs/examples/language/exceptions/noexcept-and-safety.md) · [throw / try / catch](docs/examples/language/exceptions/throw-try-catch.md)
- **[Coroutines](docs/examples/language/coroutines/README.md)** — `co_await`/`co_yield`/`co_return` (C++20)
  - [Coroutines](docs/examples/language/coroutines/coroutines.md)
- **[Modules](docs/examples/language/modules/README.md)** — `import`/`export`, partitions, `import std;`, building with GCC/Clang/CMake (C++20)
  - [Building modules](docs/examples/language/modules/building.md) · [Modules](docs/examples/language/modules/modules.md)
- **[Preprocessor](docs/examples/language/preprocessor/README.md)** — macros, includes, conditional compilation, `#embed`
  - [`#embed` — resource inclusion (C++26)](docs/examples/language/preprocessor/embed.md) · [The preprocessor](docs/examples/language/preprocessor/preprocessor.md)
<!-- index:language:end -->

### Everything else

- **[`docs/guidelines/`](docs/guidelines/README.md)** — how to use it well (see [Guidelines](#guidelines))
- **[`tests/`](tests/CMakeLists.txt)** — GoogleTest suite that checks the pages' claims (see [Testing and CI](docs/testing.md#running-the-tests))
- **[`tools/check_snippets.py`](tools/check_snippets.py)** — compile-checks every ```` ```cpp ```` block ([`snippet_prelude.hpp`](tools/snippet_prelude.hpp), [`snippet_baseline/`](tools/snippet_baseline/gcc.txt)); [`tools/run_tests.sh`](tools/run_tests.sh) (or [`run_tests.ps1`](tools/run_tests.ps1) on Windows) runs the tests, [`tools/ctest_to_json.py`](tools/ctest_to_json.py) writes their results as JSON
- **[`docs/testing.md`](docs/testing.md)** — how the snippets and tests are checked, the toolchain presets and the manual CI workflows
- **[`docs/platform-support.md`](docs/platform-support.md)** — platforms, compilers and architectures the tests and snippets have run on (local runs and consented GitHub Actions runs), with the known gaps per toolchain; generated by [`tools/collect_results.py`](tools/collect_results.py)
- **[`evals/`](evals/README.md)** — test cases for the `cpp-guidelines` agent skill
- **[`.claude/skills/`](.claude/skills/)** — [`cpp-guidelines`](.claude/skills/cpp-guidelines/SKILL.md) (review against the Core Guidelines), [`refresh-guidelines`](.claude/skills/refresh-guidelines/SKILL.md) (keep them current), [`add-to-topic`](.claude/skills/add-to-topic/SKILL.md) (add a feature or gotcha to the reference), [`modern-cpp`](.claude/skills/modern-cpp/SKILL.md) (**start here**: write C++ using this reference), [`platform-support`](.claude/skills/platform-support/SKILL.md) (run, read and report the platform runs)
- **[`modules/`](modules/)** — submodules: [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines), [perf-report](https://github.com/homer6/perf-report)

Every area on cppreference's main page has pages, and every snippet compiles
(or is tagged with why it can't). A GoogleTest suite that checks each page's
claims at run time is in progress:
[#41](https://github.com/homer6/cpp-docs/issues/41).

## Guidelines

The reference tells you what each feature *is*;
[`docs/guidelines/`](docs/guidelines/README.md) tells you how to *use* it well:

- [Core Guidelines digest](docs/guidelines/core-guidelines.md) — the rules that matter most
- [Memory & safety](docs/guidelines/safety.md) — profiles, ownership, dangling, C++26 hardening
- [Modern idioms](docs/guidelines/modern-idioms.md) — C++17 → C++26 and when to reach for each
- [Design principles](docs/guidelines/design-principles.md) — Ben Deane's composable, hard-to-misuse style
- [Project conventions](docs/guidelines/project-conventions.md) — naming, headers, API shape, tests
- [Industry practice](docs/guidelines/industry-practice.md) — Google/Chromium/LLVM policies, Abseil tips, the GSL
- [Tooling](docs/guidelines/tooling.md) — sanitizers, suppressions, hardened libraries, static analysis
- [Debugging](docs/guidelines/debugging.md) — gdb and lldb side by side, core dumps, `std::stacktrace`
- [Valgrind](docs/guidelines/valgrind.md) — Memcheck, Helgrind and DRD, suppressions, vgdb
- [Performance tooling](docs/guidelines/performance-tooling.md) — benchmarking, `perf`, flame graphs, heap profiling
- [Testing](docs/guidelines/testing.md) — frameworks, property-based tests, fuzzing, contracts, coverage
- [Build and packaging](docs/guidelines/build-and-packaging.md) — CMake, presets, modules, dependencies, flags

Sources and edition dates are in [sources.md](docs/guidelines/sources.md). To
refresh them or import a new reference, ask Claude to use the
`refresh-guidelines` skill.

## Use it while you write C++ (Claude Code)

The [`modern-cpp`](.claude/skills/modern-cpp/SKILL.md) skill makes Claude Code
check this reference before writing C++. It:
- looks up the page for a facility and follows its gotchas and version notes;
- checks that the feature is supported on your compilers;
- applies the guidelines and validates the result.

**Start with `/modern-cpp`.** It's the preferred entry point for writing,
modernizing or reviewing C++, and it calls `/cpp-guidelines` itself when the
guidelines matter. Use `/cpp-guidelines` directly only for:

- **Reviewing or auditing existing code against the Core Guidelines**, where
  you want findings with rule IDs (**R.11**, **C.13**) and no new code.
- **A question about a specific rule** ("what does F.16 say, and does this
  code follow it?") or about one guidelines page.
- **A focused ownership, lifetime or memory-safety fix** in code that already
  uses the right facilities.
- **Measuring the guidelines skill** with the case suite in `evals/`.

**Install it once for all your projects:** clone this repo and run
`make install`. That links `modern-cpp` and
[`cpp-guidelines`](.claude/skills/cpp-guidelines/SKILL.md) (review C++ against
the Core Guidelines, citing rule IDs) into `~/.claude/skills`.

```bash
git clone https://github.com/homer6/cpp-docs.git
cd cpp-docs && make install        # make uninstall removes the links
```

Then ask for it by name (`/modern-cpp`), or just ask for modern C++.

**Upgrading:** run `make update`. It pulls the latest release (stay on `main`)
and re-links. The links point back at your checkout, so a plain `git pull`
upgrades the skills too. `make install` also upgrades an older install: it
re-points a link to another checkout, and moves an older copied folder to
`~/.claude/skills-backup/` before linking. `SKILLS_DIR=… make install`
installs somewhere else.

## How to use these notes

Every reference page has the same shape:

1. **What it is** — one-line summary, header, and the C++ version that introduced it.
2. **Mental model** — how to picture it.
3. **Complexity / key facts** — the numbers and rules that matter.
4. **When to use it** (and when not to).
5. **Examples** — short, focused, modern C++ snippets.
6. **Gotchas** — the non-obvious traps.
7. **See also** — where to read more.

## How it's tested

- **[Testing and CI](docs/testing.md)**: checking the snippets, running the
  tests, the toolchain presets (local, portable, MSVC and clang-cl from a VS
  Developer shell, GCC 16 in Docker) and the manual GitHub Actions workflows.
- **[Platform support](docs/platform-support.md)**: the full results behind
  the table below (counts, skip reasons, known gaps per toolchain), generated
  by [`tools/collect_results.py`](tools/collect_results.py).
- **[Running a workflow](.github/workflows/README.md)**: how a consented run
  is started, watched and read.

## Conventions

Examples target the **latest C++ standard** and prefer modern idioms (CTAD,
structured bindings, ranges, `auto`, designated initializers, etc.). The C++
version is noted whenever a feature isn't available everywhere. The full house
style is in [`CLAUDE.md`](CLAUDE.md).

## Progress

**Latest release: [v0.2.1](docs/releases/v0.2.1.md)** (2026-11-01). All
releases are listed in [`docs/releases/`](docs/releases/README.md).

See [`docs/examples/README.md`](docs/examples/README.md) for the full index and
status across every topic.

## Tested platforms

<!-- platforms:start -->
<!-- Generated by tools/collect_results.py from consented runs and local results; don't edit by hand (see CLAUDE.md). -->

50 platform × compiler × architecture combinations, results as of 2026-11-02. **Tests**: GoogleTest cases passed / total; **snippets**: compile-checked blocks passed / checked. *report-only*: that toolchain's snippet results don't fail its run yet (no reviewed baseline). Skip reasons, known gaps and full counts: [`docs/platform-support.md`](docs/platform-support.md), with the [feature support matrix](docs/platform-support.md#feature-support-matrix) and one page per toolchain (every feature and example) in [`docs/platform-support/`](docs/platform-support/). The GitHub Actions runs here cover areas `^strings$` (the `areas` input); the local runs cover the whole suite.

| Platform | Compiler | Arch | Tests | Snippets | Run |
|---|---|---|---|---|---|
| Linux (Docker) | GCC 16.2.0 | arm64 | ✅ 1062/1100 (38 skipped) | ✅ 717/717 (103 skipped) | local, 2026-11-02 |
| macOS (Darwin 23.6.0) | Clang 22.1.8, libc++ | arm64 | ✅ 969/1097 (128 skipped) | ✅ 672/691 (19 baselined, 129 skipped) | local, 2026-11-02 |
| macOS (Darwin 23.6.0) | GCC 15.2.0 | arm64 | ✅ 986/1097 (111 skipped) | ✅ 691/691 (129 skipped) | local, 2026-11-02 |
| Debian 13 | GCC 15.3.0 | arm64 | ✅ 30/30 | ✅ 12/12 | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Debian 13 | GCC 16.2.0 | arm64 | ✅ 30/30 | ✅ 12/12 | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 22.1.8, libc++ | arm64 | ✅ 30/30 | ✅ 12/12 | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 22.1.8, libstdc++ | arm64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 26.04 | Clang 22.1.2, libc++ | arm64 | ✅ 30/30 | ✅ 12/12 | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 26.04 | Clang 22.1.2, libstdc++ | arm64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 26.04 | GCC 15.2.0 | arm64 | ✅ 30/30 | ✅ 12/12 | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Debian 12 | GCC 13.5.0 | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Debian 13 | GCC 14.4.0 | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Debian 13 | GCC 15.3.0 | x86_64 | ✅ 30/30 | ✅ 12/12 | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Debian 13 | GCC 16.2.0 | x86_64 | ✅ 30/30 | ✅ 12/12 | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 18.1.8, libc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 18.1.8, libstdc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 19.1.7, libc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 19.1.7, libstdc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 20.1.8, libc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 20.1.8, libstdc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 21.1.8, libc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 21.1.8, libstdc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 22.1.8, libc++ | x86_64 | ✅ 30/30 | ✅ 12/12 | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 24.04 | Clang 22.1.8, libstdc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 26.04 | Clang 22.1.2, libc++ | x86_64 | ✅ 30/30 | ✅ 12/12 | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 26.04 | Clang 22.1.2, libstdc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| Ubuntu 26.04 | GCC 15.2.0 | x86_64 | ✅ 30/30 | ✅ 12/12 | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41358257575) |
| macOS 15 | Apple Clang 17.0.0 | arm64 | ✅ 30/30 | ❌ 0/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 15 | GCC 16.2.0 | arm64 | ✅ 30/30 | ✅ 12/12 | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 15 | Clang 23.1.4, libc++ | arm64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 15 | Apple Clang 17.0.0 | x86_64 | ✅ 30/30 | ❌ 0/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 15 | GCC 16.2.0 | x86_64 | ✅ 30/30 | ✅ 12/12 | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 15 | Clang 22.1.8, libc++ | x86_64 | ✅ 30/30 | ✅ 12/12 | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 26 | Apple Clang 21.0.0 | arm64 | ✅ 30/30 | ❌ 0/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 26 | GCC 16.2.0 | arm64 | ✅ 30/30 | ✅ 12/12 | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 26 | Clang 23.1.4, libc++ | arm64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 26 | Apple Clang 21.0.0 | x86_64 | ✅ 30/30 | ❌ 0/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 26 | GCC 16.2.0 | x86_64 | ✅ 30/30 | ✅ 12/12 | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| macOS 26 | Clang 22.1.8, libc++ | x86_64 | ✅ 30/30 | ✅ 12/12 | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41365429121) |
| Windows 11 | MSVC 19.51.36260 | arm64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41367352135) |
| Windows Server 2022 | MSVC 19.44.35229 | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41367352135) |
| Windows Server 2025 | MSVC 19.51.36260 | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41367352135) |
| Windows Server 2025, MSYS2 CLANG64 | Clang 23.1.4, libstdc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41366615444) |
| Windows Server 2025, MSYS2 UCRT64 | Clang 23.1.4, libstdc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41366615444) |
| Windows Server 2025, MSYS2 UCRT64 | GCC 16.2.0 | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41366615444) |
| Windows Server 2025 | clang-cl 22.1.3 | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-02](https://github.com/homer6/cpp-docs/actions/runs/41366615444) |
| FreeBSD 14.5 | Clang 21.1.8, libc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41357988143) |
| FreeBSD 14.5 | GCC 14.2.0 | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41357988143) |
| FreeBSD 15.1 | Clang 19.1.7, libc++ | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41357988143) |
| FreeBSD 15.1 | GCC 14.2.0 | x86_64 | ✅ 30/30 | ✅ 12/12 (report-only) | [2026-11-01](https://github.com/homer6/cpp-docs/actions/runs/41357988143) |
<!-- platforms:end -->
