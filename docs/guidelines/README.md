# C++ guidelines — how to write it well

The rest of this repo (`docs/examples/`) is a **reference**: what each language
feature and library facility *is*. This section is **opinion**: how to *use*
them — which rules to follow, which idioms to reach for, and which traps to
design out.

The guiding question throughout: **can the compiler reject the mistake?**

## Pages

| Page | What's in it | Main source |
|---|---|---|
| [Core Guidelines digest](core-guidelines.md) | The rules that matter most, by section (P, R, F, C, ES, E, Con, CP, T, Per) | C++ Core Guidelines |
| [Memory & safety](safety.md) | Safety profiles, ownership, dangling, C++26 hardening and erroneous behaviour | C++ Core Guidelines + C++26 |
| [Modern idioms](modern-idioms.md) | C++17 → C++26 features and when to reach for each | cppreference |
| [Design principles](design-principles.md) | Types model the domain, no raw loops, monoids, errors in the type system | Ben Deane's talks |
| [Project conventions](project-conventions.md) | Naming, headers, API shape, testing style | House style (from automerge-cpp) |
| [Industry practice](industry-practice.md) | Where Google, Chromium, and LLVM disagree; Abseil tips that still matter | Style guides, Abseil |
| [Tooling](tooling.md) | Sanitizers, debuggers, core dumps, static analysis — what to use for which bug | Clang sanitizer docs |
| [Sources](sources.md) | Where each page came from, which edition, when last refreshed | — |

## How this section differs from the reference pages

- **It cites more than cppreference.** Guidelines come from the C++ Core
  Guidelines, conference talks, and similar sources. Facts about the language
  and library (signatures, since-versions) still defer to cppreference, and
  each guideline links to the matching reference page in `docs/examples/`.
- **Rules carry their source ID** (e.g. **R.11**, **ES.23**) so you can look
  them up in the full Core Guidelines.
- **Every page is dated in [sources.md](sources.md).** To refresh a page or
  import a new source, use the `refresh-guidelines` skill in
  `.claude/skills/refresh-guidelines/`.

## Origin

These notes began as the `cpp-modern` Claude skill in `clustera-bridge`
(Dec 2025, Core Guidelines Jul 8 2025 edition) and the automerge-cpp style
guide (Ben Deane principles). They were merged here in Oct 2026, corrected, and
updated to C++26 and the Jun 14 2026 Core Guidelines edition.
