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
[![QEMU (riscv64, s390x, ppc64le)](https://github.com/homer6/cpp-docs/actions/workflows/qemu.yml/badge.svg?branch=main&event=workflow_dispatch)](https://github.com/homer6/cpp-docs/actions/workflows/qemu.yml)

**Verified locally first:** every release passes the full test suite and the
snippet checker on GCC 15 and Clang 22 (macOS, arm64) and GCC 16 (Linux,
Docker); that's the primary proof. The badges above are the GitHub Actions
platform workflows, which run **only by hand**, with the owner's consent, at
a release: each shows its last consented run (or "no status" before the
first), not the current tip. The full platform × compiler × architecture
matrix, with counts, skip reasons, run links and dates, is in
[`docs/platform-support.md`](docs/platform-support.md).

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
- **[`tests/`](tests/CMakeLists.txt)** — GoogleTest suite that checks the pages' claims (see [Running the tests](#running-the-tests))
- **[`tools/check_snippets.py`](tools/check_snippets.py)** — compile-checks every ```` ```cpp ```` block ([`snippet_prelude.hpp`](tools/snippet_prelude.hpp), [`snippet_baseline/`](tools/snippet_baseline/gcc.txt)); [`tools/run_tests.sh`](tools/run_tests.sh) (or [`run_tests.ps1`](tools/run_tests.ps1) on Windows) runs the tests, [`tools/ctest_to_json.py`](tools/ctest_to_json.py) writes their results as JSON
- **[`docs/platform-support.md`](docs/platform-support.md)** — platforms, compilers and architectures the tests and snippets have run on (local runs and consented GitHub Actions runs), with the known gaps per toolchain; generated by [`tools/collect_results.py`](tools/collect_results.py)
- **[`evals/`](evals/README.md)** — test cases for the `cpp-guidelines` agent skill
- **[`.claude/skills/`](.claude/skills/)** — [`cpp-guidelines`](.claude/skills/cpp-guidelines/SKILL.md) (review against the Core Guidelines), [`refresh-guidelines`](.claude/skills/refresh-guidelines/SKILL.md) (keep them current), [`add-to-topic`](.claude/skills/add-to-topic/SKILL.md) (add a feature or gotcha to the reference), [`modern-cpp`](.claude/skills/modern-cpp/SKILL.md) (**start here**: write C++ using this reference)
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

## Checking the snippets

Run the checker locally:

```bash
python3 tools/check_snippets.py docs/guidelines --cxx g++-15
python3 tools/check_snippets.py docs/examples --cxx g++-15 -q \
  --baseline tools/snippet_baseline/gcc.txt
python3 tools/check_snippets.py docs --cxx /opt/homebrew/opt/llvm/bin/clang++ \
  --flag=-stdlib=libc++ --flag=-fexperimental-library -q \
  --baseline tools/snippet_baseline/clang.txt
```

All three must pass cleanly. The GCC list of known failures
(`tools/snippet_baseline/gcc.txt`) is empty and stays that way; the Clang one
(`tools/snippet_baseline/clang.txt`) lists only libc++ gaps and only shrinks. Fence tags (`body`,
`nocompile`, `compile-fail`, `needs=…`) are explained in [`CLAUDE.md`](CLAUDE.md).

Other toolchains: `--toolchain msvc` drives `cl /Zs` (syntax only, the prelude
via `/FI`) and `--toolchain clang-cl` drives `clang-cl /Zs`, both from a VS
Developer shell; each has its own baseline (`tools/snippet_baseline/msvc.txt`,
`clang-cl.txt`). Module blocks are checked with GCC and Clang only for now; with
the cl-style toolchains they skip with that reason. `--json FILE` also writes
the results (platform, compiler, counts, failures, skips with reasons) as JSON.

## Running the tests

The snippet checker proves the code **compiles**; the GoogleTest suite under
[`tests/`](tests/) checks that it **does what the pages say** (printed output,
values in comments, thrown exceptions, type claims). Run it locally with CMake
3.25+ (presets) and Ninja:

```bash
tools/run_tests.sh              # gcc-15: Homebrew g++-15, libstdc++
tools/run_tests.sh clang-22     # Homebrew Clang 22 with libc++
tools/run_tests.sh gcc-16       # GCC 16 in the gcc:16 Docker image
tools/run_tests.sh gcc-15 -L containers   # one area (ctest arguments)
```

`tools/run_tests.sh <preset>` runs `cmake --workflow --preset <preset>`
(configure → build → test into `build/<preset>/`) and then lists each skipped
test with its reason, which ctest itself doesn't show. You can also run the
workflow directly: `cmake --workflow --preset gcc-15`.

- **gcc-15** — `CMAKE_CXX_COMPILER=g++-15`.
- **clang-22** — `/opt/homebrew/opt/llvm/bin/clang++ -stdlib=libc++`, linked
  with `-L/opt/homebrew/opt/llvm/lib/c++ -L/opt/homebrew/opt/llvm/lib/unwind
  -lunwind` plus matching `-rpath`s, so the binaries use Homebrew's libc++
  (whose headers they were compiled against) rather than the older system one
  in `/usr/lib`. It also sets `CMAKE_CXX_STDLIB_MODULES_JSON` to Homebrew's
  `libc++.modules.json`, which `clang++ -print-file-name` doesn't find, so
  CMake can build the `std` module.
- **gcc-16** — runs inside Docker. The `gcc:16` image (arm64-native, Debian
  trixie) has `g++` 16.2 and git but no CMake or Ninja, so the script installs
  them from Debian (CMake 3.31) before running the workflow:

  ```bash
  docker run --rm -v "$PWD":/w -w /w gcc:16 bash -c \
    'apt-get update -qq && apt-get install -y -qq --no-install-recommends cmake ninja-build && cmake --workflow --preset gcc-16'
  ```

  The script re-runs itself in Docker only for `gcc-16`, and not when it's
  already in a container (`/.dockerenv`, `/run/.containerenv`) or
  `CPPDOCS_NO_DOCKER=1` is set (a host whose `g++` is GCC 16).

### Portable presets (any machine, CI)

The presets above are the owner's Mac. These take no paths: the compiler comes
from `CXX` (or `-DCMAKE_CXX_COMPILER=…`), and without either they use the
driver name in the table from `PATH`. They build into `build/<preset>/` with
Ninja, and the configure step fails if the compiler isn't the preset's
toolchain (say AppleClang under `gcc`), so a job can't test the wrong one.

| Preset | Toolchain | Default `CXX` |
|---|---|---|
| `gcc` | GCC + libstdc++ (Linux, macOS, FreeBSD) | `g++` |
| `clang` | Clang + the platform's library (libstdc++ on Linux, libc++ on macOS/FreeBSD) | `clang++` |
| `clang-libcxx` | Clang + libc++ (`-stdlib=libc++` via `CMAKE_CXX_FLAGS_INIT`, so `CXXFLAGS`/`LDFLAGS` still apply) | `clang++` |
| `msvc` | MSVC `cl` + MS STL (Windows) | `cl` |
| `clang-cl` | `clang-cl` + MS STL (Windows) | `clang-cl` |
| `mingw` | MinGW-w64 GCC + libstdc++ (MSYS2) | `g++` |

```bash
CXX=g++-14 tools/run_tests.sh gcc
CXX=clang++-20 tools/run_tests.sh clang-libcxx
CXX=/opt/homebrew/opt/llvm/bin/clang++ \
  LDFLAGS="-L/opt/homebrew/opt/llvm/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++" \
  tools/run_tests.sh clang-libcxx      # Homebrew LLVM on macOS (import std skips:
                                       # configure first with -DCMAKE_CXX_STDLIB_MODULES_JSON=
                                       # /opt/homebrew/opt/llvm/lib/c++/libc++.modules.json)
tools/run_tests.sh --json build/results-gcc.json gcc   # also write JSON results
```

**MSVC and clang-cl** use Ninja too, so they must run where `cl`, `link` and
the Windows SDK are on `PATH`: a *Developer PowerShell / Command Prompt for
VS* (or `vcvarsall.bat x64`; in GitHub Actions, a step that sets up the MSVC
developer environment). From there, `tools/run_tests.ps1 msvc` (PowerShell) or
`tools/run_tests.sh msvc` (Git Bash started from that shell) runs the
workflow. The two presets show up in `cmake --list-presets` only on Windows.
The tests are built with `/std:c++latest /permissive- /EHsc /utf-8
/Zc:__cplusplus /bigobj /W4` (plus `/Zc:preprocessor` for `cl`), and
GoogleTest with `gtest_force_shared_crt` so both use the DLL runtime. `mingw`
runs from an MSYS2 MinGW shell (`tools/run_tests.sh mingw`).

`tools/ctest_to_json.py --build-dir build/<preset>` turns any ctest run
(`LastTest.log`, or a `ctest --output-junit` file with `--junit`) into JSON:
platform, compiler and version, preset, counts, failures and skips with their
reasons. `tools/check_snippets.py --json` writes the same shape for snippets.

`tools/collect_results.py` turns those JSON files into
[`docs/platform-support.md`](docs/platform-support.md): give it a GitHub
Actions run ID (it fetches the run's `results-*.json` artifacts with
`gh run download`) or a folder of results, plus `--local <dir>` for local
runs; `--stdout` prints the page instead. A file that doesn't match the
schema fails with the file and field named. The page changes only from runs
the owner consented to.

GoogleTest v1.18.0 is fetched at configure time with `FetchContent`. Tests
mirror the pages: `docs/examples/<area>/.../<page>.md` is checked by
`tests/<area>/.../<page>_test.cpp`, and each area folder builds into one
executable (`examples_containers_tests`, `examples_text_processing_tests`, …;
`examples_tests` builds them all). Helpers in [`tests/support/`](tests/support/):
`capture.hpp` captures what `std::cout`, `printf` and `std::print` write, and
`feature.hpp` provides `CPPDOCS_REQUIRE(__cpp_lib_x)`, which skips a test with
the missing feature's name when a toolchain doesn't define the macro.

Named modules: [`tests/language/modules/modules.cmake`](tests/language/modules/modules.cmake)
builds the module units in `tests/language/modules/units/` with
`FILE_SET CXX_MODULES` (CMake 3.28+ and Ninja) and scans the tests that import
them. `import std` is used where CMake provides it: CMake 4.3 and 4.4 with the
experimental gate the top-level `CMakeLists.txt` sets, or 4.5+ without one.
Elsewhere — the gcc-16 image's CMake 3.31, GCC 15 with CMake 4.5 on macOS —
those tests skip and say why.

## CI (manual)

GitHub Actions workflows here are **manual only**: `workflow_dispatch` is the
only trigger, nothing runs on push, pull request or a schedule, and a run
needs the owner's go-ahead. Every workflow is read-only (`permissions:
contents: read`) with actions pinned to commit SHAs;
`python3 tools/check_workflows.py` checks that. How to run, watch and read a
workflow is in [`.github/workflows/README.md`](.github/workflows/README.md).
The local checks above remain the source of truth.

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
