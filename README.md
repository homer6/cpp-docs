# cpp-docs

A hands-on reference for learning **modern C++** — the language and the standard
library, from C++11 through the latest standard (C++26) — organized as
bite-sized, example-driven Markdown notes, plus opinionated guidelines on how to
write C++ well.

The reference mirrors how [cppreference.com](https://cppreference.com/)
organizes C++. Facts can come from any good source — cppreference, the standard
draft, WG21 papers, compiler and library docs — as long as they've been
verified locally (the snippet compiles, and behaves as described when run).

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
- **[Memory](docs/examples/memory/README.md)** — smart pointers, allocators, memory resources
  - [Allocators](docs/examples/memory/allocators.md) · [std::indirect and std::polymorphic](docs/examples/memory/indirect-polymorphic.md) · [Polymorphic memory resources (PMR)](docs/examples/memory/memory-resources.md) · [Raw memory utilities](docs/examples/memory/raw-memory.md) · [std::shared_ptr](docs/examples/memory/shared_ptr.md) · [std::unique_ptr](docs/examples/memory/unique_ptr.md) · [std::weak_ptr](docs/examples/memory/weak_ptr.md)
- **[Metaprogramming](docs/examples/metaprogramming/README.md)** — `type_traits`, constant evaluation, `ratio`, `integer_sequence`
  - [Constant evaluation context](docs/examples/metaprogramming/constant-evaluation.md) · [std::integer_sequence / index_sequence](docs/examples/metaprogramming/integer-sequence.md) · [std::ratio](docs/examples/metaprogramming/ratio.md) · [Static reflection (`<meta>`)](docs/examples/metaprogramming/reflection.md) · [Type traits](docs/examples/metaprogramming/type-traits.md)
- **[Numerics](docs/examples/numerics/README.md)** — `random`, `complex`, `<cmath>`, `<cfenv>`, `<bit>`, SIMD, linalg
  - [Bit manipulation (`<bit>`)](docs/examples/numerics/bit.md) · [Floating-point environment](docs/examples/numerics/cfenv.md) · [std::complex](docs/examples/numerics/complex.md) · [Basic linear algebra (`<linalg>`)](docs/examples/numerics/linalg.md) · [Math functions & constants](docs/examples/numerics/math.md) · [std::random](docs/examples/numerics/random.md) · [Data-parallel types (SIMD)](docs/examples/numerics/simd.md) · [std::valarray](docs/examples/numerics/valarray.md)
- **[Date & time](docs/examples/datetime/README.md)** — `<chrono>`: durations, clocks, calendars, time zones
  - [Calendar & time zones (C++20)](docs/examples/datetime/calendar-timezones.md) · [Clocks & time points](docs/examples/datetime/clocks-timepoints.md) · [std::chrono durations](docs/examples/datetime/durations.md)
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
- **[`tools/check_snippets.py`](tools/check_snippets.py)** — compile-checks every ```` ```cpp ```` block ([`snippet_prelude.hpp`](tools/snippet_prelude.hpp), [`snippet_baseline/`](tools/snippet_baseline/gcc.txt)); [`tools/run_tests.sh`](tools/run_tests.sh) runs the tests
- **[`evals/`](evals/README.md)** — test cases for the `cpp-guidelines` agent skill
- **[`.claude/skills/`](.claude/skills/)** — [`cpp-guidelines`](.claude/skills/cpp-guidelines/SKILL.md) (apply the guidelines), [`refresh-guidelines`](.claude/skills/refresh-guidelines/SKILL.md) (keep them current)
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
- [Industry practice](docs/guidelines/industry-practice.md) — Google/Chromium/LLVM policies, Abseil tips
- [Tooling](docs/guidelines/tooling.md) — sanitizers, suppressions, hardened libraries, static analysis
- [Debugging](docs/guidelines/debugging.md) — gdb and lldb side by side, core dumps, `std::stacktrace`
- [Valgrind](docs/guidelines/valgrind.md) — Memcheck, Helgrind and DRD, suppressions, vgdb
- [Performance tooling](docs/guidelines/performance-tooling.md) — benchmarking, `perf`, flame graphs, heap profiling
- [Testing](docs/guidelines/testing.md) — frameworks, property-based tests, fuzzing, contracts, coverage
- [Build and packaging](docs/guidelines/build-and-packaging.md) — CMake, presets, modules, dependencies, flags

Sources and edition dates are in [sources.md](docs/guidelines/sources.md). To
refresh them or import a new reference, ask Claude to use the
`refresh-guidelines` skill.

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

There's no CI; run the checker locally:

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

## Running the tests

The snippet checker proves the code **compiles**; the GoogleTest suite under
[`tests/`](tests/) checks that it **does what the pages say** (printed output,
values in comments, thrown exceptions, type claims). There's no CI; run it
locally with CMake 3.25+ (presets) and Ninja:

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

## Conventions

Examples target the **latest C++ standard** and prefer modern idioms (CTAD,
structured bindings, ranges, `auto`, designated initializers, etc.). The C++
version is noted whenever a feature isn't available everywhere. The full house
style is in [`CLAUDE.md`](CLAUDE.md).

## Progress

**Latest release: [v0.1.0](docs/releases/v0.1.0.md)** (2026-10-31). All
releases are listed in [`docs/releases/`](docs/releases/README.md).

See [`docs/examples/README.md`](docs/examples/README.md) for the full index and
status across every topic.
