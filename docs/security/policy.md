# Security policy, review and adoption

> Turn this section into rules a machine checks: each rule names the check that enforces it, the release is gated on those checks, reviewers ask the questions no tool can, and an existing codebase adopts them in order of exposure.

## Mental model

A rule nobody checks drifts. A policy is useful when every rule has a
mechanism behind it, and a person decides only what no mechanism can:

```
rule ─▶ check (warning, clang-tidy, test, sanitizer, CI job) ─▶ blocks the change
  └──▶ no check possible ─▶ review question ─▶ recorded decision
```

The Core Guidelines are written to be enforced this way (their profiles and
"Enforcement" notes), and expect a project to pick and tailor rules rather
than adopt all at once.

## Rules and the checks that enforce them

**MUST:** For every rule you adopt, name what enforces it. The checks below
exist in GCC 15, Clang 22 and clang-tidy 22. "Here" is where this repo
applies them.

| Rule (page) | Enforced by | Here |
|---|---|---|
| No owning raw pointers; no `malloc`/`free` ([ownership](ownership-and-lifetime.md)) | clang-tidy `cppcoreguidelines-owning-memory`, `cppcoreguidelines-no-malloc` | `.clang-tidy` |
| No use after move; no dangling views ([ownership](ownership-and-lifetime.md)) | `bugprone-use-after-move`, `bugprone-dangling-handle`; Clang `-Wdangling`, GCC `-Wdangling-reference`; ASan | `.clang-tidy`, `*-asan` presets |
| Bounds are checked ([bounds](bounds.md)) | library hardening in every build; `cppcoreguidelines-pro-bounds-pointer-arithmetic`, `-pro-bounds-array-to-pointer-decay`, `-pro-bounds-constant-array-index` | `*-hardened` presets, `hardening_test.cpp` death tests |
| Untrusted sizes are checked before arithmetic ([integers](integers.md)) | `bugprone-implicit-widening-of-multiplication-result`, `bugprone-narrowing-conversions`, `bugprone-too-small-loop-variable`; UBSan, Clang `-fsanitize=integer`; boundary tests | `integers_test.cpp` |
| No reinterpretation of storage outside reviewed helpers ([object representation](object-representation.md)) | `cppcoreguidelines-pro-type-reinterpret-cast`, `-pro-type-union-access`, `-pro-type-cstyle-cast`, `-pro-type-const-cast`; each exception a `NOLINT` with its reason | `.clang-tidy`, `fuzz/targets.hpp` |
| No reads of uninitialized memory | `cppcoreguidelines-init-variables`, `-pro-type-member-init`; `-ftrivial-auto-var-init=zero` in release; MSan | `*-hardened` presets |
| Results that report failure aren't ignored ([error handling](error-handling.md)) | `[[nodiscard]]` with `-Werror=unused-result`; `bugprone-unused-return-value`, `cert-err33-c`; `bugprone-unchecked-optional-access` | `.clang-tidy` (the optional check is off in `tests/`, where it can't see GoogleTest's `ASSERT_TRUE`) |
| No exceptions escaping where they'd terminate ([error handling](error-handling.md)) | `bugprone-exception-escape`, `cert-err58-cpp` | `.clang-tidy` |
| No `printf` with a non-literal format | `-Wformat=2` (includes `-Wformat-security` on GCC and Clang); `cppcoreguidelines-pro-type-vararg` | — |
| Randomness for security comes from the OS ([cryptography](cryptography-and-secrets.md)) | `cert-msc50-cpp` (`rand`), `cert-msc51-cpp`, `bugprone-random-generator-seed` | `.clang-tidy` |
| No shell to run programs ([filesystem and processes](filesystem-and-processes.md)) | `cert-env33-c` (`system`, `popen`) | `.clang-tidy` |
| No data races ([concurrency](concurrency.md)) | TSan; Clang `-Wthread-safety` with annotated locks | `clang-sanitizers` CI job |
| Parsers survive and reject hostile input ([parsing](parsing-and-validation.md)) | fuzzing under ASan + UBSan; corpus replay; coverage of the rejection paths | `fuzz/`, `fuzz` CI job, `verification_test.cpp` |
| Authorization is checked, and denies by default ([threat modeling](threat-modeling.md)) | negative tests: the unauthorized call is denied and changes nothing | `threat-modeling_test.cpp` |
| Every request has limits ([resource limits](resource-limits.md)) | tests at, below and above each limit | `resource-limits_test.cpp` |
| Release builds are hardened ([hardening](hardening.md)) | a hardened CI build, the binary inspected (`readelf`, `dumpbin`), death tests that the checks fire | `*-hardened` presets and CI jobs |
| Inputs are pinned by content ([supply chain](supply-chain.md)) | a check that fails on a tag or unpinned action | `tools/check_workflows.py` (actions) |
| No secrets in the repository | secret scanning on push (GitHub push protection, gitleaks) | GitHub secret scanning |

