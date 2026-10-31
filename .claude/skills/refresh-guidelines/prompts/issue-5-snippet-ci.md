# Deep research prompt — compile-checking C++ snippets in docs (issue #5)

Paste everything below the line into Perplexity Deep Research. Save the report
as `.firecrawl/deep-research-issue-5-<YYYY-MM-DD>.md`, then ask Claude to
import it into the `ci/5-compile-snippets` PR (https://github.com/homer6/cpp-docs/issues/5).

---

I maintain a Markdown documentation repo of modern C++ examples (C++20/23/26),
hundreds of ```cpp blocks, many of them fragments. I've built a stdlib-only
Python checker that extracts each block, prepends a prelude of standard headers
and placeholder declarations, and runs `g++`/`clang++ -std=c++26 -fsyntax-only
-pedantic-errors`. Blocks are tagged in the fence info string: `cpp body`
(wrap in a function), `cpp nocompile` (skip), `cpp needs=<feature>` (skip unless
enabled). It runs in GitHub Actions. Today's date is {{DATE}}.

## Questions to answer

1. **Prior art.** How do established projects compile-test C++ code in
   documentation? Cover at least: mdsnippets / MarkdownSnippets (code pulled
   from compiled source files into docs), Doxygen `\snippet`, the Boost and
   {fmt} docs, cppreference's "Run this code" (Compiler Explorer / Coliru)
   integration, Microsoft Learn's C++ docs pipeline, and Rust's `rustdoc`
   doctests (`ignore`, `no_run`, `compile_fail`, hidden `#` lines) as a design
   comparison. For each: mechanism, how fragments and intentionally-wrong
   examples are handled, and what breaks in practice.
2. **Fragment handling.** Which approaches work best for fragments that use
   undeclared context (`v`, `people`, `Widget`)? Hidden setup lines (Rust's `#`
   prefix), per-file preludes, named snippets extracted from real `.cpp`
   files, or `compile_fail` tests for BAD examples (asserting a snippet *doesn't*
   compile, and ideally with which diagnostic). Trade-offs for readers vs
   maintainers.
3. **Toolchains in CI (as of {{DATE}}).** Exact, current ways to get on GitHub
   Actions: GCC 16 (official `gcc:16` Docker image? PPA?), the latest stable
   Clang with libc++ (apt.llvm.org), and MSVC (windows runners, latest
   `/std:c++latest`). Which flags enable C++26 contracts and reflection on each
   (GCC's `-freflection`, any `-fcontracts`; Clang/EDG reflection forks), and
   which runner images ship which compiler versions.
4. **`-fsyntax-only` limits.** What does a syntax-only check miss compared with
   full compilation (template instantiation, constexpr evaluation, linking),
   and is `-c` or a full compile-and-run worth it for docs? Any differences
   between GCC and Clang in what `-fsyntax-only` diagnoses.
5. **Keeping it maintainable.** Practices for incremental rollout over a large
   existing corpus (baseline/ratchet files of known failures, per-directory
   gates), caching, and running only changed files on PRs.

## Output format

- Headings: **Prior art · Fragment strategies · CI toolchains · Limits ·
  Rollout · Recommendations for this repo · Sources**.
- Each recommendation: **short imperative** → reason → source(s) → effort.
- Cite primary sources (project docs, repo files, GitHub Actions/runner-image
  docs, compiler manuals, Docker Hub). Mark each toolchain fact with a
  **confidence level** and **source date**; flag anything you couldn't verify.
- Don't invent tool names, flags, or image tags.
