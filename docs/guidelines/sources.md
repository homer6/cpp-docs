# Sources

Where each guidelines page came from, which edition it reflects, and when it was
last checked. The `refresh-guidelines` skill reads and updates this file — keep
it accurate.

## Tracked sources

| ID | Source | URL | Edition / version tracked | Last checked |
|---|---|---|---|---|
| `ccg` | C++ Core Guidelines (Stroustrup, Sutter) | https://github.com/isocpp/CppCoreGuidelines — submodule at `modules/CppCoreGuidelines` | Jun 14, 2026 (commit `d1dda82`, the submodule pin) | 2026-10-30 |
| `cppref` | cppreference.com — C++26 / C++23 / C++20 pages | https://en.cppreference.com/w/cpp/26 | C++26 (final) | 2026-10-30 |
| `deane-types` | Ben Deane, *Using Types Effectively* | CppCon 2016 | — | 2026-10-30 |
| `deane-misuse` | Ben Deane, *Easy to Use, Hard to Misuse: Declarative Style in C++* | C++Now 2018 | — | 2026-10-30 |
| `deane-composable` | Ben Deane, *Composable C++: Principles and Patterns* | CppCon 2021 | — | 2026-10-30 |
| `automerge-style` | automerge-cpp style guide and architecture plan | https://github.com/homer6/automerge-cpp (`docs/style.md`, `docs/plans/architecture.md`) | commit `10d71bc` and later | 2026-10-30 |

## Page → sources

| Page | Sources |
|---|---|
| [core-guidelines.md](core-guidelines.md) | `ccg`, `cppref` |
| [safety.md](safety.md) | `ccg`, `cppref` |
| [modern-idioms.md](modern-idioms.md) | `cppref` |
| [design-principles.md](design-principles.md) | `deane-*`, `automerge-style` |
| [project-conventions.md](project-conventions.md) | `automerge-style`, `ccg` (F.16, NL) |

## Candidate sources (not yet imported)

Notable references worth considering for a future import. Move a row up to
"Tracked sources" when it's imported.

| Source | Why it might be worth it |
|---|---|
| Google C++ Style Guide | Widely used; a contrasting set of choices (e.g. on exceptions) |
| LLVM Coding Standards | Large-codebase conventions, compile-time hygiene |
| Abseil "Tips of the Week" | Short, practical idioms with rationale |
| SEI CERT C++ Coding Standard | Security-focused rules |
| Jason Turner, *C++ Best Practices* | Tooling, warnings, sanitizers |
| Sean Parent, *C++ Seasoning* / "no raw loops" | Origin of several design-principles ideas |

## Change log

| Date | Change |
|---|---|
| 2026-10-30 | Section created. Imported the `cpp-modern` skill (clustera-bridge, Dec 2025; CCG Jul 8 2025) and the automerge-cpp style guide. Updated to CCG Jun 14 2026 (new C.13, removed T.46, F.16 threshold 4 words, ES.23 slicing note) and C++26 (contracts, reflection, hardening, erroneous behaviour, `optional<T&>`, …). Fixed errors in the originals: invented `std::lazy`, invalid "defaulted lambda" example, `if consteval` labelled C++20 (it's C++23), wrong profile rule names (Type.2, Lifetime.2–4), backwards F.15 comment, `constexpr std::vector` overreach, and `std::reduce` needing commutativity. |
| 2026-10-30 | Core Guidelines added as a submodule at `modules/CppCoreGuidelines`, tracking `master`. Pulled upstream: still at `d1dda82` (Jun 14, 2026 edition), so no page changes. |