**MUST:** Make a finding block the change: warnings as errors for the chosen
warnings, clang-tidy with `WarningsAsErrors`, sanitizer reports that fail the
test. A report nobody has to read gets ignored.

**SHOULD:** Keep the exceptions where the code is. A suppression names the
check and the reason (`// NOLINT(cppcoreguidelines-pro-type-reinterpret-cast): libFuzzer hands over bytes`),
so it's reviewed with the code and found by `grep`. Exceptions that need a
decision beyond the code (an accepted risk, a disabled check) go in a short
register with the reason, the owner and a review date.

## Gate the release on the checks

**MUST:** A release ships only when:

- the warnings, clang-tidy and every test pass on each supported toolchain;
- the sanitizer builds (ASan + UBSan; TSan for concurrent code) are clean;
- fuzz targets ran on the parsers that changed, and each crash was fixed and
  kept as a regression input;
- the release binary was checked for its hardening, not only its build file;
- dependencies are pinned, inventoried and checked for advisories;
- every open security exception has an owner and hasn't expired;
- someone owns the response if a vulnerability is reported tomorrow
  ([deployment](deployment.md#be-ready-to-receive-and-ship-fixes)).

These gates show that the known checks ran; they don't show that the code has
no defects ([verification](verification.md)).

## Review what no tool checks

Use these questions for code that touches a trust boundary or performs a
privileged action. Each links the page that answers it.

| Area | Question |
|---|---|
| Trust | What can an attacker control, directly or indirectly? ([threat modeling](threat-modeling.md)) |
| Authority | Which permission allows this action on this object, and where is it checked? |
| Ownership and lifetime | Who owns each resource, and can any pointer, reference, view or callback outlive it? ([ownership](ownership-and-lifetime.md)) |
| Bounds and arithmetic | What makes each index and size valid, and can computing it overflow? ([bounds](bounds.md), [integers](integers.md)) |
| State | Can anyone observe a half-updated security state, after an error or from another thread? ([error handling](error-handling.md), [concurrency](concurrency.md)) |
| Parsing | Are ambiguous, malformed and partly consumed inputs rejected? ([parsing](parsing-and-validation.md)) |
| Secrets | Where are copies made, logged, compared and kept? ([cryptography](cryptography-and-secrets.md)) |
| Resources | Is every allocation, loop, queue and wait bounded? ([resource limits](resource-limits.md)) |
| Privilege | Does this process hold more than this code needs? ([deployment](deployment.md)) |
| Evidence | Which test, sanitizer run, fuzz target or analysis shows each answer above? ([verification](verification.md)) |

The `cpp-security` skill (`.claude/skills/cpp-security/`) runs this review
with these pages.

## Adopt it in order of exposure

For an existing codebase, start where an attacker reaches first and where a
compromise costs most, not where the syntax looks oldest:

| When | Do | Why |
|---|---|---|
| First | List the entry points that take untrusted input, the privileged components and the dependencies | You can't prioritise what you haven't found ([threat modeling](threat-modeling.md)) |
| First | Turn on library hardening and the compiler and linker protections for release builds | Stops whole classes of bugs from being exploitable, without code changes ([hardening](hardening.md)) |
| First | Run the tests under ASan + UBSan, and fix what they find | Finds the memory errors your tests already reach ([verification](verification.md)) |
| Next | Fix ownership, lifetime, integer and bounds problems in the exposed code, then turn on the clang-tidy checks for it so they stay fixed | Removes the defects at their source, then keeps them out |
| Next | Fuzz the parsers; add negative tests for authorization and limits | Exercises the inputs and decisions attackers aim for |
| Next | TSan for concurrent code; review the lifetimes in callbacks and coroutines | Races and late callbacks don't show up in ordinary tests ([concurrency](concurrency.md)) |
| Ongoing | Isolate privileged work, pin and sign releases, keep a patch path | Limits what a remaining bug is worth ([deployment](deployment.md), [supply chain](supply-chain.md)) |

**The rule behind all of this:** require evidence for each security property.
Don't infer it from the language version, a coding style or a clean test run.
Modern C++ removes many assumptions the code would otherwise have to
maintain by hand, and the checks above confirm the rest.

## See also

- Core Guidelines: [In: Introduction](../guidelines/core-guidelines/introduction.md)
  (adopting the rules gradually, enforcement), [Pro: Profiles](../guidelines/core-guidelines/profile.md)
- [clang-tidy checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html) ·
  [SEI CERT C++ Coding Standard](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/)
- [Project conventions: warnings and static analysis](../guidelines/project-conventions.md#warnings-and-static-analysis) ·
  [testing](../testing.md#static-analysis-308)
- Tests: [`tests/security/policy_test.cpp`](../../tests/security/policy_test.cpp)
