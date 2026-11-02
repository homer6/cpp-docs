# Industry practice — style guides and Abseil tips

> What large C++ codebases actually allow, where their guides disagree and why,
> and which Abseil "Tips of the Week" still matter now that the standard has
> caught up. **Sources:** Google C++ Style Guide, Chromium's modern C++ feature
> list, LLVM Coding Standards, Abseil Tips — all checked 2026-10-30; Microsoft GSL and gsl-lite checked 2026-11-01

"Modern C++" isn't one policy. The big guides agree on the fundamentals (RAII,
clear ownership, readable code) and disagree on features, mostly for reasons of
scale, build time, binary size, and migration cost — not because a feature is
bad. Read their restrictions as *trade-offs for that codebase*, then decide
what fits yours.

## Where the big guides disagree

| Feature | Google | Chromium | LLVM | Mozilla |
|---|---|---|---|---|
| Language baseline | C++20 ("should not use C++23 features") | C++20 allowed; C++23 support began Jan 2026, feature by feature; C++26 not yet | — | C++20 required since Mozilla 147, usable "where supported on all platforms" |
| Exceptions | **Not used** | Not used (inherits Google) | **Not used** — to reduce code and binary size | **Disabled** — no `try`/`catch`/`throw` |
| RTTI (`typeid`, `dynamic_cast`) | Avoid | Inherits Google | **Not used**; LLVM has its own opt-in `isa<>`/`dyn_cast<>` | **Disabled** |
| `auto` | Use when it helps the reader, not just to save typing | Inherits Google | "If and only if it makes the code more readable"; not "almost always auto" | Allowed |
| Coroutines | Only through libraries your project leads approve | **Banned** — needs support code and a migration plan | — | **Not allowed** — Clang's coroutines are unmaintained on 32-bit Windows |
| Modules | **Do not use** | **Banned** | — | — |
| Range adaptors and `\|` pipelines | — | **Banned** (design questions, build time, run-time cost), except `views::reverse`, `views::zip`, `views::as_rvalue` — and even those can't be piped | — | — |
| `<format>` | — | **Banned** — format specifiers differ from its existing `StrFormat`; migration is nontrivial | — | **Not allowed** — use `fmt/format.h` (patched for Gecko's float formatting) |
| `std::span` | — | **Banned** — superseded by `base::span` | — | — |
| `std::function`, `std::bind` | — | **Banned** — Chromium has its own callback types | — | — |
| `std::shared_ptr` | — | **Banned** — uses intrusive `base::RefCounted`; switching "would require significant migration" | — | — |
| `std::expected`, `std::print`, `std::flat_map`, `std::generator` | — | **TBD** (under review, C++23) | — | — |
| Standard library in general | — | Allowed, with the bans above | — | Prefers its own MFBT/XPCOM libraries; its list of allowed standard components is marked as a draft |

"—" means the guide doesn't take a specific position that we checked.

**Not in the table:**

- **SEI CERT C++** is a catalogue of secure-coding rules (2016 edition, now
  hosted at [cmu-sei.github.io](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/)),
  not a feature-adoption policy. It takes no position on most of these
  features. Use it for specific security rules, not for this comparison.
- **Bloomberg BDE** — no normative, codebase-wide style policy was found.
  Not checked further.

**What this means for these guidelines:**

- Our [design principles](design-principles.md) use exceptions-free errors,
  ranges pipelines, and `std::variant` freely. That's a choice for new,
  self-contained libraries — not a claim that the guides above are wrong.
- **Write down the reason for every restriction you adopt** (build time, binary
  size, toolchain support, migration cost). A rule without its reason can't be
  revisited when the reason goes away.
- **Publish a feature matrix, not just "requires C++23".** See
  [project conventions](project-conventions.md#feature-matrix).

## Abseil tips that still matter

Abseil's [Tips of the Week](https://abseil.io/tips/) are short design essays
from Google's C++ codebase. Many predate the standard facilities that now
overlap them; the design lessons usually outlive the API. "Standard analogue"
below means an overlapping facility exists — not that switching is automatic.

### Design and API shape

| Tip | Lesson | Standard analogue |
|---|---|---|
| [#1](https://abseil.io/tips/1) `string_view` | Take a borrowed string in interfaces; keep ownership separate. | `std::string_view` (C++17) |
| [#93](https://abseil.io/tips/93) using `absl::Span` | Express "a borrowed run of elements" in the type. | `std::span` (C++20) |
| [#163](https://abseil.io/tips/163) Passing `std::optional` parameters | Don't take `const std::optional<T>&`. Small `T`: take `std::optional<T>` by value. Large `T`: take a nullable `const T*`. `optional` is for things you *own* (members, return values). | `std::optional<const T&>` (C++26) |
| [#173](https://abseil.io/tips/173) Wrapping Arguments in Option Structs | When positional arguments get ambiguous, pass a struct of named options. | Designated initializers (C++20) make it nicer |
| [#177](https://abseil.io/tips/177) Assignability vs. Data Member Types | Avoid `const` and reference data members in value types and move-only types — they break assignment. Immutable types are justified less often than you'd think. | — |
| [#186](https://abseil.io/tips/186) Prefer to Put Functions in the Unnamed Namespace | Keep implementation-only helpers file-local rather than making them members. | — |
| [#218](https://abseil.io/tips/218) Designing Extension Points With FTADLE | Make customization points deliberate, not accidental ADL. | — |
| [#232](https://abseil.io/tips/232) When to Use `auto` for Variable Declarations | Use `auto` when the reader can still tell what matters about the type. | — |

### Errors and checking

| Tip | Lesson | Standard analogue |
|---|---|---|
| [#76](https://abseil.io/tips/76) Use `absl::Status` | Pick one error vocabulary for the project and use it consistently. | `std::expected<T, E>` (C++23) carries a result; it isn't an error vocabulary by itself |
| [#181](https://abseil.io/tips/181) Accessing the value of a `StatusOr<T>` | Check success, then access the value — don't rely on a throwing accessor. | Same for `std::expected` |
| [#224](https://abseil.io/tips/224) Avoid `vector.at()` | Decide whether an index is correct by construction (then don't re-check) or untrusted (then check and handle it explicitly). Either way, `at()` and its exception is the wrong tool. Same for `optional::value()`. | Hardened library catches the "correct by construction" mistakes — see [safety](safety.md#hardening-in-practice) |
| [#141](https://abseil.io/tips/141) Beware Implicit Conversions to `bool` | `if (opt)` tests presence, not the contained value. | — |
| [#147](https://abseil.io/tips/147) Use Exhaustive `switch` Statements Responsibly | Omit `default` only when you own the enum and want new enumerators to force callers to update. | — |
| [#227](https://abseil.io/tips/227) Be Careful with Empty Containers and Unsigned Arithmetic | `v.size() - 1` on an empty container wraps to a huge number. | `std::ssize` (C++20) |

### Lifetimes

| Tip | Lesson | Standard analogue |
|---|---|---|
| [#149](https://abseil.io/tips/149) Object Lifetimes vs. `= delete` | Deleting an overload doesn't make lifetimes safe in general. | — |
| [#180](https://abseil.io/tips/180) Avoiding Dangling References | For every borrow that escapes, know who owns the data. | New view types don't remove the problem |

### Performance and containers

| Tip | Lesson | Standard analogue |
|---|---|---|
| [#112](https://abseil.io/tips/112) emplace vs. push_back | Use `emplace` to construct in place, not as a reflexive replacement for `push_back`. | — |
| [#117](https://abseil.io/tips/117) Copy Elision and Pass-by-value | Reason about the actual copies and moves before choosing a signature. | Guaranteed elision (C++17) |
| [#136](https://abseil.io/tips/136) Unordered Containers | Never depend on hash-table iteration order. | — |
| [#144](https://abseil.io/tips/144) Heterogeneous Lookup in Associative Containers | Look up by `string_view` without building a temporary `std::string`. | Transparent comparators (C++14), transparent hashing (C++20) |
| [#152](https://abseil.io/tips/152) `AbslHashValue` and You | Hashing must agree with equality. | `std::hash` specialization (different mechanism) |
| [#197](https://abseil.io/tips/197) Reader Locks Should Be Rare | Shared/reader locks usually cost more than they save; measure before using one. | `std::shared_mutex` has the same trade-off |
| [#231](https://abseil.io/tips/231) Some Minor Overlooked Algorithms | Check `<algorithm>` before writing another loop. | — |

### Superseded APIs, lasting lessons

| Tip | Lesson | Standard analogue |
|---|---|---|
| [#108](https://abseil.io/tips/108) Avoid `std::bind` | Use lambdas. | `std::bind_front` (C++20) if you must |
| [#124](https://abseil.io/tips/124) `absl::StrFormat()` | Use type-checked formatting. | `std::format` (C++20) — but format strings aren't interchangeable |

**Not the same thing, despite the names:** `std::flat_map` (C++23, a sorted
vector) is not `absl::flat_hash_map` (a hash table), and `std::expected` alone
isn't the whole `absl::Status` ecosystem (error codes, payloads, propagation
macros).

## The GSL in practice

The Core Guidelines lean on a small support library, the
[GSL](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-gsl).
Many rules say "use `gsl::not_null`", "`gsl::narrow`" or "`gsl::owner`". The
Guidelines define its contents, and libraries implement them.

| Library | Standards | Notes |
|---|---|---|
| [Microsoft GSL](https://github.com/microsoft/GSL) 5.0.1 (Sep 2026) | C++14–C++23 | The reference implementation, header-only, namespace `gsl`. Contract failures call `std::terminate`. |
| [gsl-lite](https://github.com/gsl-lite/gsl-lite) 1.1.1 (Aug 2026) | C++98–C++23 | Single header for older codebases. Version 1 uses namespace `gsl_lite` and doesn't define unprefixed `Expects`/`Ensures` by default. |

**Still worth using with C++20/23:**
- `gsl::not_null<T>` / `strict_not_null<T>`: construction from `nullptr` doesn't compile, and a null pointer at run time terminates;
- `gsl::narrow` (throws `gsl::narrowing_error` when the value changes) and `narrow_cast` (states that truncation is intended);
- `gsl::finally`, a one-line scope guard;
- `gsl::at` and `gsl::span`, which are bounds-checked (a hardened standard library gives `std::span` the same; see [safety](safety.md#gsl-guidelines-support-library)).

**Skip the parts the standard replaced:**
- `gsl::byte` (use `std::byte`; deprecated);
- `gsl::unique_ptr` / `shared_ptr` aliases (deprecated);
- `joining_thread`, which Microsoft GSL never implemented (use `std::jthread`).

`Expects`/`Ensures` become `pre`/`post` once your compilers support C++26
contracts.

**Tooling expects the GSL's names.** clang-tidy's Core Guidelines checks are
built around them:
- `cppcoreguidelines-owning-memory` treats only `gsl::owner<T*>` as an owning
  raw pointer;
- `cppcoreguidelines-pro-bounds-constant-array-index` flags non-constant
  indexes into arrays and `std::array`, and can rewrite them to `gsl::at` (its
  `GslHeader` option);
- `cppcoreguidelines-narrowing-conversions` flags the conversions `gsl::narrow`
  makes explicit.

Adopting the GSL's names, even as local aliases, is what lets those checks see
ownership and intent.

## See also

- [Microsoft GSL](https://github.com/microsoft/GSL) · [gsl-lite](https://github.com/gsl-lite/gsl-lite) ·
  [Core Guidelines: GSL](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-gsl)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) ·
  [Chromium modern C++ features](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++-features.md) ·
  [LLVM Coding Standards](https://llvm.org/docs/CodingStandards.html) ·
  [Abseil Tips](https://abseil.io/tips/)
- [Design principles](design-principles.md) ·
  [Project conventions](project-conventions.md)
