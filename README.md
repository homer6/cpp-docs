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

```
docs/
  examples/                  # the reference — what each feature is
    README.md                #   master index and progress
    containers/              #   sequence, associative, unordered, adaptors, views
    strings/                 #   string, string_view, char_traits
    text-processing/         #   format, charconv, regex, localization, C strings, text_encoding
    algorithms/              #   <algorithm>, <numeric>, parallel and ranges algorithms
    ranges/                  #   views, adaptors, factories, range concepts
    iterators/               #   categories, adaptors, operations
    utilities/               #   optional, variant, tuple, expected, any, bitset, functional
    memory/                  #   smart pointers, allocators, memory resources
    metaprogramming/         #   type traits, ratio, integer_sequence, reflection
    numerics/                #   math, random, complex, bit, cfenv, SIMD, linalg, valarray
    datetime/                #   chrono durations, clocks, calendars, time zones
    io/                      #   streams, files, filesystem, print, syncstream
    concurrency/             #   threads, mutexes, atomics, futures, coordination, safe reclamation
    execution/               #   senders/receivers (C++26)
    diagnostics/             #   exceptions, system_error, stacktrace, debugging
    language-support/        #   numeric_limits, <=>, source_location, typeid, signals, setjmp
    concepts/                #   the standard concepts library
    feature-test-macros/     #   detecting what your compiler supports
    named-requirements/      #   Cpp17* requirements and how concepts relate
    language/                #   the core language
      basic-concepts/        #     types, object model, storage, contracts, comments, keywords, freestanding
      expressions/           #     value categories, operators, conversions, constant expressions
      statements/            #     selection, iteration, jump statements
      declarations-initialization/
      functions/             #     overloading, default and variadic args, lambdas
      classes/               #     special members, inheritance, operator overloading, deducing this, bit-fields
      templates/             #     basics, variadics, specialization and CTAD, constraints
      exceptions/            #     throw/try/catch, noexcept and exception safety
      coroutines/  modules/  preprocessor/   # preprocessor includes #embed
  guidelines/                # how to use it well (see below)
tests/                       # GoogleTest suite checking each page's claims (see below)
tools/                       # check_snippets.py — compile-checks every ```cpp block; run_tests.sh
evals/                       # test cases for the cpp-guidelines agent skill
.claude/skills/              # cpp-guidelines (apply the guidelines), refresh-guidelines (keep them current)
modules/                     # submodules: C++ Core Guidelines, perf-report
CMakeLists.txt  CMakePresets.json   # build for tests/ only
```

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
- [Tooling](docs/guidelines/tooling.md) — sanitizers, debuggers, core dumps, static analysis
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
```

Both must pass cleanly; the list of known failures
(`tools/snippet_baseline/gcc.txt`) is empty and stays that way. Fence tags (`body`,
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
  in `/usr/lib`.
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

## Conventions

Examples target the **latest C++ standard** and prefer modern idioms (CTAD,
structured bindings, ranges, `auto`, designated initializers, etc.). The C++
version is noted whenever a feature isn't available everywhere. The full house
style is in [`CLAUDE.md`](CLAUDE.md).

## Progress

See [`docs/examples/README.md`](docs/examples/README.md) for the full index and
status across every topic.
