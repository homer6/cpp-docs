# Guideline evals — ownership & lifetime pilot

A small, vendor-neutral suite for measuring whether a coding agent applies the
C++ Core Guidelines **correctly**: it finds real violations, cites the right
rule, leaves correct code alone, respects project constraints, and asks when the
code doesn't say enough.

It's designed to compare the **same agent with and without** the
[`cpp-guidelines`](../.claude/skills/cpp-guidelines/SKILL.md) skill. Related
upstream proposal: https://github.com/isocpp/CppCoreGuidelines/issues/2342

## Scope of this pilot

21 cases on resource management, ownership, and lifetime:
R.1, R.3, R.5, R.11, R.20, R.21, I.11, C.13, Lifetime.1 / ES.65, F.43,
F.15–F.18, ES.20, E.25.

Rule IDs refer to the edition pinned in `modules/CppCoreGuidelines`.

**Reference coverage ≠ evaluated coverage.** The skill can point an agent at
every guidelines page; only the rules listed above have cases here.

## Case format

Each case is a pair of files in `cases/`:

- `<id>.cpp` — a small, self-contained snippet that compiles on its own.
- `<id>.json` — the expected outcome:

```json
{
  "id": "r3-nullable-observer-param",
  "outcome": "compliant",
  "rules": ["R.3", "F.7"],
  "context": { "std": "c++20", "exceptions": true },
  "expect": {
    "must_not_flag": ["R.3", "R.20", "R.11"],
    "must_not_change": ["parameter must stay a non-owning pointer"]
  },
  "notes": "Why the expected outcome is right.",
  "source": "held-out (written for this suite, not copied from the Guidelines' examples)"
}
```

| `outcome` | Meaning | A good answer |
|---|---|---|
| `violation` | The code breaks a rule | Flags it, cites one of `expect.must_cite_any`, proposes a fix with every `expect.fix.properties` |
| `compliant` | The code is correct | Flags nothing in `must_not_flag`; changes nothing in `must_not_change` |
| `exception` | A rule's documented exception applies | Doesn't report a violation of `must_not_cite_as_violation`; may mention it softly |
| `constraint` | A project constraint changes the answer (e.g. `"exceptions": false`) | Respects the constraint; doesn't recommend what it forbids |
| `needs-context` | The code can't be judged without more information | Says what's missing and asks (or states an assumption) instead of rewriting — see `expected_behaviour` |

`context` is given to the agent along with the code: language standard,
whether exceptions are enabled, and anything else a real project would tell it.
`context.requires` lists library features a compiler must have.

## Running an evaluation

There's no runner tied to a particular agent. For each case:

1. Give the agent the `.cpp` file, the `context`, and the same task prompt for
   every case, for example: *"Review this code against the C++ Core Guidelines.
   Report any problems with rule IDs, and propose minimal fixes."*
2. Score the answer against the `.json` (by hand, or with an LLM judge whose
   output a human spot-checks).
3. If the agent proposes a fix, compile it (`-std=<context.std> -fsyntax-only`,
   plus `-fno-exceptions` when `exceptions` is false) and check the fix
   properties.

Run once **without** the skill and once **with** it, same model and settings.

## Scoring

Record per case, then aggregate per rule:

| Measure | Counts when |
|---|---|
| **Missed violation** | `violation` case, nothing relevant flagged |
| **Wrong attribution** | Problem flagged, but none of `must_cite_any` cited |
| **False positive** | `compliant` / `exception` / `constraint` case flagged |
| **Incorrect edit** | Fix doesn't compile, breaks a `fix.properties` item, or changes something in `must_not_change` |
| **Ignored constraint** | Recommends something `context` rules out (e.g. exceptions with `"exceptions": false`) |
| **Didn't ask** | `needs-context` case rewritten without stating what's unknown |

Always record: model name and version, agent/client and version, skill commit,
compiler and version, date, the exact task prompt, and tokens and wall time
per run. A skill that helps but triples the cost is a different trade-off from
one that helps cheaply.

Score detection, rule attribution and fix validity as **separate measures**,
not one overall grade. ContextCRBench (arXiv 2511.07017, Nov 2025) splits code
review the same way: hunk-level assessment, line-level defect localization and
comment generation are three tasks.

### Judging

- Check mechanical items (the fix compiles, the cited rule ID exists) with a
  script, not a model.
- If an LLM judges the rest, **have a human review a sample of its verdicts**.
  Don't count agreement between models as correctness. In a census of 22,854
  repositories' `AGENTS.md`/`CLAUDE.md` files (arXiv 2610.31715, v2 Oct 26,
  2026), the annotating models agreed far more with each other than with the
  human reviewer.
- To compare two skill versions overall, show a judge both answers without
  saying which version produced which (blind comparison).
- Drop measures that pass in both arms; they tell you nothing about the
  skill.

The [Agent Skills guide to evaluating skills](https://agentskills.io/skill-creation/evaluating-skills)
describes the same with/without-skill loop. It keeps cases in
`evals/evals.json` inside the skill directory, but that's a convention, not
part of the format spec. This suite keeps one `.cpp` + `.json` pair per case
so every snippet can be compiled.

## clang-tidy checks that cite these rules

A check whose docs cite a rule covers **part** of it at most. Use these as a
baseline to compare the agent against, not as the expected answer.

| Check (clang-tidy 23.1 docs) | Rules it cites | Limits |
|---|---|---|
| `cppcoreguidelines-owning-memory` | I.11, R.3, C.33, GSL.view | Type-based, not flow-sensitive; only knows owners marked `gsl::owner<T*>` |
| `cppcoreguidelines-no-malloc` | R.10 | `malloc`/`free` family only |
| `cppcoreguidelines-init-variables` | ES.20, Type.5 | Local variables |
| `cppcoreguidelines-rvalue-reference-param-not-moved` | F.18 | — |
| `cppcoreguidelines-missing-std-forward` | F.19 | — |
| `cppcoreguidelines-special-member-functions` | C.21 | — |

R.1, R.5, R.11, R.20, R.21, C.13 and the Lifetime profile have no check in
this table. That only means none was found, not that none exists.

## Planned next steps

Design proposals for growing the suite (from the issue #3 research report).
They're plans, not measured improvements:

- For each `needs-context` case, write down the clarification a good answer
  should ask for.
- Add attribution traps: plausible but wrong rule IDs a reviewer might cite.
- Add cases where a constraint makes the usual rewrite wrong (ABI, externally
  owned resources).
- Keep a held-out set that never appears in the skill or its examples.
- When a case gets near-duplicate variants, report results per case family
  as well as per case, so variants aren't counted as independent findings.
- Add arms for the comparison: minimal guidance, and irrelevant guidance of
  the same length, to check whether extra context alone explains any gain.

## Checking the suite itself

```bash
git submodule update --init modules/CppCoreGuidelines
python3 evals/check.py                 # uses $CXX, default c++
CXX=g++-15 python3 evals/check.py
```

This validates the metadata, checks every rule ID exists in the pinned
Guidelines, and compiles each case with `-fsyntax-only`. It does **not** run an
agent.

Tested: all 21 cases pass with GCC 15. Apple Clang 15's libc++ has no
`std::jthread`, so the two C.13 cases need a newer libc++ (see
`context.requires`).

## Adding cases

- Write new code — don't copy the Guidelines' own examples (agents have seen
  them). Mark `source` accordingly.
- Pair every `violation` case with a nearby `compliant` case, so blanket rewrites
  are caught (e.g. a non-owning raw pointer next to an owning one).
- Keep each case to one main issue, and let `must_cite_any` list every rule a
  reasonable reviewer could cite.
