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
    text-processing/         #   format/print, charconv, regex, localization
    algorithms/              #   <algorithm>, <numeric>, parallel and ranges algorithms
    ranges/                  #   views, adaptors, factories, range concepts
    iterators/               #   categories, adaptors, operations
    utilities/               #   optional, variant, tuple, expected, any, bitset, functional
    memory/                  #   smart pointers, allocators, memory resources
    metaprogramming/         #   type traits, ratio, integer_sequence, reflection
    numerics/                #   math, random, complex, bit, SIMD, linalg, valarray
    datetime/                #   chrono durations, clocks, calendars, time zones
    io/                      #   streams, files, string streams, filesystem
    concurrency/             #   threads, mutexes, atomics, futures, coordination
    execution/               #   senders/receivers (C++26)
    diagnostics/             #   exceptions, system_error, stacktrace
    language-support/        #   numeric_limits, <=>, source_location, initializer_list
    concepts/                #   the standard concepts library
    feature-test-macros/     #   detecting what your compiler supports
    language/                #   the core language
      basic-concepts/        #     types, object model, storage and linkage, contracts
      expressions/           #     value categories, operators, conversions, constant expressions
      statements/            #     selection, iteration
      declarations-initialization/
      functions/             #     overloading, default and variadic args, lambdas
      classes/               #     special members, inheritance, unions and aggregates
      templates/             #     basics, variadics, specialization and CTAD
      exceptions/            #     throw/try/catch, noexcept and exception safety
      coroutines/  modules/  preprocessor/
  guidelines/                # how to use it well (see below)
tools/                       # check_snippets.py — compile-checks every ```cpp block
evals/                       # test cases for the cpp-guidelines agent skill
.claude/skills/              # cpp-guidelines (apply the guidelines), refresh-guidelines (keep them current)
modules/                     # submodules: C++ Core Guidelines, perf-report
```

Work still to do is tracked in [#20](https://github.com/homer6/cpp-docs/issues/20)
(missing topics and snippets that don't compile yet).

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

`docs/guidelines/` must pass cleanly. `docs/examples/` is checked against a
list of known failures that should only shrink. Fence tags (`body`,
`nocompile`, `compile-fail`, `needs=…`) are explained in [`CLAUDE.md`](CLAUDE.md).

## Conventions

Examples target the **latest C++ standard** and prefer modern idioms (CTAD,
structured bindings, ranges, `auto`, designated initializers, etc.). The C++
version is noted whenever a feature isn't available everywhere. The full house
style is in [`CLAUDE.md`](CLAUDE.md).

## Progress

See [`docs/examples/README.md`](docs/examples/README.md) for the full index and
status across every topic.
