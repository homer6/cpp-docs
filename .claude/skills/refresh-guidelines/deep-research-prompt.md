# Deep research prompt — modern C++ best practices

Paste everything below the line into Perplexity Deep Research (or
`perplexity_research`). Save the report as
`.firecrawl/deep-research-<YYYY-MM-DD>.md`, then ask Claude to run the
`refresh-guidelines` skill in **import** mode on it.

Update the "already covered" list before each run so the report focuses on
what's new.

---

I maintain a set of C++ coding guidelines for experienced engineers, targeting
**C++26** (with C++20/23 as the practical baseline most compilers support well).
I need a deep, well-sourced research report on **current best practices for
writing modern C++ (C++20, C++23, C++26)**, drawn from the most respected
industry and committee sources. Today's date is {{DATE}}.

## What I already cover (don't re-explain these basics; only report changes, disagreements, or nuance)

- C++ Core Guidelines (Jun 2026 edition): RAII, smart pointers, Rule of Zero/Five,
  parameter passing (F.15–F.21), `{}` initialization, `const` by default, safety
  profiles (type/bounds/lifetime)
- C++26 contracts (`pre`/`post`/`contract_assert`), static reflection (`^^`,
  splicers, `template for`), `std::execution`, hardened standard library,
  erroneous behaviour for uninitialized reads and `[[indeterminate]]`,
  `std::optional<T&>`, `std::inplace_vector`, `std::function_ref`,
  `std::indirect`/`std::polymorphic`
- Ben Deane-style design: illegal states unrepresentable, strong types,
  `std::variant` for closed sets, no raw loops, monoids/folds, `std::expected`
  for errors, value semantics, concepts over inheritance

## Questions to answer

1. **Abseil** — Which "Tips of the Week" (abseil.io/tips) and Abseil
   guidance remain most valuable in a C++23/26 world? Which have been
   superseded by the standard (e.g. `absl::StatusOr` vs `std::expected`,
   `absl::Span` vs `std::span`, `absl::string_view`, `absl::flat_hash_map` vs
   `std::unordered_map`/`std::flat_map`)? List the top ~25 tips by lasting value,
   each with its number, title, one-line lesson, and whether a standard facility
   now replaces it.
2. **Major style guides** — Summarize the current positions of: Google C++ Style
   Guide, Chromium C++ style and its allowed/banned modern-features list, LLVM
   Coding Standards, Mozilla, Bloomberg BDE, and the SEI CERT C++ Coding
   Standard. Focus on where they **disagree** (exceptions, RTTI, `auto`,
   `std::function`, coroutines, modules, ranges, `std::format`, naming) and
   *why*. A comparison table would help.
3. **C++26 adoption guidance** — Which C++26 features are experts recommending
   for production use now, which should wait, and why? Include current
   compiler and standard-library support (GCC, Clang/libc++, MSVC/STL) for
   contracts, reflection, `std::execution`, hardening, and erroneous behaviour,
   as of the latest releases.
4. **Contested standard status** — Settle these with primary sources (WG21
   papers, the final C++26 working draft, or committee trip reports):
   - Is a coroutine task type (P3552, `std::execution::task`) in final C++26?
   - Is trivial relocatability (P2786, `trivially_relocatable_if_eligible`) in
     final C++26, or was it removed (Kona, Nov 2025)?
   - What is the status of safety profiles after C++26 (TS, white paper,
     C++29 target)?
5. **Memory safety in practice** — Current industry consensus on making C++
   safer: hardening modes (libc++ `_LIBCPP_HARDENING_MODE`, libstdc++
   `_GLIBCXX_ASSERTIONS`), compiler flags (`-ftrivial-auto-var-init`,
   `-fstack-protector-strong`, `_FORTIFY_SOURCE`, `-fbounds-safety`),
   sanitizers in CI, and what Google, Microsoft, Apple and Chromium report
   about their real-world results. Include measured overheads where published.
6. **Tooling best practices** — Recommended clang-tidy check sets, warning flags
   (`-Wall -Wextra -Wpedantic -Wconversion …`), static analysis, fuzzing, and
   build practice for modules (`import std;`) as of today.
7. **Expert talks and writing (2024–2026)** — The most influential recent talks
   and articles on modern C++ practice (e.g. Herb Sutter, Bjarne Stroustrup,
   Sean Parent, Ben Deane, Jason Turner, Barry Revzin, Hana Dusíková, Timur
   Doumler, Titus Winters). For each: title, venue, year, link, and its one or
   two key recommendations.
8. **Anything I'm missing** — Important modern-C++ best practices or emerging
   consensus not listed above (e.g. around `std::print`/`std::format`,
   `std::mdspan`, `std::simd`, `std::linalg`, deducing `this`, ranges pitfalls,
   coroutine lifetime traps, error-handling strategy).

## Output format

- Organize the report under these headings, so I can merge it into my pages:
  **Core rules · Memory & safety · Modern idioms (C++20/23/26) · Design
  principles · Project conventions & tooling · Sources**.
- Write each recommendation as: **short imperative rule** → one-line reason →
  source(s) → standard version required → (optional) a minimal BAD/GOOD
  `cpp` snippet.
- Where sources disagree, show **both positions and their reasons**. Don't pick
  one silently.
- **Cite primary sources** for every claim (cppreference.com, open-std.org
  papers, isocpp.github.io Core Guidelines, abseil.io, the style guides
  themselves, conference video/slides). Avoid unsourced blog summaries and AI
  content farms.
- Mark each fact about standard status or compiler support with a **confidence
  level** and the **date** of its source. Flag anything where sources conflict.
- Don't invent APIs, paper numbers, or tip numbers. If something can't be
  verified, say so.
- End with a **candidate sources table**: name, URL, what it's best for,
  update frequency, licence (can it be quoted/summarized?).
