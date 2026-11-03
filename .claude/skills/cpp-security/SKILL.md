---
name: cpp-security
description: Security for C++ code, using the cpp-docs repo's security section (docs/security/) together with its Core Guidelines, safety and platform-support pages. It reviews code for security (trust boundaries, untrusted input, integer and size checks, bounds, lifetimes across callbacks and coroutines, parsing, fail-closed errors, secrets, filesystem/process/FFI boundaries, resource limits), answers "how do I do X securely in C++", hardens a build for the actual compiler and standard library and shows how to confirm it, and plans the verification (sanitizers, fuzzing, static analysis, failure tests). Findings cite the security page, Core Guidelines IDs and SEI CERT IDs. Use for "/cpp-security", "is this secure", "security review", "harden this build", "what flags should I ship with", "how do I parse/validate untrusted input", "how do I handle secrets", "fuzz this", or any C++ task where an attacker controls part of the input.
---

# C++ security with the cpp-docs reference

**Security is not the same as safety.** Safety is code that doesn't misbehave
(no undefined behaviour, dangling, overflow or races); `cpp-guidelines` and
`docs/guidelines/safety.md` cover it. Security is code that holds up when
someone is *trying* to make it misbehave or misuse what it's allowed to do.
This skill starts from the attacker: what they control, what must stay
protected, and which practices and checks keep it that way. Safety defects
matter here when untrusted input can reach them.

| The task | Use |
|---|---|
| Security review, a security question, hardening a build, planning security tests | this skill |
| Write or modernize C++, choose a facility, "is X supported on Y?" | `modern-cpp` (call it for the writing step) |
| Core Guidelines audit with rule IDs, "what does R.11 say?" | `cpp-guidelines` (call it for rule-level detail) |

**Framing.** C++ written to the Core Guidelines, with explicit ownership,
bounded interfaces, checked conversions and the type, bounds and lifetime
profiles, and verified with analysis, sanitizers and fuzzing, is how secure C++
gets built. Recommend those practices; never present C++ as "not memory-safe".

**Finding the repo.** Inside a cpp-docs checkout, paths are relative to its
root. From another project, use `$CPP_DOCS` if it's set; otherwise follow the
skill's link back to its checkout (`make install` links it there):

```bash
CPP_DOCS="${CPP_DOCS:-$(realpath ~/.claude/skills/cpp-security/SKILL.md | xargs dirname | xargs dirname | xargs dirname | xargs dirname)}"
test -f "$CPP_DOCS/docs/security/README.md" || echo "not a cpp-docs checkout: set CPP_DOCS"
```

Read every page from there. `git -C "$CPP_DOCS" pull` keeps it current.

**Check each page's status first.** `docs/security/README.md` has a Status
column. A **verified** page's claims were checked against primary sources and
our compilers: cite them. An **imported** page is still the research report
the section started from: treat its claims as leads, check anything you rely
on (vendor docs, cppreference, CERT, the local compiler), and say which claims
you checked.

## 1. Learn the project and its threat model

Find out (build files, `CLAUDE.md`, CI config, or ask):

- **Toolchain:** language standard, every compiler and standard library with
  versions, optimization level of the shipped build, existing hardening flags.
  The language version doesn't turn on any protection by itself
  (`docs/security/language-versions.md`).
- **Policies:** exceptions or RTTI off, banned features, an existing
  clang-tidy, sanitizer or fuzzing setup.
- **What an attacker controls** (`docs/security/threat-modeling.md`): network
  messages, files, IPC, command-line arguments, environment, configuration,
  plugins, callbacks. Trace that input to where it's parsed and to what it can
  cause (allocation sizes, indexes, paths, commands, privileged actions).
- **What must stay protected:** secrets and credentials, privileged
  operations, other tenants' data, availability.

If the code doesn't show where its input comes from, say what you assumed
(e.g. "`len` comes from the packet header, so it's attacker-controlled") or
ask. Data that's trusted by construction doesn't need the same checks; say why
it's trusted.

