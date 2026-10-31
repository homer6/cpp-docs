# Deep research prompt — gaps from the first import (issue #4)

Paste everything below the line into Perplexity Deep Research (or
`perplexity_research`). Save the report as
`.firecrawl/deep-research-issue-4-<YYYY-MM-DD>.md`, then ask Claude to run the
`refresh-guidelines` skill in **import** mode on it, on the
`docs/4-verify-research-gaps` branch.

Tracks https://github.com/homer6/cpp-docs/issues/4.

---

I maintain C++ coding guidelines targeting C++26. A first research pass left
four gaps. Some of each was verified on {{DATE}} from vendor pages (noted
below). I need a deep, primary-source report that fills in the rest. Today's
date is {{DATE}}.

## Already verified (confirm only if something changed; don't re-explain)

- GCC 16: contracts (P2900), reflection (P2996, `-freflection`), expansion
  statements (P1306), erroneous behaviour (P2795). libstdc++ (GCC 16):
  experimental `optional<T&>`, `function_ref`, `copyable_function`,
  `inplace_vector`, `indirect`/`polymorphic`, `std::simd`.
- Clang's status page: contracts, reflection, and erroneous behaviour "No";
  expansion statements "Clang 23 (Partial)".
- Microsoft's conformance page (updated 2026-10-16) has no C++26 rows; the STL
  tracks C++26 on a GitHub project board.
- libc++: `optional<T&>` and `function_ref` complete; contracts, `<meta>`,
  hardening (P3471), `indirect`/`polymorphic`, `std::execution::task` not
  marked complete.
- Mozilla: C++20 required since Mozilla 147; exceptions and RTTI disabled;
  coroutines and `std::format` not allowed (use `fmt`).
- `std::execution::task` debate: P4007R3, P4089R0, P4123R0 summarized.
- MSVC STL hardening: P3471 partial since VS 2022 17.14 (May 2025), opt-in
  `_MSVC_STL_HARDENING=1`, off by default, violations call `__fastfail()`
  because C++26 contracts aren't implemented (microsoft/STL#5286 open);
  P3697R1 added in VS 2026 18.0 (MSVC Build Tools 14.50).
- Croydon (Mar 2026) adopted P3980R1, P3941R4, P3927R2, P4151R1 (libc++
  status page), addressing some of the issues P4007 lists.

## Still open from the last run (prioritize these)

The 2026-10-31 report verified only the MSVC hardening item above. Everything
below was "not established" or "unverified" — answer from primary sources or
say "not found":

- MSVC (VS 2026 18.x) and Clang/libc++ release-by-release rows for contracts,
  reflection, `template for`, erroneous behaviour, `std::execution`, `task`,
  `inplace_vector`, `indirect`/`polymorphic`, `std::simd`, `std::linalg` —
  tie each to a release note or tag, not a project-board "done" card.
- Has MSVC STL hardening become on by default in any VS 2026 release?
- Apple Clang / Xcode → upstream Clang mapping.
- BDE: any public normative coding standard (exceptions, RTTI, `auto`,
  allocators, `BSLS_ASSERT` vs C++26 contracts).
- CERT C++: is the published edition still 2016, and what has changed in the
  living rule collection since?
- Talk catalogue 2024–2026 with official video/slides links.
- `task`: SG1/LEWG polls and decisions after Croydon (Brno 2026 and later),
  P3552 author responses, C++29 or defect-report plans, implementer
  performance data.

## Questions

1. **Compiler and library support beyond the vendor status pages.**
   - Clang/libc++ and MSVC/STL: release-by-release C++26 support for contracts,
     reflection, `template for`, erroneous behaviour, standard-library hardening
     (P3471), `std::execution` (P2300) and `std::execution::task` (P3552),
     `inplace_vector`, `indirect`/`polymorphic`, `std::simd`, `std::linalg`.
     Include any experimental forks (e.g. Bloomberg's clang-p2996 reflection
     fork, EDG) and whether they're usable in production.
   - The MSVC STL C++26 project board: which of these are done, in progress,
     or not started, with the Visual Studio version that ships them.
   - Apple Clang / Xcode: which upstream Clang version it tracks and what
     that means for C++26.
2. **Bloomberg BDE and SEI CERT C++.**
   - Does Bloomberg publish a normative C++ coding standard (BDE coding
     standards, "BDE Style")? What does it say about exceptions, RTTI, `auto`,
     allocators (`bslma`), and contracts (BDE's `BSLS_ASSERT` / contracts
     work)? Primary documents only.
   - SEI CERT C++: which rules most constrain modern C++ feature use
     (exceptions, `noexcept`, containers, concurrency, memory)? Is there any
     update beyond the 2016 edition?
3. **Verified talk catalogue, 2024–2026.** The most influential talks on modern
   C++ practice by Herb Sutter, Bjarne Stroustrup, Sean Parent, Ben Deane,
   Jason Turner, Barry Revzin, Hana Dusíková, Timur Doumler, and Titus Winters
   (and others of similar standing). For each: exact title, conference, year,
   official video or slides link, and the one or two practices it recommends.
   **Only include talks with an official video or slides.**
4. **`std::execution::task` debate.** What did the committee do with the issues
   in P4007, P4089, and P4123 at Brno (2026) and later? Are there responses
   from the P3552 authors (Dietmar Kühl et al.) or SG1/LEWG polls? Is any fix
   planned for C++29 or as a defect report? What do implementers (NVIDIA
   stdexec, libc++, libstdc++, MSVC) say about performance?

## Output format

- Headings: **Toolchain support · BDE · CERT · Talks · task debate · Sources**.
- Toolchain support as tables: feature × release, each cell with a version or
  "no", a confidence level, and a source date.
- Cite primary sources only: vendor status pages and release notes, WG21
  papers (open-std.org), official conference channels, the standards
  documents themselves. No unsourced blog summaries.
- Mark every claim with **confidence** and **source date**. Say "not found"
  rather than guess. Don't invent paper numbers, versions, or talk titles.
