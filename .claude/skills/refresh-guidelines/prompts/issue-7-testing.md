# Deep research prompt — C++ testing practice (issue #7)

Paste everything below the line into Perplexity Deep Research. Save the report
as `.firecrawl/deep-research-testing-<YYYY-MM-DD>.md`, then ask Claude to run the
`refresh-guidelines` skill in **import** mode on it, targeting
`docs/guidelines/testing.md`.

---

I maintain C++ coding guidelines for experienced engineers, targeting **C++26**
with C++20/23 as the practical baseline. I need a deep, well-sourced research
report on **current best practice for testing modern C++** — as of {{DATE}}.

## What I already cover (only report changes, disagreements, or nuance)

- Test naming after behaviour, arrange/act/assert, one logical property per test
- GoogleTest (latest 1.18.0, requires C++17), Catch2 v3 (3.16.1), doctest (2.5.3)
  basics
- Property-based testing of algebraic laws (commutativity, associativity,
  idempotence, identity) with RapidCheck (`rc::check`, `RC_GTEST_PROP`; no
  tagged releases)
- libFuzzer harness basics (`LLVMFuzzerTestOneInput`, `-fsanitize=fuzzer,address`)
- Source-based coverage with Clang (`-fprofile-instr-generate -fcoverage-mapping`,
  `llvm-profdata`, `llvm-cov`) and GCC (`--coverage`, gcov/gcovr)
- `static_assert` + concepts/requires-expressions to check that misuse fails to
  compile
- Verified 2026-10-31 from GitHub release tags: GoogleTest 1.18.0
  (2026-08-10, C++17 minimum since 1.17.0), Catch2 3.16.1 (2026-10-08, C++14
  minimum), doctest 2.5.3 (2026-07-06, C++11); FuzzTest is tagged by date
  (latest 2026-06-29)
- Verified from P2900R14 (2025-02-13): the handler takes
  `const std::contracts::contract_violation&` and returns void; no header
  declares it; replaceability is implementation-defined; a throwing handler
  is allowed and skips `enforce` termination; for `pre`/`post` the exception
  acts as if thrown from the function body (so `std::terminate` if the
  function is `noexcept`, and a function-try-block doesn't catch it); for
  `contract_assert` the handler search continues from the statement
- GCC 13's `-fcontracts` is the C++20-era design (N4820 + P1290, P1332,
  P1429), not P2900 (GCC 13 manual)
- GCC rejects combining `-fsanitize=thread` with `-fsanitize=address`

## Still open from the last run (prioritize these)

- Which shipping compilers/libraries implement P2900 today (GCC 16? Clang?
  libc++/libstdc++), their flags for choosing the evaluation semantic, and
  whether each lets you replace `handle_contract_violation`
- Framework compile-time comparison and feature matrix, including Boost.Test,
  Boost.UT and snitch, from measurements (not framework self-benchmarks)
- RapidCheck and FuzzTest: maintenance, supported platforms/compilers,
  shrinking, seed/counterexample reproduction, GoogleTest/Catch2 integration
- libFuzzer maintenance stance, AFL++ commands, Centipede vs FuzzTest,
  OSS-Fuzz/ClusterFuzzLite setup, structure-aware fuzzing APIs
- Clang MC/DC minimum version and flags; gcov/gcovr/lcov differences;
  template/inline aggregation; merging across binaries
- CMake `try_compile`/`WILL_FAIL` and LLVM `-verify` mechanics for negative
  compilation tests
- Relacy/CDSChecker maintenance; TLA+ bridges
- A talk catalogue (official video/slide links only)

## Questions to answer

1. **Frameworks** — How do GoogleTest, Catch2 v3, doctest, Boost.Test, and newer
   options (e.g. `ut`/Boost.UT, snitch) compare today on compile time, C++
   standard required, parameterized/typed tests, death tests, mocking (gMock vs
   alternatives like trompeloeil), sanitizer friendliness, and maintenance
   (release cadence in 2025–2026)? A comparison table with dates.
2. **Property-based testing** — Which C++ libraries are maintained in 2025–2026
   (RapidCheck, libFuzzer-driven properties such as FuzzTest from Google,
   others)? How does Google's **FuzzTest** combine property tests and fuzzing,
   and what are its platform/compiler requirements? Shrinking support,
   generator APIs, GoogleTest/Catch2 integration.
3. **Fuzzing** — Current libFuzzer status (LLVM's stance on maintenance), AFL++
   (persistent mode, `afl-clang-lto`), Centipede/FuzzTest, OSS-Fuzz and
   ClusterFuzzLite for CI. Recommended sanitizer combinations, corpus and
   dictionary management, structure-aware fuzzing (libprotobuf-mutator), and
   how to turn crashes into regression tests.
4. **Testing C++26 contracts** — With P2900 contracts: how should tests check
   that a precondition fires? Can a user-provided
   `handle_contract_violation(const std::contracts::contract_violation&)` throw,
   and does the exception propagate out of the function whose precondition
   failed (cite the exact P2900R14 wording)? Death tests under `enforce` vs
   `observe` + custom handler. What do GCC 16 and libc++ support today (flags
   such as `-fcontracts`, evaluation-semantic selection)?
5. **Coverage** — Best practice for gcov/gcovr/lcov vs llvm-cov in 2026: branch
   vs line vs MC/DC coverage (Clang's MC/DC support and version), coverage of
   templates and inline functions, merging coverage across test binaries, and
   what coverage targets experts actually recommend (and warn against).
6. **Compile-fail (negative compilation) tests** — Techniques to assert that
   misuse does not compile: `static_assert(!requires { ... })`, concept
   checks, CMake `try_compile`/`WILL_FAIL` tests, LLVM `lit`-style
   `expected-error` tests, Catch2/doctest support. Pros, cons, and pitfalls
   (e.g. SFINAE-unfriendly errors that can't be detected with `requires`).
7. **Testing concurrency** — Running tests under TSan, stress/repeat runs
   (`--gtest_repeat`, `--gtest_shuffle`), deterministic schedulers or model
   checkers usable from C++ (e.g. Relacy, CDSChecker, TLA+ bridges).
8. **What experts recommend** — Recent (2023–2026) talks or articles on C++
   testing practice from recognized authors, each with title, venue, year, and
   link (official video or slides only — no invented talks).

## Output format

- Headings: **Frameworks · Property-based testing · Fuzzing · Contracts ·
  Coverage · Compile-fail tests · Concurrency · Sources**.
- Each recommendation: **short imperative rule** → one-line reason → source(s)
  → standard/tool version required → (optional) minimal `cpp` snippet.
- Where sources disagree, show both positions and why.
- Cite primary sources: official docs and repos (google.github.io/googletest,
  github.com/catchorg/Catch2, github.com/doctest/doctest,
  github.com/emil-e/rapidcheck, github.com/google/fuzztest, llvm.org/docs,
  clang.llvm.org/docs, gcc.gnu.org/onlinedocs, aflplus.plus, google.github.io/oss-fuzz),
  WG21 papers on open-std.org for contracts.
- Mark each fact about versions or support with a **confidence level** and the
  **date** of its source. Don't invent APIs, macros, flags, or versions; say
  when something can't be verified.
- End with a candidate sources table: name, URL, best for, update pattern,
  licence.