## 2. Load only the pages that apply

| The code does | Read (under `docs/security/`) | Also |
|---|---|---|
| Takes input across a trust boundary; privileged actions | `threat-modeling.md` | Core Guidelines P.4–P.8, I.* |
| Owns or borrows objects across callbacks, queues, threads | `ownership-and-lifetime.md` | `docs/guidelines/safety.md`, R.*, F.42–F.56, Lifetime.* |
| Indexes, offsets, ranges from input | `bounds.md` | Bounds.*, `docs/examples/containers/views/span.md` |
| Sizes, lengths, counts, arithmetic on input | `integers.md` | ES.100–ES.107, `docs/examples/numerics/saturation.md`, `docs/examples/utilities/utility.md` (`cmp_*`, `in_range`) |
| Sends structs over the wire; casts; uninitialized state | `object-representation.md` | ES.20, Type.*, `docs/examples/language/basic-concepts/erroneous-behaviour.md` |
| Parses text, numbers, formats, regexes, nested data | `parsing-and-validation.md` | `docs/examples/text-processing/charconv.md`, `formatting.md`, `regex.md` |
| Shares state between threads; async; coroutines | `concurrency.md` | CP.*, `docs/examples/concurrency/`, `docs/examples/language/coroutines/coroutines.md` |
| Makes authorization or other security decisions that can fail | `error-handling.md` | E.*, `docs/examples/utilities/expected.md` |
| Random tokens, keys, passwords, comparisons of secrets, TLS | `cryptography-and-secrets.md` | `docs/examples/numerics/random.md` |
| Paths, files, launching processes, loading libraries, C interfaces | `filesystem-and-processes.md` | `docs/examples/io/filesystem.md` |
| Unbounded work: recursion, allocation, queues, decompression | `resource-limits.md` | |
| Build flags, library hardening, binary protections | `hardening.md`, `language-versions.md` | `docs/guidelines/safety.md#hardening-in-practice`, `docs/guidelines/build-and-packaging.md` |
| Testing for security: sanitizers, fuzzing, static analysis | `verification.md` | `docs/guidelines/tooling.md`, `docs/guidelines/testing.md` |
| Dependencies, release artifacts, CI | `supply-chain.md` | |
| Deployment: privileges, isolation, logs, crash reports | `deployment.md` | |
| A team policy, release gate or review checklist | `policy.md` | |

Find a Core Guidelines rule's text and our BAD/GOOD example through
`docs/guidelines/core-guidelines/README.md` (or let `cpp-guidelines` do it).
SEI CERT rules are cited by ID and linked to
<https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/>
(and the CERT C standard for C-level rules); don't copy CERT's text.

## 3. Review: walk the checklist

For code that touches a trust boundary or a privileged action, answer each
question (the full checklist is in `docs/security/policy.md`):

| Area | Question |
|---|---|
| Trust | What can an attacker control, directly or indirectly? |
| Authority | What specific permission allows this action on this object, and is it checked separately from parsing? |
| Ownership and lifetime | Can a pointer, reference, iterator, view or callback outlive what it refers to, including after a suspension or in a queued task? |
| Bounds | What establishes that every index, extent and range is valid, before the access? |
| Arithmetic | Can a size, offset or count from input overflow, wrap, narrow or change sign, including inside the check itself? |
| State | Can an operation see a half-initialized or half-updated security state? |
| Parsing | Are trailing data, duplicates, unknown values, bad encodings and inconsistent lengths rejected as specified? Is the work bounded? |
| Failure | Does every error, timeout and exception leave a defined state, and deny rather than allow? |
| Secrets | Are copies, logs, comparisons and retention controlled? Is the random source cryptographic? |
| Filesystem, processes, FFI | Check-then-use races, symlinks, shell interpretation, library loading, ownership across C interfaces? |
| Resources | Are memory, work, queue length, handles and time bounded per request and per client? |
| Build | Which library hardening and binary protections are on in the *shipped* (optimized) build? |
| Evidence | Which test, sanitizer run, fuzz target or analysis shows each property? |

