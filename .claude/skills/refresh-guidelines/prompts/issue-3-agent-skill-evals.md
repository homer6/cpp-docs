# Deep research prompt — agent skills and evals for C++ guidelines (issue #3)

Paste everything below the line into Perplexity Deep Research. Save the report
as `.firecrawl/deep-research-issue-3-<YYYY-MM-DD>.md`, then ask Claude to import
it into the `feat/3-guidelines-agent-skill` PR with the `refresh-guidelines`
skill (import mode: verify each claim against the primary source it cites).

---

I'm building (1) a portable **agent skill** that helps coding agents apply the
C++ Core Guidelines when writing and reviewing C++, and (2) a
**vendor-neutral evaluation suite** that measures whether the skill helps:
correct rule attribution, few false positives, correct fixes, and asking for
context when needed. This relates to an open proposal on the Core Guidelines
repository (https://github.com/isocpp/CppCoreGuidelines/issues/2342). Today's
date is {{DATE}}.

## What I already have (don't re-explain)

- A `SKILL.md` that tells an agent to check the project's C++ standard and
  constraints, load relevant guideline pages on demand, cite rule IDs, and
  validate with compiler/tests/clang-tidy.
- A 21-case pilot on ownership and lifetime rules (R.1, R.3, R.5, R.11, R.20,
  R.21, I.11, C.13, Lifetime.1, F.15–F.18, ES.20, E.25). Each case is a `.cpp`
  file plus JSON metadata with an outcome: violation / compliant / exception /
  constraint / needs-context.

## Already verified (from the 2026-10-31 report; don't repeat)

- Agent Skills spec (agentskills.io/specification, originally from Anthropic,
  open standard): required `name` (≤64, lowercase/digits/hyphens, matches the
  directory) and `description` (≤1024); optional `license`, `compatibility`
  (≤500), `metadata`, `allowed-tools` (experimental); `scripts/`,
  `references/`, `assets/`; progressive disclosure (metadata ~100 tokens,
  body <5000 tokens recommended, under 500 lines); file references relative
  to the skill root, one level deep; `skills-ref validate`.
- agentskills.io's evaluating-skills guide: with/without-skill runs,
  assertions, grading with evidence, tokens and time, blind comparison,
  human review.
- ContextCRBench (arXiv 2511.07017): three review tasks (hunk assessment,
  line localization, comment generation).
- AGENTS.md/CLAUDE.md census (arXiv 2610.31715): 22,854 repos; models agree
  more with each other than with the human reviewer.
- Configuration smells (arXiv 2606.15828): six smells across 100 repos.
- Instruction categories (arXiv 2610.21421): five categories, 31
  combinations, Gemma 4 and GPT-5.4, five Python libraries.
- clang-tidy 23.1 checks whose docs cite rules: `owning-memory` (I.11, R.3,
  C.33, GSL.view), `no-malloc` (R.10), `init-variables` (ES.20, Type.5),
  `rvalue-reference-param-not-moved` (F.18), `missing-std-forward` (F.19),
  `special-member-functions` (C.21).

## Still open (prioritize these)

1. **Client support** — which agents/clients load Agent Skills, since which
   version, and how each discovers and loads them (the agentskills.io client
   page didn't render for us). Cite each client's own docs.
2. **Skills outside the skill directory** — how clients handle a skill whose
   instructions point at files in the surrounding repo rather than its own
   `references/`.
3. **clang-tidy coverage** for R.1, R.5, R.11, R.20, R.21, C.13, ES.65 and
   the Lifetime profile, from check docs that cite those rules; MSVC Core
   Check's documented rule mapping.
4. **Ablation numbers** — published with/without effect sizes for guidance
   files or skills on coding or review tasks. Quote the paper's own numbers.
   (2610.21421's abstract says the number of instructions alone didn't
   determine token usage; it doesn't say more instructions hurt code quality.)
5. **"Evaluating AGENTS.md: Are Repository-Level Context Files Helpful for
   Coding Agents?"** and **"Too Noisy To Learn"** — exact arXiv IDs and
   findings (the last report cited the wrong paper for the second).
6. **Prior art and licences** — C++ guideline datasets, linters or agent rule
   packages, with licence and pinned commit.

## Questions to answer (first run; the open list above takes priority)

1. **Agent Skills format** — What is the current state of the open "Agent
   Skills" (`SKILL.md`) format? Who maintains the specification, what fields
   and directory conventions does it define, and which coding agents and
   clients support it (and since when)? Note differences between clients
   (frontmatter fields, how skills are discovered and loaded, size limits).
2. **Evaluating coding agents on review tasks** — What methodology do
   published benchmarks and papers use for evaluating LLM code review,
   static-analysis-style findings, or guideline compliance? Specifically:
   measuring false positives vs missed findings, judging whether the cited
   rule is correct, held-out vs contaminated test cases, LLM-as-judge
   reliability and how to validate it, and checking that proposed fixes
   compile and preserve behaviour. Name the benchmarks/papers, their venues
   and dates, and what they found.
3. **Skill or prompt ablations** — Any published results comparing the same
   model with and without guidance documents, rules files, or skills for
   coding tasks. How large were the effects, and how were they measured?
4. **clang-tidy ↔ Core Guidelines mapping** — Which clang-tidy checks
   (`cppcoreguidelines-*`, `bugprone-*`, `modernize-*`, others) correspond to
   which Core Guidelines rules, especially R.*, I.11, C.13, F.15–F.18, ES.20,
   ES.65, and the Type/Bounds/Lifetime profiles? Which rules have no automated
   check? Include MSVC Core Check and other analyzers if they map rules
   explicitly.
5. **Prior art** — Existing projects that package the Core Guidelines (or
   other C++ style guides) for tools or agents: guideline-linked linters,
   rule datasets with labelled examples, LLM code-review tools for C++,
   cursor/agent rules files for C++. What can be reused, and under what
   licence?
6. **Keeping in sync** — How do similar projects keep derived material in
   sync with a living upstream document (pinning commits, regenerating
   indexes, detecting changed rules)?

## Output format

- Headings: **Agent Skills format · Evaluation methodology · Ablation evidence
  · clang-tidy mapping · Prior art · Sync strategies · Recommendations for my
  pilot · Sources**.
- For the clang-tidy mapping, a table: rule ID → check name(s) → coverage
  (full / partial / none) → source.
- Every claim cited to a primary source (specifications, official docs,
  papers on arXiv/ACL/ICSE/FSE, repositories), with its date and a confidence
  level. Flag anything where sources conflict.
- Don't invent benchmark names, numbers, or check names. If something can't
  be verified, say so.
- End with a short list of concrete changes you'd make to my pilot
  (case design, scoring, sample size, judging).
