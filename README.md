# cpp-docs

A hands-on reference for learning **modern C++** — the language and the standard
library, from C++11 through the latest standard (C++26) — organized as
bite-sized, example-driven Markdown notes.

The canonical source of truth for everything here is
[cppreference.com](https://cppreference.com/), and these notes mirror how
cppreference organizes the reference. Each page links back to the relevant
cppreference articles so you can dig deeper.

## Scope

We're learning **all of C++**, not just one corner of it. The plan follows
cppreference's two big buckets — the **language** and the **standard library** —
and we fill them in topic by topic. **Containers are the starting point**, not
the whole story.

## Layout

```
docs/examples/
  containers/        # ← we start here
    sequence/        #   array, vector, inplace_vector, deque, list, forward_list
    associative/     #   set, multiset, map, multimap, flat_* (C++23)
    unordered/       #   unordered_set/multiset/map/multimap
    adaptors/        #   stack, queue, priority_queue
    views/           #   span (C++20), mdspan (C++23)
  strings/           # (planned) string, string_view, char traits, formatting
  algorithms/        # (planned) <algorithm>, <numeric>, ranges algorithms
  ranges/            # (planned) views, adaptors, range-based pipelines
  utilities/         # (planned) optional, variant, tuple, pair, expected, ...
  language/          # (planned) expressions, templates, classes, concepts, ...
  ...
```

Each cppreference area becomes a subfolder under `docs/examples/`. New areas get
added as we work through them.

## How to use these notes

Every page has a consistent shape:

1. **What it is** — one-line summary, header, and the C++ version that introduced it.
2. **Mental model** — how to picture it.
3. **Complexity / key facts** — the numbers and rules that matter.
4. **When to use it** (and when not to).
5. **Examples** — short, focused, modern C++ snippets.
6. **Gotchas** — the non-obvious traps.
7. **See also** — links to cppreference.

## Conventions

Examples target the **latest C++ standard** and prefer modern idioms (CTAD,
structured bindings, ranges, `auto`, designated initializers, etc.). The C++
version is noted whenever a feature isn't available everywhere.

## Progress

See [`docs/examples/README.md`](docs/examples/README.md) for the full index and
status across every topic.