### Report findings, most exploitable first

```markdown
- **<file>:<line> · <short name>** (<docs/security/page.md#anchor> · <Core Guidelines ID> · <CERT ID>)
  Attack: <what the attacker sends and what they get, one sentence>.
  Fix: <minimal change>. Test: <the test, sanitizer run or fuzz input that shows it's fixed>.
```

Order: remote code execution and memory corruption reachable from input, then
authorization bypass and secret disclosure, then denial of service, then
defence in depth (hardening, missing tests). A compliant result is a line too:
what the code does right and which checks it already has.

Don't over-flag:

- **Trusted data** (compile-time constants, values the program computed and
  bounded itself) doesn't need input validation; say why it's trusted.
- **A check that's already correct** is a result, not a finding. Read the
  whole path: validation often happens a few calls earlier.
- **Don't propose debug-only checks as the fix.** `assert`, debug-mode
  containers and contract assertions (which can be evaluated with the
  *ignore* semantic) don't run in every build; required validation is
  ordinary code.
- **Hardening is defence in depth, not the fix.** Library hardening and
  binary protections catch or block what was missed; fix the defect too.
- **Don't swap a correct facility for another correct one** to look more
  secure (e.g. `.at()` where an explicit check already precedes `[]`).

## 4. Harden a build

1. **Identify the actual toolchain and standard library** of the shipped
   build, not the compiler's name: GCC/libstdc++, Clang with libc++ or
   libstdc++, Apple Clang, MSVC/MS STL.
2. **Measure what's on now**, in the shipped configuration (optimization
   level included). For example, libstdc++ 15+ enables its assertions only
   in unoptimized builds:
   `g++ -std=c++20 -O2 -dM -E -include vector -x c++ /dev/null | grep -E '_GLIBCXX_ASSERTIONS|_FORTIFY_SOURCE'`;
   for libc++, check `_LIBCPP_HARDENING_MODE` the same way.
3. **Choose the switches** from `docs/security/hardening.md` and
   `docs/security/language-versions.md` for that toolchain: library hardening,
   fortification, stack protection, position independence and linker
   protections, control-flow protection. Prefer what the page marks verified;
   for an imported row, check the vendor's documentation first.
4. **Confirm in the artifact**: the macros in the build, the linker and binary
   properties (`readelf`, `checksec`, `dumpbin`), and a deliberate
   out-of-bounds access in a test that the hardened build stops.
5. **Keep test and production builds separate**: sanitizer builds are for
   testing; ship the hardening profile.

## 5. Plan the verification

Use `docs/security/verification.md`, `docs/guidelines/tooling.md` and
`docs/guidelines/testing.md`:

- ASan + UBSan, TSan (separately) and MSan where the toolchain supports them,
  over tests that reach the input paths.
- A fuzz target for every parser or decoder of untrusted input, with a corpus,
  limits on input size and time, and minimized crashes kept as regression
  tests.
- Static analysis (clang-tidy `cert-*`, `cppcoreguidelines-*`, `bugprone-*`)
  configured for the real build; findings fixed or justified.
- Tests for the failure paths: malformed input, boundary sizes, failed
  allocation, failed authorization, timeouts, cancellation.

## 6. Write and validate

- Use `modern-cpp` for the code itself (facility choice, toolchain support)
  and `cpp-guidelines` for rule detail.
- Compile with the project's flags; warnings stay at zero.
- Run the tests, the sanitizer builds and, for a parser, the fuzz target for
  a short time.
- Say what you couldn't validate, on which compiler and library, and which
  claims came from an imported page you didn't check.

## 7. Cite, and feed gaps back

Name the pages and evidence you relied on (`docs/security/integers.md#…`,
CERT INT30-C, `docs/platform-support/notes.md`). If a security page was
missing, wrong or still imported where you needed it verified, say so and
offer an issue in homer6/cpp-docs (the security epic is #286).
